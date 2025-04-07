
#ifndef _UART_SHELL_H_
#define _UART_SHELL_H_

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "../state.h"
#include "../leds/board.h"
#include "../sensors/potentiometers.h"
#include "../sensors/buttons.h"

void uart_shell_task(void *pvParameters);

#endif /* _SHELL_INIT_H_ */