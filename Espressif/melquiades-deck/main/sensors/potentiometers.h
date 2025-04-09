#ifndef POTENTIOMETERS_H
#define POTENTIOMETERS_H

#include "driver/adc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../state.h"
#include "../bluetooth/spp_init.h"

void init_potentiometers();
void read_potentiometers();

#endif // POTENTIOMETERS_H