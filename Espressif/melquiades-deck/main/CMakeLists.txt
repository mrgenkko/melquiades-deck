idf_component_register(
    SRCS    "melquiades_deck_main.c"
            "state.c"
            "audio/audio_output.c"
            "audio/sine_wave.c"
            "bluetooth/init.c"
            "leds/board.c"
            "sensors/potentiometers.c"
            "sensors/buttons.c"
            "shell/shell_init.c"
            "shell/bt_shell.c"
            "shell/uart_shell.c"
    INCLUDE_DIRS "audio" "bluetooth" "leds" "sensors" "shell"    
)
