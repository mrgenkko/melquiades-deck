#ifndef STATE_H
#define STATE_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>

// Estados booleanos
extern bool sensors_streaming_bt;
extern bool sensors_streaming_uart;

// TaskHandles globales
extern TaskHandle_t tasl_ledboard_handle;
extern TaskHandle_t task_pot_handle;
extern TaskHandle_t task_btn_handle;

//chars
extern const char cmd_commands[];

#endif // STATE_H
