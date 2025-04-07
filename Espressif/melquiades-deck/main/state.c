#include "state.h"

//Estados booleanos
bool sensors_streaming_bt = false;
bool sensors_streaming_uart = false;

// Inicialización de TaskHandles
TaskHandle_t tasl_ledboard_handle = NULL;
TaskHandle_t task_pot_handle = NULL;
TaskHandle_t task_btn_handle = NULL;

//Inicializamos CHARS
const char cmd_commands[] = 
        "Comandos disponibles:\r\n"                         
        "  start_led_board - Iniciar LED\r\n"
        "  stop_led_board - Detener LED\r\n"
        "  start_sensors - Iniciamos lectura de sensores\r\n"
        "  stop_sensors - Detenemos lectura de sensores\r\n"
        "  help - Mostrar esta ayuda\r\n"
        "  status - Estado de variables y tasks\r\n";