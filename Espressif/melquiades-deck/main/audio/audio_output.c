#include "audio_output.h"
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
    
    // Establecer el volumen al máximo - Solo para propósitos de prueba
    // (Esto no afecta al PCM5102A directamente, pero asegura que el I2S envíe señales a máxima amplitud)
    i2s_set_clk(I2S_NUM, I2S_SAMPLE_RATE, I2S_BITS_PER_SAMPLE, I2S_CHANNEL_STEREO);
    
    // Iniciar el I2S
    ret = i2s_start(I2S_NUM);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start I2S: %d", ret);
        return ret;
    }
    
    ESP_LOGI(TAG, "I2S initialized successfully - AMPLITUD MÁXIMA");
    ESP_LOGI(TAG, "PCM5102A DAC connected on pins - DOUT: %d, BCLK: %d, LRC: %d", 
              I2S_DOUT, I2S_BCLK, I2S_LRC);
    ESP_LOGI(TAG, "Asegúrate que el pin XSMT del PCM5102A está en HIGH (conectado a 3.3V o flotante)");
              
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
    
    ESP_LOGI(TAG, "I2S deinitialized successfully");
    return ESP_OK;
}

esp_err_t audio_output_write(uint8_t* data, size_t length)
{
    size_t bytes_written = 0;
    esp_err_t ret = i2s_write(I2S_NUM, data, length, &bytes_written, portMAX_DELAY);
    
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
    
    ESP_LOGI(TAG, "Sample rate set to %d Hz", sample_rate);
    return ESP_OK;
}

void audio_output_set_volume(uint8_t volume)
{
    ESP_LOGI(TAG, "Volume control requested: %d%%. Note: PCM5102A does not support direct volume control via I2S.", volume);
    // El PCM5102A no tiene control de volumen vía I2S
}