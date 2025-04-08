#ifndef SINE_WAVE_H
#define SINE_WAVE_H

//bibliotecas del sistema
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//bibliotecas custom
#include "audio_output.h"

/**
 * @brief Tarea para probar el PCM5102 con una onda senoidal, si todo esta ok deberiamos escuchar 2 frecuencias cada 10 segundos
 * 
 * @param pvParameters esto es estandar cuando es una funcion tipo task
 */
void sine_wave_task(void *pvParameters);

#endif // SINE_WAVE_H