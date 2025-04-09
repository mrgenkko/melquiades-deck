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
#include "audio/audio_output.h"
#include "audio/sine_wave.h"
#include "bluetooth/a2dp_sink.h"
#include "bluetooth/spp_init.h"
#include "leds/board.h"
#include "sensors/buttons.h"
#include "sensors/potentiometers.h"
#include "shell/uart_shell.h"


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
    //Inicializamos componentes de board y sensores
    init_potentiometers();
    init_pulsadores();
    init_led_board();    
    // Inicializar los perfiles Bluetooth
    init_a2dp_sink();  // Inicializar A2DP para audio
    init_bluetooth();  // Inicializar SPP para comandos

    // Crear tareas
    xTaskCreate(bt_shell_task, "bt_shell_task", 4096, NULL, 5, NULL);
    xTaskCreate(uart_shell_task, "uart_shell_task", 4096, NULL, 5, NULL);    

    //Creamos tarea para onda senoidal (prueba de psm5102)
    //xTaskCreate(sine_wave_task, "sine_wave_task", 4096, NULL, 5, NULL);
}
