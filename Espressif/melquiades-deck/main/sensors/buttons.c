#include "buttons.h"
//Bibliotecas de sistema
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//bibliotecas custom
#include "../state.h"
#include "../bluetooth/spp_init.h"

void init_pulsadores()
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BTN1) | (1ULL << BTN2) | (1ULL << BTN3) |
                        (1ULL << BTN4) | (1ULL << BTN5) | (1ULL << BTN6),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,     // Desactivar pull-up interno
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Sin pull-down interno (usamos externo)
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);
}

void leer_pulsadores()
{    
    char response[100];  // Reservas espacio suficiente
    while (1)
    {
        //construimos valores de pulsadores
        int estado1 = gpio_get_level(BTN1);
        int estado2 = gpio_get_level(BTN2);
        int estado3 = gpio_get_level(BTN3);
        int estado4 = gpio_get_level(BTN4);
        int estado5 = gpio_get_level(BTN5);
        int estado6 = gpio_get_level(BTN6);
        //construimos response        
        snprintf(response, sizeof(response), "Botones: %d, %d, %d, %d, %d, %d\n", estado1, estado2, estado3, estado4, estado5, estado6);
        //Validamos el shell activo        
        if(sensors_streaming_uart){            
            printf("%s", response);
        }else if(sensors_streaming_bt){
            send_bt_response(response);
        }                    
        // Espera 500ms entre lecturas
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
