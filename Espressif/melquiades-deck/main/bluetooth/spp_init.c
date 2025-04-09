#include "spp_init.h"
// Bibliotecas de sistema
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
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
#include "bluetooth_common.h"
#include "a2dp_sink.h"

// Definiciones de archivo
#define SPP_TAG "Melquiades_Deck_SPP"
#define SPP_SERVER_NAME "Melquiades_Deck_ESP32"
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

    ESP_LOGI(SPP_TAG, "Inicializando Bluetooth SPP...");

    // Usar la inicialización común del Bluetooth
    if ((ret = init_bluetooth_common()) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "Inicialización Bluetooth común falló: %s", esp_err_to_name(ret));
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
            /*****COMANDOS PARA LED*****/
            if (strcmp(cmd.data, "led_board start") == 0)
            {
                if (tasl_ledboard_handle == NULL)
                {
                    xTaskCreate(manage_led_board, "manage_led_board", 2048, NULL, 5, &tasl_ledboard_handle);
                    printf("Encendido en el led de la board ON.\n");
                }
                else
                {
                    printf("La tarea de encender led ya está en ejecución.\n");
                }
            }
            else if (strcmp(cmd.data, "led_board stop") == 0)
            {
                if (tasl_ledboard_handle != NULL)
                {
                    vTaskDelete(tasl_ledboard_handle);
                    tasl_ledboard_handle = NULL;
                    printf("Se pausa encendido de led.\n");
                }
            }
            /*****COMANDOS PARA SENSORES*****/
            else if (strcmp(cmd.data, "sensors start") == 0)
            {
                if (sensors_streaming_uart)
                {
                    printf("Actualmente nos encontramos transmitiendo data via UART.\n");
                }
                else
                {
                    // Cambiamos estado global a true para no incurrir en recreacion de tareas
                    sensors_streaming_uart = true;
                    if (task_pot_handle == NULL)
                    {
                        xTaskCreate(read_potentiometers, "read_potentiometers", 2048, NULL, 5, &task_pot_handle);
                    }
                    if (task_btn_handle == NULL)
                    {
                        xTaskCreate(leer_pulsadores, "leer_pulsadores", 2048, NULL, 5, &task_btn_handle);
                    }
                }
            }
            else if (strcmp(cmd.data, "sensors stop") == 0)
            {
                if (sensors_streaming_uart && !sensors_streaming_bt)
                {
                    sensors_streaming_uart = false;
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
                }
                else if (sensors_streaming_uart && sensors_streaming_bt)
                {
                    sensors_streaming_uart = false;
                    printf("Se cierra streaming en UART, BT sigue transmitiendo.\n");
                }
                else
                {
                    printf("No nos encontramos realizando ningun tipo de streaming.\n");
                }
            }
            /*****COMANDOS PARA VOLUMEN*****/
            else if (strncmp(cmd.data, "set_volume ", 11) == 0)
            {
                char *param = cmd.data + 11;
                if (isdigit((unsigned char)param[0]))
                {
                    int volumen = atoi(param);
                    set_volume(volumen);
                    printf("Se cambia volumen a %d.\n", volumen);
                }
                else
                {
                    printf("Error: volumen inválido.\n");
                }
            }
            /*****COMANDOS PARA ECUALIZADOR*****/
            else if (strncmp(cmd.data, "eq ", 3) == 0)
            {
                char *param = cmd.data + 3;
                if (strcmp(param, "flat") == 0)
                {
                    set_eq_preset(EQ_FLAT);
                    printf("Se cambia ecualizacion a EQ_FLAT.\n");
                }
                else if (strcmp(param, "bass_boost") == 0)
                {
                    set_eq_preset(EQ_BASS_BOOST);
                    printf("Se cambia ecualizacion a EQ_BASS_BOOST.\n");
                }
                else if (strcmp(param, "mid_boost") == 0)
                {
                    set_eq_preset(EQ_MID_BOOST);
                    printf("Se cambia ecualizacion a EQ_MID_BOOST.\n");
                }
                else if (strcmp(param, "treble_boost") == 0)
                {
                    set_eq_preset(EQ_TREBLE_BOOST);
                    printf("Se cambia ecualizacion a EQ_TREBLE_BOOST.\n");
                }
                else if (strcmp(param, "vocal") == 0)
                {
                    set_eq_preset(EQ_VOCAL);
                    printf("Se cambia ecualizacion a EQ_VOCAL.\n");
                }
            }
            /*****COMANDOS PARA BALANCE*****/
            else if (strncmp(cmd.data, "headphone_balance ", 18) == 0)
            {
                char *param = cmd.data + 18;
                // Saltar espacios iniciales
                while (*param == ' ') param++;
                if (isdigit((unsigned char)param[0]) || param[0] == '-' || param[0] == '.')
                {
                    float value = strtof(param, NULL);
                    set_balance(value);
                    printf("Se cambia balance a %.2f\n", value);
                }
                else
                {
                    printf("Error: balance inválido.\n");
                }     
            }
            /*****COMANDOS PARA DSP*****/
            else if (strcmp(cmd.data, "dsp enabled") == 0)
            {
                set_dsp_enabled(true);
                printf("Se activa DSP.\n");
            }
            else if (strcmp(cmd.data, "dsp disabled") == 0)
            {
                set_dsp_enabled(false);
                printf("Se desactiva DSP.\n");
            }
            /*****OTROS*****/
            else if (strcmp(cmd.data, "help") == 0)
            {
                printf(cmd_commands);
            }
            else
            {
                printf("Comando no válido. Escriba help para listado de comandos validos:\n");
            }
        }
    }
}
