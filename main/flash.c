#include "flash.h"

#include <nvs_flash.h>

NvsFlashStatus initFlash(void) {
    NvsFlashStatus status = NVS_FLASH_STATUS_OK;
    esp_err_t error = nvs_flash_init();
    if (error == ESP_ERR_NVS_NO_FREE_PAGES ||
        error == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        error = nvs_flash_init();
        status = NVS_FLASH_STATUS_ERASED;
    }
    ESP_ERROR_CHECK(error);
    return status;
}
