#include "uart_shell.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "../state.h"
#include "../leds/board.h"
#include "../sensors/potentiometers.h"
#include "../sensors/buttons.h"
#include "../bluetooth/a2dp_sink.h"

// Funcion para manejar el shell usado via UART
void uart_shell_task(void *pvParameters)
{
    char input[20]; // Buffer para recibir el comando
    printf("\nIngrese comando:\n");
    while (1)
    {
        if (fgets(input, sizeof(input), stdin) != NULL)
        {
            // Eliminar salto de línea
            input[strcspn(input, "\n")] = 0;
            /*****COMANDOS PARA LED*****/
            if (strcmp(input, "led_board start") == 0)
            {
                if (tasl_ledboard_handle == NULL)
                {
                    xTaskCreate(manage_led_board, "manage_led_board", 2048, NULL, 5, &tasl_ledboard_handle);
                    printf("Encendido en el led de la board ON.\n");
                }
                else
                {
                    printf("La tarea de encender led ya está en ejecución.\n");
                }
            }
            else if (strcmp(input, "led_board stop") == 0)
            {
                if (tasl_ledboard_handle != NULL)
                {
                    vTaskDelete(tasl_ledboard_handle);
                    tasl_ledboard_handle = NULL;
                    printf("Se pausa encendido de led.\n");
                }
            }
            /*****COMANDOS PARA SENSORES*****/
            else if (strcmp(input, "sensors start") == 0)
            {
                if (sensors_streaming_uart)
                {
                    printf("Actualmente nos encontramos transmitiendo data via UART.\n");
                }
                else
                {
                    // Cambiamos estado global a true para no incurrir en recreacion de tareas
                    sensors_streaming_uart = true;
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
                }
                else if (sensors_streaming_uart && sensors_streaming_bt)
                {
                    sensors_streaming_uart = false;
                    printf("Se cierra streaming en UART, BT sigue transmitiendo.\n");
                }
                else
                {
                    printf("No nos encontramos realizando ningun tipo de streaming.\n");
                }
            }
            /*****COMANDOS PARA VOLUMEN*****/
            else if (strncmp(input, "set_volume ", 11) == 0)
            {
                char *param = input + 11;
                if (isdigit((unsigned char)param[0]))
                {
                    int volumen = atoi(param);
                    set_volume(volumen);
                    printf("Se cambia volumen a %d.\n", volumen);
                }
                else
                {
                    printf("Error: volumen inválido.\n");
                }
            }
            /*****COMANDOS PARA ECUALIZADOR*****/
            else if (strncmp(input, "eq ", 3) == 0)
            {
                char *param = input + 3;
                if (strcmp(param, "flat") == 0)
                {
                    set_eq_preset(EQ_FLAT);
                    printf("Se cambia ecualizacion a EQ_FLAT.\n");
                }
                else if (strcmp(param, "bass_boost") == 0)
                {
                    set_eq_preset(EQ_BASS_BOOST);
                    printf("Se cambia ecualizacion a EQ_BASS_BOOST.\n");
                }
                else if (strcmp(param, "mid_boost") == 0)
                {
                    set_eq_preset(EQ_MID_BOOST);
                    printf("Se cambia ecualizacion a EQ_MID_BOOST.\n");
                }
                else if (strcmp(param, "treble_boost") == 0)
                {
                    set_eq_preset(EQ_TREBLE_BOOST);
                    printf("Se cambia ecualizacion a EQ_TREBLE_BOOST.\n");
                }
                else if (strcmp(param, "vocal") == 0)
                {
                    set_eq_preset(EQ_VOCAL);
                    printf("Se cambia ecualizacion a EQ_VOCAL.\n");
                }
            }
            /*****COMANDOS PARA BALANCE*****/
            else if (strncmp(input, "headphone_balance ", 18) == 0)
            {
                char *param = input + 18;
                // Saltar espacios iniciales
                while (*param == ' ') param++;
                if (isdigit((unsigned char)param[0]) || param[0] == '-' || param[0] == '.')
                {
                    float value = strtof(param, NULL);
                    set_balance(value);
                    printf("Se cambia balance a %.2f\n", value);
                }
                else
                {
                    printf("Error: balance inválido.\n");
                }     
            }
            /*****COMANDOS PARA DSP*****/
            else if (strcmp(input, "dsp enabled") == 0)
            {
                set_dsp_enabled(true);
                printf("Se activa DSP.\n");
            }
            else if (strcmp(input, "dsp disabled") == 0)
            {
                set_dsp_enabled(false);
                printf("Se desactiva DSP.\n");
            }
            /*****OTROS*****/
            else if (strcmp(input, "help") == 0)
            {
                printf(cmd_commands);
            }
            else
            {
                printf("Comando no válido. Escriba help para listado de comandos validos:\n");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500)); // Delay para que el sistema no se emperique
    }
}
