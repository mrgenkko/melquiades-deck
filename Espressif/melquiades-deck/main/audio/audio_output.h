#ifndef AUDIO_OUTPUT_H
#define AUDIO_OUTPUT_H

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"



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
 * @brief Configura el volumen general
 * 
 * @param volume_percent Volumen como porcentaje (0-100)
 */
void audio_output_set_volume(uint8_t volume_percent);

/**
 * @brief Activa o desactiva el procesamiento DSP
 * 
 * @param enable true para activar, false para desactivar
 */
void audio_output_enable_dsp(bool enable);

/**
 * @brief Configura el ecualizador de 3 bandas
 * 
 * @param bass_db Ganancia de bajos en dB (-20 a +20)
 * @param mid_db Ganancia de medios en dB (-20 a +20)
 * @param treble_db Ganancia de agudos en dB (-20 a +20)
 */
void audio_output_set_eq(float bass_db, float mid_db, float treble_db);

/**
 * @brief Configura el balance entre canales izquierdo y derecho
 * 
 * @param left_gain_db Ganancia canal izquierdo en dB (-20 a +20)
 * @param right_gain_db Ganancia canal derecho en dB (-20 a +20)
 */
void audio_output_set_channel_balance(float left_gain_db, float right_gain_db);

/**
 * @brief Restablece la configuración DSP a valores por defecto
 */
void audio_output_reset_dsp(void);

#endif // AUDIO_OUTPUT_H