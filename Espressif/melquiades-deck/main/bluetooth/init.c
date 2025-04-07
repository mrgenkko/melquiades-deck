#include "init.h"
// Bibliotecas de sistema
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_spp_api.h"
// Bibliotecas custom
#include "../state.h"
#include "../leds/board.h"
#include "../sensors/buttons.h"
#include "../sensors/potentiometers.h"

// Definiciones de archivo
#define SPP_TAG "ESP32_SPP"
#define SPP_SERVER_NAME "Melquiades_ESP32"
#define BT_DEVICE_NAME "Melquiades-Deck"

static const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;
static const esp_spp_sec_t sec_mask = ESP_SPP_SEC_AUTHENTICATE;
static const esp_spp_role_t role_slave = ESP_SPP_ROLE_SLAVE;

static uint32_t spp_handle = 0;
static QueueHandle_t cmd_queue;
static bool bt_connected = false;

// Estructura para mensajes en la cola
typedef struct
{
    char data[64];
    int len;
} bt_cmd_t;

// Callback para eventos GAP
static void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_BT_GAP_AUTH_CMPL_EVT:
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGI(SPP_TAG, "Autenticación exitosa: %s", param->auth_cmpl.device_name);
        }
        else
        {
            ESP_LOGE(SPP_TAG, "Autenticación fallida: %d", param->auth_cmpl.stat);
        }
        break;
    default:
        break;
    }
}

// Callback para eventos SPP
static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    bt_cmd_t cmd;

    switch (event)
    {
    case ESP_SPP_INIT_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_INIT_EVT");
        // Iniciar servidor SPP después de la inicialización
        esp_spp_start_srv(sec_mask, role_slave, 0, SPP_SERVER_NAME);
        break;

    case ESP_SPP_START_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_START_EVT");
        // Establecer el nombre del dispositivo después de iniciar SPP
        esp_bt_dev_set_device_name(BT_DEVICE_NAME);
        // Configurar el dispositivo como visible y conectable
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        break;

    case ESP_SPP_SRV_OPEN_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_SRV_OPEN_EVT - Cliente conectado");
        spp_handle = param->srv_open.handle;
        bt_connected = true;

        // Enviar mensaje de bienvenida
        const char *welcome_msg = "Bienvenido a Melquiades Deck\r\n";
        esp_spp_write(spp_handle, strlen(welcome_msg), (uint8_t *)welcome_msg);

        // Enviar menú de ayuda        
        esp_spp_write(spp_handle, strlen(cmd_commands), (uint8_t *)cmd_commands);
        break;

    case ESP_SPP_CLOSE_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_CLOSE_EVT - Cliente desconectado");
        bt_connected = false;
        break;

    case ESP_SPP_DATA_IND_EVT:
        // Recepción de datos
        ESP_LOGI(SPP_TAG, "ESP_SPP_DATA_IND_EVT len=%d", param->data_ind.len);

        // Procesar solo comandos de tamaño razonable
        if (param->data_ind.len < sizeof(cmd.data) - 1)
        {
            // Copiar datos al buffer y asegurar terminación con cero
            memcpy(cmd.data, param->data_ind.data, param->data_ind.len);
            cmd.data[param->data_ind.len] = '\0';
            cmd.len = param->data_ind.len;

            // Eliminar caracteres de retorno de carro y nueva línea
            for (int i = 0; i < cmd.len; i++)
            {
                if (cmd.data[i] == '\r' || cmd.data[i] == '\n')
                {
                    cmd.data[i] = '\0';
                }
            }

            ESP_LOGI(SPP_TAG, "Comando recibido: %s", cmd.data);

            // Enviar a la cola de comandos si no está vacío
            if (strlen(cmd.data) > 0)
            {
                xQueueSend(cmd_queue, &cmd, portMAX_DELAY);

                // Enviar eco del comando
                char echo[80];
                snprintf(echo, sizeof(echo), "Comando: %s\r\n", cmd.data);
                esp_spp_write(spp_handle, strlen(echo), (uint8_t *)echo);
            }
        }
        break;

    case ESP_SPP_WRITE_EVT:
        // No es necesario hacer nada después de escribir
        break;

    default:
        break;
    }
}

// Función para enviar respuesta por Bluetooth si hay conexión
void send_bt_response(const char *response)
{
    if (bt_connected && spp_handle > 0)
    {
        esp_spp_write(spp_handle, strlen(response), (uint8_t *)response);
    }
}

// Función para inicializar Bluetooth
void init_bluetooth(void)
{
    esp_err_t ret;
    cmd_queue = xQueueCreate(10, sizeof(bt_cmd_t));

    ESP_LOGI(SPP_TAG, "Inicializando Bluetooth...");

    // Liberar memoria BLE si solo usaremos Bluetooth clásico
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    // Configuración del controlador Bluetooth
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    // Inicializar controlador BT
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK)
    {
        ESP_LOGE(SPP_TAG, "Inicialización del controlador BT falló: %s", esp_err_to_name(ret));
        return;
    }

    // Habilitar controlador BT en modo clásico
    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK)
    {
        ESP_LOGE(SPP_TAG, "Habilitación del controlador BT falló: %s", esp_err_to_name(ret));
        return;
    }

    // Inicializar Bluedroid
    if ((ret = esp_bluedroid_init()) != ESP_OK)
    {
        ESP_LOGE(SPP_TAG, "Inicialización de Bluedroid falló: %s", esp_err_to_name(ret));
        return;
    }

    // Habilitar Bluedroid
    if ((ret = esp_bluedroid_enable()) != ESP_OK)
    {
        ESP_LOGE(SPP_TAG, "Habilitación de Bluedroid falló: %s", esp_err_to_name(ret));
        return;
    }

    // Registrar callback para GAP
    if ((ret = esp_bt_gap_register_callback(esp_bt_gap_cb)) != ESP_OK)
    {
        ESP_LOGE(SPP_TAG, "Registro de callback GAP falló: %s", esp_err_to_name(ret));
        return;
    }

    // Registrar callback para SPP
    if ((ret = esp_spp_register_callback(esp_spp_cb)) != ESP_OK)
    {
        ESP_LOGE(SPP_TAG, "Registro de callback SPP falló: %s", esp_err_to_name(ret));
        return;
    }

    // Inicializar SPP
    if ((ret = esp_spp_init(esp_spp_mode)) != ESP_OK)
    {
        ESP_LOGE(SPP_TAG, "Inicialización de SPP falló: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(SPP_TAG, "Bluetooth inicializado correctamente");
}

// Tarea para procesar comandos de la cola
void bt_shell_task(void *pvParameter)
{
    bt_cmd_t cmd;
    char response[512];

    while (1)
    {
        if (xQueueReceive(cmd_queue, &cmd, portMAX_DELAY))
        {
            // Procesar comando recibido
            if (strcmp(cmd.data, "help") == 0)
            {                
                send_bt_response(cmd_commands);
            }
            else if (strcmp(cmd.data, "start_led_board") == 0)
            {
                //Validamos que no este el task de led activo                
                if (tasl_ledboard_handle == NULL)
                {
                    xTaskCreate(manage_led_board, "manage_led_board", 2048, NULL, 5, &tasl_ledboard_handle);
                    snprintf(response, sizeof(response), "Encendido el led de la board.\r\n");
                }
                else
                {
                    snprintf(response, sizeof(response), "La tarea ya se encuentra en ejecucion.\r\n");
                }
                send_bt_response(response);
            }
            else if (strcmp(cmd.data, "stop_led_board") == 0)
            {
                // Aquí irá tu código para detener la tarea del LED
                snprintf(response, sizeof(response), "Deteniendo LED...\r\n");
                if (tasl_ledboard_handle != NULL)
                {
                    vTaskDelete(tasl_ledboard_handle);
                    tasl_ledboard_handle = NULL;
                    snprintf(response, sizeof(response), "Se pausa encendido de LED.\r\n");
                }
                send_bt_response(response);
            }
            // Iniciamos seccion de sensores
            else if (strcmp(cmd.data, "start_sensors") == 0)
            {
                if (sensors_streaming_bt)
                {
                    snprintf(response, sizeof(response), "Actualmente nos encontramos transmitiendo data via BT.\r\n");
                    send_bt_response(response);
                }
                else
                {
                    // Cambiamos estado global a true para no incurrir en recreacion de tareas
                    sensors_streaming_bt = true;
                    if (task_pot_handle == NULL)
                    {
                        xTaskCreate(read_potentiometers, "read_potentiometers", 2048, NULL, 5, &task_pot_handle);
                    }
                    if (task_btn_handle == NULL)
                    {
                        xTaskCreate(leer_pulsadores, "leer_pulsadores", 2048, NULL, 5, &task_btn_handle);
                    }
                    snprintf(response, sizeof(response), "Se inicia streaming via BT.\r\n");
                    send_bt_response(response);
                }
            }
            else if (strcmp(cmd.data, "stop_sensors") == 0)
            {
                if (sensors_streaming_bt && !sensors_streaming_uart)
                {
                    sensors_streaming_bt = false;
                    if (task_pot_handle != NULL)
                    {
                        vTaskDelete(task_pot_handle);
                        task_pot_handle = NULL;
                    }
                    if (task_btn_handle != NULL)
                    {
                        vTaskDelete(task_btn_handle);
                        task_btn_handle = NULL;
                    }
                    snprintf(response, sizeof(response), "Se cierra streaming en BT.\r\n");
                    send_bt_response(response);
                }
                else if (sensors_streaming_bt && sensors_streaming_uart)
                {
                    sensors_streaming_bt = false;
                    snprintf(response, sizeof(response), "Se cierra streaming en BT, UART sigue transmitiendo.\r\n");
                    send_bt_response(response);
                }
                else
                {
                    snprintf(response, sizeof(response), "No nos encontramos realizando ningun tipo de streaming.\r\n");
                    send_bt_response(response);
                }
            }

            else
            {
                snprintf(response, sizeof(response), "Comando desconocido: %s\r\nEscriba 'help' para obtener ayuda\r\n", cmd.data);
                send_bt_response(response);
            }
        }
    }
}
