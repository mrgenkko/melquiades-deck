#include "common_shell.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "../state.h"
#include "../leds/board.h"
#include "../sensors/buttons.h"
#include "../sensors/potentiometers.h"
#include "../bluetooth/a2dp_sink.h"

void handle_command(const char *input, char *output, size_t size, const char *origen){    
    /*****COMANDOS PARA LED*****/
    if (strcmp(input, "led_board start") == 0)
    {
        if (tasl_ledboard_handle == NULL)
        {
            xTaskCreate(manage_led_board, "manage_led_board", 2048, NULL, 5, &tasl_ledboard_handle);
            snprintf(output, size, "Encendido en el led de la board ON.\n");            
        }
        else
        {
            snprintf(output, size, "La tarea de encender led ya está en ejecución.\n");            
        }
    }
    else if (strcmp(input, "led_board stop") == 0)
    {
        if (tasl_ledboard_handle != NULL)
        {
            vTaskDelete(tasl_ledboard_handle);
            tasl_ledboard_handle = NULL;
            snprintf(output, size, "Se pausa encendido de led.\n");            
        }
    }
    /*****COMANDOS PARA SENSORES*****/
    else if (strcmp(input, "sensors start") == 0)
    {        
        bool crearTask = false;
        if (strcmp(origen, "UART") == 0){            
            if (sensors_streaming_uart)
            {                
                snprintf(output, size, "Actualmente nos encontramos transmitiendo data via UART.\n");
            }
            else
            {                
                sensors_streaming_uart = true;
                crearTask = true;
                snprintf(output, size, "Iniciamos transmision UART.\n");
            }
        }
        if (strcmp(origen, "BT") == 0){                 
            if (sensors_streaming_bt)
            {                
                snprintf(output, size, "Actualmente nos encontramos transmitiendo data via BT.\n");
            }
            else
            {
                sensors_streaming_bt = true;
                crearTask = true;
                snprintf(output, size, "Iniciamos transmision BT.\n");
            }
        }

        if(crearTask){
            if (task_pot_handle == NULL)
            {
                xTaskCreate(read_potentiometers, "read_potentiometers", 2048, NULL, 5, &task_pot_handle);
            }
            if (task_btn_handle == NULL)
            {
                xTaskCreate(leer_pulsadores, "leer_pulsadores", 2048, NULL, 5, &task_btn_handle);
            }            
        }        
    }
    else if (strcmp(input, "sensors stop") == 0)
    {
        if (strcmp(origen, "BT") == 0){        
            if (sensors_streaming_bt && !sensors_streaming_uart)
            {
                sensors_streaming_bt = false;
                if (task_pot_handle != NULL)
                {
                    vTaskDelete(task_pot_handle);
                    task_pot_handle = NULL;
                }
                if (task_btn_handle != NULL)
                {
                    vTaskDelete(task_btn_handle);
                    task_btn_handle = NULL;
                }
                snprintf(output, size, "Se cierra streaming en BT.\n");
            }
            else if (sensors_streaming_bt && sensors_streaming_uart)
            {
                sensors_streaming_bt = false;            
                snprintf(output, size, "Se cierra streaming en BT, UART sigue transmitiendo.\n");
            }
            else
            {            
                snprintf(output, size, "No nos encontramos realizando ningun tipo de streaming.\n");
            }
        }
        if (strcmp(origen, "UART") == 0){        
            if (sensors_streaming_uart && !sensors_streaming_bt)
            {
                sensors_streaming_uart = false;
                if (task_pot_handle != NULL)
                {
                    vTaskDelete(task_pot_handle);
                    task_pot_handle = NULL;
                }
                if (task_btn_handle != NULL)
                {
                    vTaskDelete(task_btn_handle);
                    task_btn_handle = NULL;
                }
                snprintf(output, size, "Se cierra streaming en UART.\n");
            }
            else if (sensors_streaming_uart && sensors_streaming_bt)
            {
                sensors_streaming_uart = false;            
                snprintf(output, size, "Se cierra streaming en UART, BT sigue transmitiendo.\n");
            }
            else
            {            
                snprintf(output, size, "No nos encontramos realizando ningun tipo de streaming.\n");
            }
        }        
    }
    /*****COMANDOS PARA VOLUMEN*****/
    else if (strncmp(input, "set_volume ", 11) == 0)
    {
        const char *param = input + 11;
        if (isdigit((unsigned char)param[0]))
        {
            int volumen = atoi(param);
            set_volume(volumen);            
            snprintf(output, size, "Se cambia volumen a %d.\n", volumen);
        }
        else
        {
            snprintf(output, size, "Error: volumen inválido.\n");            
        }
    }
    /*****COMANDOS PARA ECUALIZADOR*****/
    else if (strncmp(input, "eq ", 3) == 0)
    {
        const char *param = input + 3;
        if (strcmp(param, "flat") == 0)
        {
            set_eq_preset(EQ_FLAT);            
            snprintf(output, size, "Se cambia ecualizacion a EQ_FLAT.\n");
        }
        else if (strcmp(param, "bass_boost") == 0)
        {
            set_eq_preset(EQ_BASS_BOOST);            
            snprintf(output, size, "Se cambia ecualizacion a EQ_BASS_BOOST.\n");
        }
        else if (strcmp(param, "mid_boost") == 0)
        {
            set_eq_preset(EQ_MID_BOOST);            
            snprintf(output, size, "Se cambia ecualizacion a EQ_MID_BOOST.\n");
        }
        else if (strcmp(param, "treble_boost") == 0)
        {
            set_eq_preset(EQ_TREBLE_BOOST);
            snprintf(output, size, "Se cambia ecualizacion a EQ_TREBLE_BOOST.\n");            
        }
        else if (strcmp(param, "vocal") == 0)
        {
            set_eq_preset(EQ_VOCAL);
            snprintf(output, size, "Se cambia ecualizacion a EQ_VOCAL.\n");
        }
    }
    /*****COMANDOS PARA BALANCE*****/
    else if (strncmp(input, "headphone_balance ", 18) == 0)
    {
        const char *param = input + 18;
        // Saltar espacios iniciales
        while (*param == ' ') param++;
        if (isdigit((unsigned char)param[0]) || param[0] == '-' || param[0] == '.')
        {
            float value = strtof(param, NULL);
            set_balance(value);
            snprintf(output, size, "Se cambia balance a %.2f.\n", value);            
        }
        else
        {            
            snprintf(output, size, "Error: balance inválido.\n");
        }     
    }
    /*****COMANDOS PARA DSP*****/
    else if (strcmp(input, "dsp enabled") == 0)
    {
        set_dsp_enabled(true);
        snprintf(output, size, "Se activa DSP.\n");        
    }
    else if (strcmp(input, "dsp disabled") == 0)
    {
        set_dsp_enabled(false);
        snprintf(output, size, "Se desactiva DSP.\n");
    }
    /*****OTROS*****/
    else if (strcmp(input, "help") == 0)
    {
        snprintf(output, size, cmd_commands);        
    }
    else
    {
        snprintf(output, size, "Comando no válido. Escriba help para listado de comandos validos.\n");        
    }
}
