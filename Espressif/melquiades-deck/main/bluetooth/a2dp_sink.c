#include "a2dp_sink.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"
#include "../audio/audio_output.h"
#include "bluetooth_common.h"

#define BT_A2DP_TAG "Melquiades_Deck_A2DP"
#define BT_DEVICE_NAME "Melquiades-Deck"

static uint32_t m_pkt_cnt = 0;
static esp_a2d_audio_state_t m_audio_state = ESP_A2D_AUDIO_STATE_STOPPED;
static const char *m_a2d_conn_state_str[] = {"Desconectado", "Conectando", "Conectado", "Desconectando"};
static const char *m_a2d_audio_state_str[] = {"Suspendido", "Parado", "Iniciado"};

static void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
static void bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
static void bt_app_avrc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);
static void bt_app_a2d_data_cb(const uint8_t *data, uint32_t len);

void init_a2dp_sink(void)
{
    esp_err_t ret;

    ESP_LOGI(BT_A2DP_TAG, "Inicializando A2DP Sink");

    // Usar la inicialización común del Bluetooth
    if ((ret = init_bluetooth_common()) != ESP_OK) {
        ESP_LOGE(BT_A2DP_TAG, "Inicialización Bluetooth común falló: %s", esp_err_to_name(ret));
        return;
    }

    /* Inicializar el hardware de audio */
    audio_output_init();

    /* Registrar el callback GAP para gestionar eventos GAP como detección de dispositivos */
    if ((ret = esp_bt_gap_register_callback(bt_app_gap_cb)) != ESP_OK) {
        ESP_LOGE(BT_A2DP_TAG, "Registro de callback GAP falló: %s", esp_err_to_name(ret));
        return;
    }

    /* Registrar el callback A2DP sink para manejar eventos A2DP */
    if ((ret = esp_a2d_register_callback(bt_app_a2d_cb)) != ESP_OK) {
        ESP_LOGE(BT_A2DP_TAG, "Registro de callback A2DP falló: %s", esp_err_to_name(ret));
        return;
    }

    /* Registrar el callback de datos A2DP para recibir el streaming de audio */
    if ((ret = esp_a2d_sink_register_data_callback(bt_app_a2d_data_cb)) != ESP_OK) {
        ESP_LOGE(BT_A2DP_TAG, "Registro de callback de datos A2DP falló: %s", esp_err_to_name(ret));
        return;
    }

    /* Inicializar el servicio A2DP sink */
    if ((ret = esp_a2d_sink_init()) != ESP_OK) {
        ESP_LOGE(BT_A2DP_TAG, "Inicialización de A2DP sink falló: %s", esp_err_to_name(ret));
        return;
    }

    /* Inicializar AVRCP como controlador */
    if ((ret = esp_avrc_ct_init()) != ESP_OK) {
        ESP_LOGE(BT_A2DP_TAG, "Inicialización de AVRCP CT falló: %s", esp_err_to_name(ret));
        return;
    }

    /* Registrar callbacks AVRP */
    if ((ret = esp_avrc_ct_register_callback(bt_app_avrc_ct_cb)) != ESP_OK) {
        ESP_LOGE(BT_A2DP_TAG, "Registro de callback AVRCP CT falló: %s", esp_err_to_name(ret));
        return;
    }

    /* Configurar Bluetooth como visible y conectable */
    if ((ret = esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE)) != ESP_OK) {
        ESP_LOGE(BT_A2DP_TAG, "Configuración del modo de escaneo falló: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(BT_A2DP_TAG, "A2DP sink inicializado correctamente - Esperando conexión...");
}

static void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_BT_GAP_AUTH_CMPL_EVT: {
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(BT_A2DP_TAG, "Autenticación exitosa: %s", param->auth_cmpl.device_name);
        } else {
            ESP_LOGE(BT_A2DP_TAG, "Autenticación fallida: %d", param->auth_cmpl.stat);
        }
        break;
    }
    default:
        break;
    }
}

static void bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
{
    switch (event) {
    case ESP_A2D_CONNECTION_STATE_EVT: {
        ESP_LOGI(BT_A2DP_TAG, "Estado de conexión A2DP cambiado: %s, [%02x:%02x:%02x:%02x:%02x:%02x]",
                 m_a2d_conn_state_str[param->conn_stat.state],
                 param->conn_stat.remote_bda[0], param->conn_stat.remote_bda[1],
                 param->conn_stat.remote_bda[2], param->conn_stat.remote_bda[3],
                 param->conn_stat.remote_bda[4], param->conn_stat.remote_bda[5]);

        if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
            // Reiniciar contador de paquetes cuando se desconecta
            m_pkt_cnt = 0;
        }
        break;
    }
    case ESP_A2D_AUDIO_STATE_EVT: {
        ESP_LOGI(BT_A2DP_TAG, "Estado de audio A2DP cambiado: %s",
                 m_a2d_audio_state_str[param->audio_stat.state]);
        m_audio_state = param->audio_stat.state;
        break;
    }
    case ESP_A2D_AUDIO_CFG_EVT: {
        ESP_LOGI(BT_A2DP_TAG, "Configuración de audio A2DP: %d", param->audio_cfg.mcc.type);
        // La frecuencia de muestreo cambia en función del dispositivo conectado
        uint16_t sample_rate = 16000;
        char oct0 = param->audio_cfg.mcc.cie.sbc[0];
        if (oct0 & (0x01 << 6)) {
            sample_rate = 32000;
        } else if (oct0 & (0x01 << 5)) {
            sample_rate = 44100;
        } else if (oct0 & (0x01 << 4)) {
            sample_rate = 48000;
        }
        ESP_LOGI(BT_A2DP_TAG, "Configure audio player: %d", sample_rate);
        // Configurar la salida de audio
        audio_output_set_sample_rate(sample_rate);
        break;
    }
    default:
        ESP_LOGI(BT_A2DP_TAG, "Evento A2DP no gestionado: %d", event);
        break;
    }
}

static void bt_app_a2d_data_cb(const uint8_t *data, uint32_t len)
{
    // Esta función recibe los datos de audio decodificados
    if (++m_pkt_cnt % 100 == 0) {
        ESP_LOGI(BT_A2DP_TAG, "Audio recibido: paquete n° %u, longitud %u", m_pkt_cnt, len);
    }
    // Enviar los datos de audio al DAC
    audio_output_write((uint8_t *)data, len);
}

static void bt_app_avrc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param)
{
    switch (event) {
    case ESP_AVRC_CT_CONNECTION_STATE_EVT: {
        ESP_LOGI(BT_A2DP_TAG, "Estado de conexión AVRC cambiado: %d", param->conn_stat.connected);
        break;
    }
    default:
        ESP_LOGI(BT_A2DP_TAG, "Evento AVRC no gestionado: %d", event);
        break;
    }
}