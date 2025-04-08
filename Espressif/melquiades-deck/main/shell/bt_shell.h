
#ifndef _BT_SHELL_H_
#define _BT_SHELL_H_

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "../state.h"
#include "../leds/board.h"
#include "../sensors/potentiometers.h"
#include "../sensors/buttons.h"

void bt_shell_task(void *pvParameters);

#endif /* _BT_SHELL_H_ */