#include "bluetooth_common.h"
//bibliotecas de sistema
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"

#define BT_COMMON_TAG "BT_COMMON"
#define BT_DEVICE_NAME "Melquiades-Deck"

static bool bluetooth_initialized = false;

esp_err_t init_bluetooth_common(void)
{
    if (bluetooth_initialized) {
        ESP_LOGI(BT_COMMON_TAG, "Bluetooth ya está inicializado");
        return ESP_OK;
    }

    esp_err_t ret;
    ESP_LOGI(BT_COMMON_TAG, "Inicializando Bluetooth común");

    // Liberar memoria BLE si solo usaremos Bluetooth clásico
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    // Configuración del controlador Bluetooth
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    // Inicializar controlador BT
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE(BT_COMMON_TAG, "Inicialización del controlador BT falló: %s", esp_err_to_name(ret));
        return ret;
    }

    // Habilitar controlador BT en modo clásico
    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
        ESP_LOGE(BT_COMMON_TAG, "Habilitación del controlador BT falló: %s", esp_err_to_name(ret));
        return ret;
    }

    // Inicializar Bluedroid
    if ((ret = esp_bluedroid_init()) != ESP_OK) {
        ESP_LOGE(BT_COMMON_TAG, "Inicialización de Bluedroid falló: %s", esp_err_to_name(ret));
        return ret;
    }

    // Habilitar Bluedroid
    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(BT_COMMON_TAG, "Habilitación de Bluedroid falló: %s", esp_err_to_name(ret));
        return ret;
    }

    // Establecer el nombre del dispositivo - un solo nombre para ambos perfiles
    esp_bt_dev_set_device_name(BT_DEVICE_NAME);
    
    // Configurar el dispositivo como visible y conectable
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

    bluetooth_initialized = true;
    ESP_LOGI(BT_COMMON_TAG, "Bluetooth inicializado correctamente");
    return ESP_OK;
}