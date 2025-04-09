// audio_dsp.c
#include "audio_dsp.h"
#include <math.h>
#include <string.h>
#include "esp_log.h"

#define TAG "AUDIO_DSP"

// Constantes para conversión de dB a escala lineal
#define DB_TO_LINEAR(x) powf(10.0f, (x) / 20.0f)
#define MAX_GAIN_DB 20.0f        // Ganancia máxima permitida en dB
#define MIN_GAIN_DB -20.0f       // Atenuación máxima permitida en dB

// Coeficientes para filtros IIR simplificados (biquad)
typedef struct {
    float b0, b1, b2;  // Numerador
    float a1, a2;      // Denominador (a0 siempre es 1)
    float x1, x2;      // Valores previos de entrada
    float y1, y2;      // Valores previos de salida
} biquad_filter_t;

// Estado para el filtro de bajos
static biquad_filter_t bass_filter_left;
static biquad_filter_t bass_filter_right;

// Estado para el filtro de medios
static biquad_filter_t mid_filter_left;
static biquad_filter_t mid_filter_right;

// Estado para el filtro de agudos
static biquad_filter_t treble_filter_left;
static biquad_filter_t treble_filter_right;

// Frecuencia de muestreo actual
static uint32_t current_sample_rate = 44100;

// Buffer temporal para procesar audio
static uint8_t* temp_buffer = NULL;
static size_t temp_buffer_size = 0;

/**
 * @brief Diseña un filtro biquad para un tipo específico
 * 
 * @param filter Puntero al filtro a configurar
 * @param type Tipo de filtro (0=lowpass, 1=bandpass, 2=highpass)
 * @param freq_hz Frecuencia central/corte en Hz
 * @param q Factor Q
 * @param gain_db Ganancia en dB
 * @param sample_rate Frecuencia de muestreo
 */
static void design_biquad(biquad_filter_t* filter, int type, float freq_hz, float q, float gain_db, uint32_t sample_rate) {
    float omega = 2.0f * M_PI * freq_hz / sample_rate;
    float sn = sinf(omega);
    float cs = cosf(omega);
    float alpha = sn / (2.0f * q);
    float A = powf(10.0f, gain_db / 40.0f);
    
    float b0, b1, b2, a0, a1, a2;
    
    if (type == 0) {  // Low-pass
        b0 = (1.0f - cs) / 2.0f;
        b1 = 1.0f - cs;
        b2 = (1.0f - cs) / 2.0f;
        a0 = 1.0f + alpha;
        a1 = -2.0f * cs;
        a2 = 1.0f - alpha;
    } else if (type == 1) {  // Band-pass
        b0 = alpha * A;
        b1 = 0;
        b2 = -alpha * A;
        a0 = 1.0f + alpha;
        a1 = -2.0f * cs;
        a2 = 1.0f - alpha;
    } else {  // High-pass
        b0 = (1.0f + cs) / 2.0f;
        b1 = -(1.0f + cs);
        b2 = (1.0f + cs) / 2.0f;
        a0 = 1.0f + alpha;
        a1 = -2.0f * cs;
        a2 = 1.0f - alpha;
    }
    
    // Normalizar coeficientes
    filter->b0 = b0 / a0;
    filter->b1 = b1 / a0;
    filter->b2 = b2 / a0;
    filter->a1 = a1 / a0;
    filter->a2 = a2 / a0;
    
    // Reiniciar historial
    filter->x1 = 0;
    filter->x2 = 0;
    filter->y1 = 0;
    filter->y2 = 0;
}

/**
 * @brief Aplica un filtro biquad a una muestra
 * 
 * @param filter Filtro a aplicar
 * @param input Valor de entrada
 * @return float Valor de salida
 */
static float apply_biquad(biquad_filter_t* filter, float input) {
    // y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
    float output = filter->b0 * input + 
                  filter->b1 * filter->x1 + 
                  filter->b2 * filter->x2 - 
                  filter->a1 * filter->y1 - 
                  filter->a2 * filter->y2;
    
    // Actualizar historial
    filter->x2 = filter->x1;
    filter->x1 = input;
    filter->y2 = filter->y1;
    filter->y1 = output;
    
    return output;
}

/**
 * @brief Configura los filtros en base a la frecuencia de muestreo actual
 */
static void configure_filters(void) {
    // Filtros para bajos (Low-pass con corte en 250Hz)
    design_biquad(&bass_filter_left, 0, 250.0f, 0.707f, 0.0f, current_sample_rate);
    design_biquad(&bass_filter_right, 0, 250.0f, 0.707f, 0.0f, current_sample_rate);
    
    // Filtros para medios (Band-pass entre 250Hz y 4kHz)
    design_biquad(&mid_filter_left, 1, 1000.0f, 1.0f, 0.0f, current_sample_rate);
    design_biquad(&mid_filter_right, 1, 1000.0f, 1.0f, 0.0f, current_sample_rate);
    
    // Filtros para agudos (High-pass con corte en 4kHz)
    design_biquad(&treble_filter_left, 2, 4000.0f, 0.707f, 0.0f, current_sample_rate);
    design_biquad(&treble_filter_right, 2, 4000.0f, 0.707f, 0.0f, current_sample_rate);
}

esp_err_t audio_dsp_init(uint32_t sample_rate) {
    ESP_LOGI(TAG, "Inicializando módulo DSP con frecuencia de muestreo: %d Hz", sample_rate);
    
    current_sample_rate = sample_rate;
    configure_filters();
    
    // Inicializar buffer temporal
    temp_buffer_size = 2048;  // Tamaño arbitrario, ajustar según necesidad
    temp_buffer = malloc(temp_buffer_size);
    if (temp_buffer == NULL) {
        ESP_LOGE(TAG, "No se pudo asignar memoria para el buffer temporal DSP");
        return ESP_ERR_NO_MEM;
    }
    
    return ESP_OK;
}

void audio_dsp_default_config(dsp_config_t* config) {
    if (config != NULL) {
        config->gain_db = 0.0f;         // Sin ganancia adicional
        config->bass_gain_db = 0.0f;    // Sin enfatizar bajos
        config->mid_gain_db = 0.0f;     // Sin enfatizar medios
        config->treble_gain_db = 0.0f;  // Sin enfatizar agudos
        config->separate_channels = false;
        config->left_gain_db = 0.0f;
        config->right_gain_db = 0.0f;
    }
}

esp_err_t audio_dsp_process(const uint8_t* input_buffer, uint8_t* output_buffer, size_t length, const dsp_config_t* config) {
    if (input_buffer == NULL || output_buffer == NULL || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Asegurar que tenemos suficiente espacio en el buffer temporal
    if (length > temp_buffer_size) {
        free(temp_buffer);
        temp_buffer_size = length;
        temp_buffer = malloc(temp_buffer_size);
        if (temp_buffer == NULL) {
            ESP_LOGE(TAG, "No se pudo redimensionar el buffer temporal DSP");
            return ESP_ERR_NO_MEM;
        }
    }
    
    // Copiamos el buffer de entrada primero
    memcpy(output_buffer, input_buffer, length);
    
    // Las muestras de 16 bits están en little-endian
    int16_t* samples = (int16_t*)output_buffer;
    int num_samples = length / 2;  // Cada muestra es de 2 bytes
    
    // Aplicar ganancia general (convertir de dB a lineal)
    float gain = DB_TO_LINEAR(fminf(fmaxf(config->gain_db, MIN_GAIN_DB), MAX_GAIN_DB));
    
    // Ganancias para cada banda
    float bass_gain = DB_TO_LINEAR(fminf(fmaxf(config->bass_gain_db, MIN_GAIN_DB), MAX_GAIN_DB));
    float mid_gain = DB_TO_LINEAR(fminf(fmaxf(config->mid_gain_db, MIN_GAIN_DB), MAX_GAIN_DB));
    float treble_gain = DB_TO_LINEAR(fminf(fmaxf(config->treble_gain_db, MIN_GAIN_DB), MAX_GAIN_DB));
    
    // Ganancias específicas para canales
    float left_gain = 1.0f;
    float right_gain = 1.0f;
    
    if (config->separate_channels) {
        left_gain = DB_TO_LINEAR(fminf(fmaxf(config->left_gain_db, MIN_GAIN_DB), MAX_GAIN_DB));
        right_gain = DB_TO_LINEAR(fminf(fmaxf(config->right_gain_db, MIN_GAIN_DB), MAX_GAIN_DB));
    }
    
    // Procesamiento de cada muestra
    for (int i = 0; i < num_samples; i += 2) {
        // Procesar canal izquierdo (índice par)
        if (i < num_samples) {
            // Convertir de int16 a float para procesamiento
            float sample_left = (float)samples[i] / 32768.0f;
            
            // Aplicar ecualizador de 3 bandas
            float bass_out = apply_biquad(&bass_filter_left, sample_left) * bass_gain;
            float mid_out = apply_biquad(&mid_filter_left, sample_left) * mid_gain;
            float treble_out = apply_biquad(&treble_filter_left, sample_left) * treble_gain;
            
            // Sumar las tres bandas
            float out_left = (bass_out + mid_out + treble_out) * gain * left_gain;
            
            // Limitar a [-1.0, 1.0]
            if (out_left > 1.0f) out_left = 1.0f;
            if (out_left < -1.0f) out_left = -1.0f;
            
            // Convertir de vuelta a int16
            samples[i] = (int16_t)(out_left * 32767.0f);
        }
        
        // Procesar canal derecho (índice impar)
        if (i + 1 < num_samples) {
            // Convertir de int16 a float para procesamiento
            float sample_right = (float)samples[i + 1] / 32768.0f;
            
            // Aplicar ecualizador de 3 bandas
            float bass_out = apply_biquad(&bass_filter_right, sample_right) * bass_gain;
            float mid_out = apply_biquad(&mid_filter_right, sample_right) * mid_gain;
            float treble_out = apply_biquad(&treble_filter_right, sample_right) * treble_gain;
            
            // Sumar las tres bandas
            float out_right = (bass_out + mid_out + treble_out) * gain * right_gain;
            
            // Limitar a [-1.0, 1.0]
            if (out_right > 1.0f) out_right = 1.0f;
            if (out_right < -1.0f) out_right = -1.0f;
            
            // Convertir de vuelta a int16
            samples[i + 1] = (int16_t)(out_right * 32767.0f);
        }
    }
    
    return ESP_OK;
}

esp_err_t audio_dsp_deinit(void) {
    if (temp_buffer != NULL) {
        free(temp_buffer);
        temp_buffer = NULL;
        temp_buffer_size = 0;
    }
    
    return ESP_OK;
}