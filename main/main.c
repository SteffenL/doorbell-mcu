#include "api.h"
#include "flash.h"
#include "pin.h"
#include "sleep.h"
#include "wifi.h"

#include <esp_err.h>
#include <esp_event.h>
#include <esp_log.h>

ApiClientContext apiClientContext = {.serverUrl = SERVER_URL};
DeviceHealth deviceHealth = {.batteryLevel = 0, .batteryVoltage = 0};

void setup(void) {
    esp_log_level_set("*", ESP_LOG_ERROR);

    NvsFlashStatus flashStatus = initFlash();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    initSleep(1 << RING_BUTTON_PIN);
    initWifi();

    if (flashStatus == NVS_FLASH_STATUS_ERASED) {
        setWifiConfig();
    }
}

void loop(void) {
    startWifi();

    if (waitForWifiConnection() == WIFI_WAIT_RESULT_OK) {
        if (wakeTriggeredByPin(RING_BUTTON_PIN)) {
            ApiClient_ring(&apiClientContext);
        } else {
            ApiClient_ping(&apiClientContext, &deviceHealth);
        }
    }

    stopWifi();
    sleepNow();
}

void app_main(void) {
    setup();

    while (true) {
        loop();
    }
}
