// bluetooth_common.h
#ifndef BLUETOOTH_COMMON_H
#define BLUETOOTH_COMMON_H

#include "esp_err.h"

/**
 * @brief Inicializa el controlador Bluetooth y el stack Bluedroid
 * @return ESP_OK si la inicialización es exitosa
 */
esp_err_t init_bluetooth_common(void);

#endif /* BLUETOOTH_COMMON_H */