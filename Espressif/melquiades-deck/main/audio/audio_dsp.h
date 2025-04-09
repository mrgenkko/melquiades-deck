// audio_dsp.h
#ifndef AUDIO_DSP_H
#define AUDIO_DSP_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "esp_err.h"

// Estructura para configuración del ecualizador
typedef struct {
    float gain_db;            // Ganancia general en dB
    float bass_gain_db;       // Ganancia de bajos (< 250Hz) en dB
    float mid_gain_db;        // Ganancia de medios (250Hz-4kHz) en dB
    float treble_gain_db;     // Ganancia de agudos (> 4kHz) en dB
    bool separate_channels;   // Procesar canales independientemente
    float left_gain_db;       // Ganancia específica canal izquierdo
    float right_gain_db;      // Ganancia específica canal derecho
} dsp_config_t;

/**
 * @brief Inicializa el módulo DSP
 * 
 * @param sample_rate Frecuencia de muestreo del audio
 * @return esp_err_t ESP_OK si todo va bien
 */
esp_err_t audio_dsp_init(uint32_t sample_rate);

/**
 * @brief Aplica procesamiento DSP a los datos de audio
 * 
 * @param input_buffer Buffer con los datos de entrada
 * @param output_buffer Buffer para los datos procesados
 * @param length Longitud en bytes
 * @param config Configuración del DSP
 * @return esp_err_t ESP_OK si todo va bien
 */
esp_err_t audio_dsp_process(const uint8_t* input_buffer, uint8_t* output_buffer, size_t length, const dsp_config_t* config);

/**
 * @brief Configura el DSP con valores por defecto
 * 
 * @param config Puntero a la estructura de configuración
 */
void audio_dsp_default_config(dsp_config_t* config);

/**
 * @brief Libera recursos del DSP
 * 
 * @return esp_err_t ESP_OK si todo va bien
 */
esp_err_t audio_dsp_deinit(void);

#endif // AUDIO_DSP_H