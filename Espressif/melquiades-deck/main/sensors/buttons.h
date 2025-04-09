#ifndef BUTTONS_H
#define BUTTONS_H

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../state.h"
#include "../bluetooth/spp_init.h"

// Puertos asignados a pulsadores/botones
#define BTN1 GPIO_NUM_27 // ADC17
#define BTN2 GPIO_NUM_25 // ADC18
#define BTN3 GPIO_NUM_32 // ADC04
#define BTN4 GPIO_NUM_4  // ADC10
#define BTN5 GPIO_NUM_0  // ADC11
#define BTN6 GPIO_NUM_2  // ADC12

void init_pulsadores();
void leer_pulsadores();

#endif // BUTTONS_H