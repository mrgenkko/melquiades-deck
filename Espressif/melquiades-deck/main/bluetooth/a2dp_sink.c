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

// Agregar comandos para controlar DSP vía AVRC
#define CUSTOM_AVRC_CMD_DSP_TOGGLE 0x01
#define CUSTOM_AVRC_CMD_EQ_PRESET  0x02
#define CUSTOM_AVRC_CMD_BALANCE    0x03

static uint32_t m_pkt_cnt = 0;
static esp_a2d_audio_state_t m_audio_state = ESP_A2D_AUDIO_STATE_STOPPED;
static const char *m_a2d_conn_state_str[] = {"Desconectado", "Conectando", "Conectado", "Desconectando"};
static const char *m_a2d_audio_state_str[] = {"Suspendido", "Parado", "Iniciado"};


static const struct {
    const char* name;
    float bass;
    float mid;
    float treble;
} eq_presets[EQ_MAX_PRESETS] = {
    {"Plano",          0.0f,  0.0f,  0.0f},
    {"Refuerzo Bajo", 10.0f,  0.0f, -2.0f},
    {"Refuerzo Medio", -2.0f,  8.0f, -2.0f},
    {"Refuerzo Agudo", -2.0f,  0.0f, 10.0f},
    {"Vocal",          -3.0f,  6.0f,  3.0f}
};

// Estado actual del DSP
static struct {
    bool enabled;
    eq_preset_t eq_preset;
    uint8_t volume;
    float balance;  // -1.0 (izq) a 1.0 (der)
} dsp_state = {
    .enabled = true,
    .eq_preset = EQ_FLAT,
    .volume = 75,     // 75% volumen por defecto
    .balance = 0.0f   // Balance centrado
};

static void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
static void bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
static void bt_app_avrc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);
static void bt_app_a2d_data_cb(const uint8_t *data, uint32_t len);
static void apply_dsp_settings(void);

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
    
    // Aplicar configuración inicial de DSP
    apply_dsp_settings();

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

static void apply_dsp_settings(void)
{
    // Activar/desactivar DSP
    audio_output_enable_dsp(dsp_state.enabled);
    
    // Aplicar preset de EQ
    eq_preset_t preset = dsp_state.eq_preset;
    if (preset < EQ_MAX_PRESETS) {
        audio_output_set_eq(
            eq_presets[preset].bass,
            eq_presets[preset].mid,
            eq_presets[preset].treble
        );
        ESP_LOGI(BT_A2DP_TAG, "Aplicando preset EQ: %s", eq_presets[preset].name);
    }
    
    // Aplicar volumen
    audio_output_set_volume(dsp_state.volume);
    
    // Aplicar balance
    float balance = dsp_state.balance;
    if (balance < 0) {
        // Aumentar canal izquierdo, atenuar derecho
        audio_output_set_channel_balance(0.0f, balance * -10.0f);
    } else if (balance > 0) {
        // Aumentar canal derecho, atenuar izquierdo
        audio_output_set_channel_balance(balance * -10.0f, 0.0f);
    } else {
        // Balance neutral
        audio_output_set_channel_balance(0.0f, 0.0f);
    }
}

void set_dsp_enabled(bool enabled)
{
    dsp_state.enabled = enabled;
    apply_dsp_settings();
    ESP_LOGI(BT_A2DP_TAG, "DSP %s", enabled ? "activado" : "desactivado");
}

void set_eq_preset(eq_preset_t preset)
{
    if (preset < EQ_MAX_PRESETS) {
        dsp_state.eq_preset = preset;
        apply_dsp_settings();
        ESP_LOGI(BT_A2DP_TAG, "Preset EQ cambiado a: %s", eq_presets[preset].name);
    }
}

void set_volume(uint8_t volume)
{
    if (volume > 100) {
        volume = 100;
    }
    dsp_state.volume = volume;
    apply_dsp_settings();
    ESP_LOGI(BT_A2DP_TAG, "Volumen configurado: %d%%", volume);
}

void set_balance(float balance)
{
    // Limitar entre -1.0 y 1.0
    if (balance < -1.0f) balance = -1.0f;
    if (balance > 1.0f) balance = 1.0f;
    
    dsp_state.balance = balance;
    apply_dsp_settings();
    
    if (balance < 0) {
        ESP_LOGI(BT_A2DP_TAG, "Balance configurado: %.1f%% (hacia izquierda)", balance * -100.0f);
    } else if (balance > 0) {
        ESP_LOGI(BT_A2DP_TAG, "Balance configurado: %.1f%% (hacia derecha)", balance * 100.0f);
    } else {
        ESP_LOGI(BT_A2DP_TAG, "Balance configurado: centrado");
    }
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
    /*
    if (++m_pkt_cnt % 100 == 0) {
        ESP_LOGI(BT_A2DP_TAG, "Audio recibido: paquete n° %u, longitud %u", m_pkt_cnt, len);
    }
    */
    
    // Enviar los datos de audio al procesador DSP y luego al DAC
    audio_output_write((uint8_t *)data, len);
}

static void bt_app_avrc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param)
{
    switch (event) {
    case ESP_AVRC_CT_CONNECTION_STATE_EVT: {
        ESP_LOGI(BT_A2DP_TAG, "Estado de conexión AVRC cambiado: %d", param->conn_stat.connected);
        break;
    }
    case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT: {
        // Aquí podrías manejar respuestas a comandos personalizados si se extendiera AVRC
        break;
    }
    default:
        ESP_LOGI(BT_A2DP_TAG, "Evento AVRC no gestionado: %d", event);
        break;
    }
}

// Estas funciones podrían ser expuestas en el archivo .h si quieres controlarlas desde otra parte del código

void dsp_toggle_enabled(void)
{
    set_dsp_enabled(!dsp_state.enabled);
}

void dsp_next_eq_preset(void)
{
    eq_preset_t next = (dsp_state.eq_preset + 1) % EQ_MAX_PRESETS;
    set_eq_preset(next);
}

void dsp_volume_up(void)
{
    uint8_t new_vol = dsp_state.volume + 5;
    if (new_vol > 100) new_vol = 100;
    set_volume(new_vol);
}

void dsp_volume_down(void)
{
    uint8_t new_vol = dsp_state.volume;
    if (new_vol < 5) new_vol = 0;
    else new_vol -= 5;
    set_volume(new_vol);
}

void dsp_balance_left(void)
{
    float new_balance = dsp_state.balance - 0.1f;
    if (new_balance < -1.0f) new_balance = -1.0f;
    set_balance(new_balance);
}

void dsp_balance_right(void)
{
    float new_balance = dsp_state.balance + 0.1f;
    if (new_balance > 1.0f) new_balance = 1.0f;
    set_balance(new_balance);
}

void dsp_balance_center(void)
{
    set_balance(0.0f);
}