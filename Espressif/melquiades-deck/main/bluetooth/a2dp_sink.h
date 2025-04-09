#ifndef A2DP_SINK_H
#define A2DP_SINK_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Presets de ecualizador disponibles
typedef enum {
    EQ_FLAT = 0,
    EQ_BASS_BOOST,
    EQ_MID_BOOST,
    EQ_TREBLE_BOOST,
    EQ_VOCAL,
    EQ_MAX_PRESETS
} eq_preset_t;

/**
 * @brief Inicializa el módulo A2DP Sink
 */
void init_a2dp_sink(void);

/**
 * @brief Activa o desactiva el procesamiento DSP
 * 
 * @param enabled true para activar, false para desactivar
 */
void set_dsp_enabled(bool enabled);

/**
 * @brief Configura un preset de ecualizador
 * 
 * @param preset Preset de EQ seleccionado
 */
void set_eq_preset(eq_preset_t preset);

/**
 * @brief Configura el volumen global
 * 
 * @param volume Volumen (0-100%)
 */
void set_volume(uint8_t volume);

/**
 * @brief Configura el balance entre canales
 * 
 * @param balance Balance (-1.0 = izquierda, 0.0 = centro, 1.0 = derecha)
 */
void set_balance(float balance);

/**
 * @brief Alterna entre DSP activado/desactivado
 */
void dsp_toggle_enabled(void);

/**
 * @brief Cambia al siguiente preset de ecualizador
 */
void dsp_next_eq_preset(void);

/**
 * @brief Aumenta el volumen en 5%
 */
void dsp_volume_up(void);

/**
 * @brief Disminuye el volumen en 5%
 */
void dsp_volume_down(void);

/**
 * @brief Desplaza el balance hacia la izquierda
 */
void dsp_balance_left(void);

/**
 * @brief Desplaza el balance hacia la derecha
 */
void dsp_balance_right(void);

/**
 * @brief Centra el balance
 */
void dsp_balance_center(void);

#endif /* A2DP_SINK_H */