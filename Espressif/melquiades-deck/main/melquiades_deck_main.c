// Bibliotecas sistema
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"

// Bibliotecas custom
#include "state.h"
#include "bluetooth/init.h"
#include "leds/board.h"
#include "sensors/buttons.h"
#include "sensors/potentiometers.h"
#include "shell/shell_init.h"
#include "shell/uart_shell.h"
#include "shell/bt_shell.h"

// Funcion ppal de la app
void app_main()
{
    //Habilitamos un log mas amplio en la consola del esp32
    esp_log_level_set("*", ESP_LOG_VERBOSE);
    // Inicializar NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    //Inicializamos componentes necesarios
    init_potentiometers();
    init_pulsadores();
    init_led_board();
    init_bluetooth();
    //Creamos tareas para las shell de UART y BT
    xTaskCreate(bt_shell_task, "bt_shell_task", 4096, NULL, 5, NULL);
    xTaskCreate(uart_shell_task, "uart_shell_task", 4096, NULL, 5, NULL);        
    // Inicializar el shell
    //shell_init();
}
