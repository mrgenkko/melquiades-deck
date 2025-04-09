#include "potentiometers.h"
//bibliotecas de sistema
#include "driver/adc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//bibliotecas custom
#include "../state.h"
#include "../bluetooth/spp_init.h"
// Puertos asignados a los potenciometros
#define ADC_POT_1 ADC1_CHANNEL_0 // GPIO36 (ADC00)
#define ADC_POT_2 ADC2_CHANNEL_9 // GPIO26 (ADC19)
#define ADC_POT_3 ADC1_CHANNEL_3 // GPIO39 (ADC03)
#define ADC_POT_4 ADC1_CHANNEL_7 // GPIO35 (ADC07)
#define ADC_POT_5 ADC1_CHANNEL_5 // GPIO33 (ADC05)
#define ADC_POT_6 ADC1_CHANNEL_6 // GPIO34 (ADC10)

void init_potentiometers(){
    // Configurar ADC1
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC_POT_1, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC_POT_3, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC_POT_4, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC_POT_5, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC_POT_6, ADC_ATTEN_DB_11);
    // Configurar ADC2
    adc2_config_channel_atten(ADC_POT_2, ADC_ATTEN_DB_11);
}

void read_potentiometers(){
    char response[100];  // Reservas espacio suficiente
    while (1)
    {
        int val1 = adc1_get_raw(ADC_POT_1);
        int val3 = adc1_get_raw(ADC_POT_3);
        int val4 = adc1_get_raw(ADC_POT_4);
        int val5 = adc1_get_raw(ADC_POT_5);
        int val6 = adc1_get_raw(ADC_POT_6);

        // Como GPIO26 está en ADC2, hay que leerlo con adc2_get_raw
        int val2;
        adc2_get_raw(ADC_POT_2, ADC_WIDTH_BIT_12, &val2);
        //construimos response        
        snprintf(response, sizeof(response), "Potenciómetros: %d, %d, %d, %d, %d, %d\r\n", val1, val2, val3, val4, val5, val6);
        //Validamos el shell activo        
        if(sensors_streaming_uart){
            printf("%s", response);
        }else if(sensors_streaming_bt){
            send_bt_response(response);
        }        
        //Delay para no sobrecargar
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

