#include "audio_output.h"
#include "audio_dsp.h"
#include "driver/i2s.h"
#include "esp_log.h"

#define TAG "AUDIO_OUTPUT"

// Definición de pines I2S
#define I2S_DOUT 23  // Data out
#define I2S_BCLK 18  // Bit clock
#define I2S_LRC  5   // Word/LR clock

// Configuración I2S
#define I2S_NUM           I2S_NUM_0
#define I2S_SAMPLE_RATE   44100
#define I2S_BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_16BIT
#define I2S_CHANNEL_FORMAT I2S_CHANNEL_FMT_RIGHT_LEFT
#define I2S_COMM_FORMAT   I2S_COMM_FORMAT_STAND_I2S
#define DMA_BUF_COUNT     8
#define DMA_BUF_LEN       64

// Buffer para procesamiento DSP
static uint8_t* dsp_buffer = NULL;
static size_t dsp_buffer_size = 0;
static dsp_config_t dsp_config;
static bool dsp_enabled = true;  // Activar DSP por defecto

static i2s_config_t i2s_config = {
    .mode = I2S_MODE_MASTER | I2S_MODE_TX,
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE,
    .channel_format = I2S_CHANNEL_FORMAT,
    .communication_format = I2S_COMM_FORMAT,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = DMA_BUF_COUNT,
    .dma_buf_len = DMA_BUF_LEN,
    .use_apll = true,       // Usar APLL para mejor calidad
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
};

static i2s_pin_config_t i2s_pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
};

esp_err_t audio_output_init(void)
{
    esp_err_t ret;
    
    // Instalar el driver I2S
    ret = i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install I2S driver: %d", ret);
        return ret;
    }
    
    // Configurar los pines I2S
    ret = i2s_set_pin(I2S_NUM, &i2s_pin_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set I2S pin: %d", ret);
        return ret;
    }
    
    // Establecer el reloj I2S
    i2s_set_clk(I2S_NUM, I2S_SAMPLE_RATE, I2S_BITS_PER_SAMPLE, I2S_CHANNEL_STEREO);
    
    // Iniciar el I2S
    ret = i2s_start(I2S_NUM);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start I2S: %d", ret);
        return ret;
    }
    
    // Inicializar el DSP
    ret = audio_dsp_init(I2S_SAMPLE_RATE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize DSP: %d", ret);
        return ret;
    }
    
    // Configurar el DSP con valores por defecto
    audio_dsp_default_config(&dsp_config);
    
    // Valor de ganancia por defecto (aumentar volumen)
    dsp_config.gain_db = 6.0f;  // +6dB de ganancia
    
    // Inicializar buffer DSP
    dsp_buffer_size = 4096;  // Tamaño inicial, se redimensionará si es necesario
    dsp_buffer = malloc(dsp_buffer_size);
    if (dsp_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate DSP buffer");
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "I2S initialized successfully with DSP processing");
    ESP_LOGI(TAG, "PCM5102A DAC connected on pins - DOUT: %d, BCLK: %d, LRC: %d", 
              I2S_DOUT, I2S_BCLK, I2S_LRC);
    ESP_LOGI(TAG, "DSP enabled with gain: %.1f dB", dsp_config.gain_db);
              
    return ESP_OK;
}

esp_err_t audio_output_deinit(void)
{
    esp_err_t ret;
    
    // Detener I2S
    ret = i2s_stop(I2S_NUM);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop I2S: %d", ret);
        return ret;
    }
    
    // Desinstalar el driver
    ret = i2s_driver_uninstall(I2S_NUM);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to uninstall I2S driver: %d", ret);
        return ret;
    }
    
    // Deinicializar DSP
    audio_dsp_deinit();
    
    // Liberar buffer DSP
    if (dsp_buffer != NULL) {
        free(dsp_buffer);
        dsp_buffer = NULL;
    }
    
    ESP_LOGI(TAG, "I2S deinitialized successfully");
    return ESP_OK;
}

esp_err_t audio_output_write(uint8_t* data, size_t length)
{
    size_t bytes_written = 0;
    esp_err_t ret = ESP_OK;
    
    // Aplicar DSP si está habilitado
    if (dsp_enabled && data != NULL && length > 0) {
        // Asegurar que tenemos suficiente espacio en el buffer DSP
        if (length > dsp_buffer_size) {
            free(dsp_buffer);
            dsp_buffer_size = length;
            dsp_buffer = malloc(dsp_buffer_size);
            if (dsp_buffer == NULL) {
                ESP_LOGE(TAG, "Failed to resize DSP buffer");
                return ESP_ERR_NO_MEM;
            }
        }
        
        // Procesar audio con DSP
        ret = audio_dsp_process(data, dsp_buffer, length, &dsp_config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to process audio with DSP: %d", ret);
            return ret;
        }
        
        // Escribir datos procesados al I2S
        ret = i2s_write(I2S_NUM, dsp_buffer, length, &bytes_written, portMAX_DELAY);
    } else {
        // Bypass DSP y escribir directamente al I2S
        ret = i2s_write(I2S_NUM, data, length, &bytes_written, portMAX_DELAY);
    }
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write to I2S: %d", ret);
        return ret;
    }
    
    if (bytes_written != length) {
        ESP_LOGW(TAG, "Bytes written (%d) differs from length specified (%d)", bytes_written, length);
    }
    
    return ESP_OK;
}

esp_err_t audio_output_set_sample_rate(uint32_t sample_rate)
{
    esp_err_t ret = i2s_set_sample_rates(I2S_NUM, sample_rate);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set sample rate %d: %d", sample_rate, ret);
        return ret;
    }
    
    // Actualizar la frecuencia de muestreo en el DSP
    ret = audio_dsp_init(sample_rate);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to update DSP sample rate: %d", ret);
        return ret;
    }
    
    ESP_LOGI(TAG, "Sample rate set to %d Hz", sample_rate);
    return ESP_OK;
}

void audio_output_set_volume(uint8_t volume_percent)
{
    // Convertir porcentaje a dB (0-100% -> -40dB a +20dB)
    // 0% = -40dB (casi mudo), 50% = 0dB (ganancia unitaria), 100% = +20dB (ganancia máxima)
    if (volume_percent > 100) {
        volume_percent = 100;
    }
    
    float volume_db;
    if (volume_percent < 50) {
        // Escala logarítmica para volúmenes bajos
        volume_db = -40.0f + (volume_percent * 0.8f); // 0% -> -40dB, 50% -> 0dB
    } else {
        // Escala lineal para volúmenes altos
        volume_db = (volume_percent - 50) * 0.4f; // 50% -> 0dB, 100% -> +20dB
    }
    
    // Actualizar configuración DSP
    dsp_config.gain_db = volume_db;
    
    ESP_LOGI(TAG, "Volume set to %d%% (%.1f dB)", volume_percent, volume_db);
}

// Funciones nuevas para controlar el DSP

void audio_output_enable_dsp(bool enable)
{
    dsp_enabled = enable;
    ESP_LOGI(TAG, "DSP %s", enable ? "enabled" : "disabled");
}

void audio_output_set_eq(float bass_db, float mid_db, float treble_db)
{
    dsp_config.bass_gain_db = bass_db;
    dsp_config.mid_gain_db = mid_db;
    dsp_config.treble_gain_db = treble_db;
    
    ESP_LOGI(TAG, "EQ set - Bass: %.1f dB, Mid: %.1f dB, Treble: %.1f dB",
             bass_db, mid_db, treble_db);
}

void audio_output_set_channel_balance(float left_gain_db, float right_gain_db)
{
    dsp_config.separate_channels = true;
    dsp_config.left_gain_db = left_gain_db;
    dsp_config.right_gain_db = right_gain_db;
    
    ESP_LOGI(TAG, "Channel balance set - Left: %.1f dB, Right: %.1f dB",
             left_gain_db, right_gain_db);
}

void audio_output_reset_dsp(void)
{
    audio_dsp_default_config(&dsp_config);
    dsp_config.gain_db = 0.0f;  // Sin ganancia adicional
    dsp_config.separate_channels = false;
    
    ESP_LOGI(TAG, "DSP settings reset to defaults");
}