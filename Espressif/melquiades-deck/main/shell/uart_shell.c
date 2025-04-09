#include "uart_shell.h"
//Bibliotecas de sistema
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//bibliotecas custom
#include "common_shell.h"
#include "../state.h"
#include "../leds/board.h"
#include "../sensors/potentiometers.h"
#include "../sensors/buttons.h"
#include "../bluetooth/a2dp_sink.h"

// Funcion para manejar el shell usado via UART
void uart_shell_task(void *pvParameters)
{
    char response[1024];
    char input[20]; // Buffer para recibir el comando
    printf("\nIngrese comando:\n");
    while (1)
    {
        if (fgets(input, sizeof(input), stdin) != NULL)
        {
            // Eliminar salto de línea
            input[strcspn(input, "\n")] = 0;
            
            handle_command(input, response, sizeof(response), "UART");
            printf(response);
        }
        vTaskDelay(pdMS_TO_TICKS(500)); // Delay para que el sistema no se emperique
    }
}
