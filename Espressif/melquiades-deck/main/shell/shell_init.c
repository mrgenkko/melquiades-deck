/**
 * @file shell/shell_init.c
 * @brief Implementación de una consola por UART usando esp_console
 */

#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "shell_init.h"

#define PROMPT_STR "Melquiades-Deck> "
#define MAX_HISTORY_SIZE 20
#define MAX_COMMAND_LINE_SIZE 256

static const char *TAG = "shell";

// Comando de ejemplo: test
static int cmd_test(int argc, char **argv)
{
    printf("Comando de prueba ejecutado\n");
    return 0;
}

// Comando de ejemplo: echo
static struct
{
    struct arg_str *message;
    struct arg_end *end;
} echo_args;

static int cmd_echo(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&echo_args);
    if (nerrors != 0)
    {
        arg_print_errors(stderr, echo_args.end, argv[0]);
        return 1;
    }

    if (echo_args.message->count > 0)
    {
        printf("%s\n", echo_args.message->sval[0]);
    }
    return 0;
}

// Comando de reset
static int cmd_reset(int argc, char **argv)
{
    printf("Reiniciando el sistema...\n");
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
    return 0;
}

// Comando para mostrar memoria libre
static int cmd_free(int argc, char **argv)
{
    printf("Memoria libre: %d bytes\n", esp_get_free_heap_size());
    return 0;
}

// Comando help personalizado
static int cmd_help(int argc, char **argv)
{
    const char *help_str = "Comandos disponibles:\n"
                           "  help   - Muestra este mensaje de ayuda\n"
                           "  test   - Comando de prueba simple\n"
                           "  echo   - Repite el mensaje ingresado\n"
                           "  reset  - Reinicia el sistema\n"
                           "  free   - Muestra la memoria libre disponible\n";
    printf("%s\n", help_str);
    return 0;
}

// Registro de comandos disponibles
static void register_commands(void)
{
    // Comando help
    const esp_console_cmd_t cmd_help = {
        .command = "help",
        .help = "Mostrar la lista de comandos",
        .hint = NULL,
        .func = &cmd_help,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_help));

    // Comando reset
    const esp_console_cmd_t cmd_reset_conf = {
        .command = "reset",
        .help = "Reiniciar el sistema",
        .hint = NULL,
        .func = &cmd_reset,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_reset_conf));

    // Comando free
    const esp_console_cmd_t cmd_free_conf = {
        .command = "free",
        .help = "Obtener información de memoria libre",
        .hint = NULL,
        .func = &cmd_free,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_free_conf));

    // Comando test
    const esp_console_cmd_t test_cmd = {
        .command = "test",
        .help = "Comando de prueba simple",
        .hint = NULL,
        .func = &cmd_test,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&test_cmd));

    // Comando echo
    echo_args.message = arg_str1(NULL, NULL, "<mensaje>", "Mensaje para hacer eco");
    echo_args.end = arg_end(2);

    const esp_console_cmd_t echo_cmd = {
        .command = "echo",
        .help = "Repite el mensaje ingresado",
        .hint = NULL,
        .func = &cmd_echo,
        .argtable = &echo_args};
    ESP_ERROR_CHECK(esp_console_cmd_register(&echo_cmd));
}

// Configura la UART para la consola
static void initialize_console(void)
{
    /* Deshabilitar búfer en stdin y stdout */
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    /* Configurar UART con parámetros estándar */
    const uart_config_t uart_config = {
        .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE, // Nombre corregido
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
        // No usar source_clk en v4.4.1
    };

    /* Instalar driver UART */
    ESP_ERROR_CHECK(uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, // Nombre corregido
                                        256, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config));

    /* Configurar VFS en UART */
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);
    esp_vfs_dev_uart_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);
    esp_vfs_dev_uart_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);
}

// Tarea para ejecutar la consola
static void console_task(void *arg)
{
    initialize_console();

    /* Inicialización de linenoise */
    linenoiseSetMultiLine(1);
    linenoiseSetDumbMode(1); // Modo compatible con terminales simples
    linenoiseHistorySetMaxLen(MAX_HISTORY_SIZE);

    /* Inicializar y configurar esp_console */
    esp_console_config_t console_config = {
        .max_cmdline_length = MAX_COMMAND_LINE_SIZE,
        .max_cmdline_args = 8,
#ifdef CONFIG_LOG_COLORS
        .hint_color = atoi(LOG_COLOR_CYAN)
#endif
    };
    ESP_ERROR_CHECK(esp_console_init(&console_config));

    /* Registrar comandos */
    register_commands();

    printf("\n"
           "=================================================\n"
           "               ESP32 Console Shell               \n"
           "=================================================\n"
           "Escribe 'help' para ver los comandos disponibles\n\n");

    /* Bucle principal de la consola */
    while (true)
    {
        char *line = linenoise(PROMPT_STR);
        if (line == NULL)
        {
            continue; // Línea vacía o interrupción
        }

        if (strlen(line) > 0)
        {
            linenoiseHistoryAdd(line);
            printf("Esta es la linea chimba: %s\n", line);
            /* Procesar el comando ingresado */
            int ret;
            esp_err_t err = esp_console_run(line, &ret);
            if (err == ESP_ERR_NOT_FOUND)
            {
                printf("Comando desconocido: %s\n", line);
            }
            else if (err == ESP_ERR_INVALID_ARG)
            {
                printf("Error en los argumentos del comando\n");
            }
            else if (err == ESP_OK && ret != ESP_OK)
            {
                printf("El comando retornó un código de error: 0x%x\n", ret);
            }
            else{
                printf("error chimbo \n");
            }
        }

        linenoiseFree(line);
    }
}

// Inicializa el shell - esta es la función que llamarás desde app_main
void shell_init(void)
{
    ESP_LOGI(TAG, "Iniciando shell por UART");

    // Crea una tarea para la consola
    xTaskCreate(console_task, "console", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Shell iniciado correctamente");
}