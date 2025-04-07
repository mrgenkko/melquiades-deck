#include "uart_shell.h"

// Función para limpiar el buffer de entrada
void clear_stdin()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ; // Vacía los caracteres restantes en el buffer
}

//Funcion para manejar el shell usado via UART
void uart_shell_task(void *pvParameters)
{
    char input[20]; // Buffer para recibir el comando
    printf("\nIngrese comando:\n");
    while (1)
    {
        // Lee un string de hasta 19 caracteres
        if (scanf("%19s", input) == 1)
        {
            // Limpia el buffer después de la lectura
            clear_stdin(); 
            //Iniciamos las validaciones de los comandos recibidos
            if (strcmp(input, "start_led_board") == 0)
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
            else if (strcmp(input, "stop_led_board") == 0)
            {
                if (tasl_ledboard_handle != NULL)
                {
                    vTaskDelete(tasl_ledboard_handle);
                    tasl_ledboard_handle = NULL;
                    printf("Se pausa encendido de led.\n");
                }
            }
            //Seccion de sensores
            else if (strcmp(input, "start_sensors") == 0)
            {
                if(sensors_streaming_uart){
                    printf("Actualmente nos encontramos transmitiendo data via UART.\n");
                }else{
                    //Cambiamos estado global a true para no incurrir en recreacion de tareas
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
            else if (strcmp(input, "stop_sensors") == 0)
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
                }else if(sensors_streaming_uart && sensors_streaming_bt)
                {
                    sensors_streaming_uart = false;
                    printf("Se cierra streaming en UART, BT sigue transmitiendo.\n");
                } else
                {
                    printf("No nos encontramos realizando ningun tipo de streaming.\n");
                }                
            }    
            // Comandos adicionales
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
