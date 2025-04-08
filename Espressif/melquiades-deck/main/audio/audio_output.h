#ifndef AUDIO_OUTPUT_H
#define AUDIO_OUTPUT_H

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

/**
 * @brief Inicializa el sistema de audio I2S para el DAC PCM5102A
 * 
 * @return esp_err_t ESP_OK si la inicialización es exitosa
 */
esp_err_t audio_output_init(void);

/**
 * @brief Deinicializa el sistema de audio I2S
 * 
 * @return esp_err_t ESP_OK si la deinicialización es exitosa
 */
esp_err_t audio_output_deinit(void);

/**
 * @brief Escribe datos de audio al DAC PCM5102A a través de I2S
 * 
 * @param data Puntero a los datos de audio
 * @param length Longitud de los datos en bytes
 * @return esp_err_t ESP_OK si la escritura es exitosa
 */
esp_err_t audio_output_write(uint8_t* data, size_t length);

/**
 * @brief Establece la tasa de muestreo para la salida de audio
 * 
 * @param sample_rate Tasa de muestreo en Hz (por ejemplo, 44100, 48000)
 * @return esp_err_t ESP_OK si el cambio es exitoso
 */
esp_err_t audio_output_set_sample_rate(uint32_t sample_rate);

/**
 * @brief Establece el nivel de volumen (simulado, ya que el PCM5102A no tiene control de volumen por I2S)
 * 
 * @param volume Nivel de volumen (0-100)
 */
void audio_output_set_volume(uint8_t volume);

#endif // AUDIO_OUTPUT_H