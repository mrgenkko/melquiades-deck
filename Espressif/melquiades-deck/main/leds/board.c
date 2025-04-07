#include "board.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Led de la board
#define LED_GPIO 19

void init_led_board(){
    gpio_pad_select_gpio(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
}

void manage_led_board(){
    while (1)
    {
        gpio_set_level(LED_GPIO, 1); // Encender LED
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_level(LED_GPIO, 0); // Apagar LED
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
