#include "sine_wave.h"

#define TAG "SINE_EXAMPLE"
// Configuración de la onda senoidal
#define SAMPLE_RATE         44100
#define SINE_FREQ           1000        // 440 Hz = nota La
#define AMPLITUDE           32767       // Amplitud máxima para 16 bits
#define BUFFER_SIZE         512         // Tamaño del buffer en samples

void generate_sine_wave(int16_t* buffer, size_t buffer_size, float frequency, int sample_rate, float amplitude) {
    static float phase = 0.0f;
    float phase_increment = 2.0f * M_PI * frequency / sample_rate;
    
    for (int i = 0; i < buffer_size; i += 2) {
        // Calcular el valor senoidal actual
        float sample_value = sinf(phase) * amplitude;
        int16_t sample = (int16_t)sample_value;
        
        // Escribir el mismo valor en ambos canales (estéreo)
        buffer[i] = sample;        // Canal izquierdo
        buffer[i + 1] = sample;    // Canal derecho
        
        // Incrementar la fase para el siguiente sample
        phase += phase_increment;
        if (phase >= 2.0f * M_PI) {
            phase -= 2.0f * M_PI;  // Mantener la fase en [0, 2π)
        }
    }
}

void sine_wave_task(void *pvParameters) {
    esp_err_t ret;
    int16_t *buffer = malloc(BUFFER_SIZE * sizeof(int16_t));
    
    if (buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for buffer");
        vTaskDelete(NULL);
        return;
    }        
    
    // Inicializar la salida de audio
    ret = audio_output_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize audio output");
        free(buffer);
        vTaskDelete(NULL);
        return;
    }
    
    // Establecer la tasa de muestreo
    ret = audio_output_set_sample_rate(SAMPLE_RATE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set sample rate");
        audio_output_deinit();
        free(buffer);
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "Generando onda senoidal a %d Hz con amplitud máxima", SINE_FREQ);
    
    // Generar onda cuadrada para máximo volumen (alternativa)
    int testing_mode = 0; // 0 = senoidal, 1 = cuadrada
    
    // Bucle principal: generar y reproducir la onda senoidal
    while (1) {
        if (testing_mode == 0) {
            // Generar datos de onda senoidal
            generate_sine_wave(buffer, BUFFER_SIZE, SINE_FREQ, SAMPLE_RATE, AMPLITUDE);
        } else {
            // Generar onda cuadrada (más audible) para pruebas
            for (int i = 0; i < BUFFER_SIZE; i++) {
                buffer[i] = (i / 16) % 2 ? 32767 : -32768;
            }
        }
        
        // Enviar datos al DAC
        ret = audio_output_write((uint8_t*)buffer, BUFFER_SIZE * sizeof(int16_t));
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error writing audio data");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        
        // Alternar entre modos cada 10 segundos
        static int counter = 0;
        if (++counter >= 1000) {
            counter = 0;
            testing_mode = !testing_mode;
            ESP_LOGI(TAG, "Cambiando a modo %s", testing_mode ? "onda cuadrada" : "onda senoidal");
        }
        
        // Un pequeño delay para permitir que el sistema respire
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    // En teoria no deberia de llegar aca, por yolo liberamos todo para evitar colapsos
    audio_output_deinit();
    free(buffer);
    vTaskDelete(NULL);
}