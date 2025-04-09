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
        "  led_board start - Iniciar LED\r\n"
        "  led_board stop - Detener LED\r\n"
        "  sensors start - Iniciamos lectura de sensores\r\n"
        "  sensors stop - Detenemos lectura de sensores\r\n"
        "  set_volume 70 - Definimos volumen del dispositivo\r\n"
        "  eq flat - Cambiamos ecualizacion a defecto, flat\r\n"
        "  eq bass_boost - Cambiamos ecualizacion para bajos, bass_boost\r\n"
        "  eq mid_boost - Cambiamos ecualizacion a frecuencias medias, mid_boost\r\n"
        "  eq treble_boost - Cambiamos ecualizacion a treble, treble_boost\r\n"
        "  eq vocal - Cambiamos ecualizacion a vocal, vocal\r\n"
        "  headphone_balance -0.2 - Cambiamos balance de los audifonos, desplazamos a izquierda o derecha\r\n"
        "  dsp enabled - Activamos DSP, filtrado de audio\r\n"
        "  dsp disabled - Desactivamos DSP, dejamos audio como venga del sistema\r\n"
        "  status - Estado de variables y tasks\r\n"
        "  help - Comando de ayuda, desplegamos comandos disponibles\r\n";