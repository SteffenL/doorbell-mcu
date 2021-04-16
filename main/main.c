#include "adc.h"
#include "api.h"
#include "battery.h"
#include "flash.h"
#include "pin.h"
#include "sleep.h"
#include "tasks.h"
#include "wifi.h"

#include <driver/dac.h>
#include <esp_err.h>
#include <esp_event.h>
#include <esp_log.h>

static ApiClientContext apiClientContext = {.serverUrl = SERVER_URL};

void setup(void) {
    esp_log_level_set("*", ESP_LOG_ERROR);

    NvsFlashStatus flashStatus = initFlash();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    initSleep(1 << RING_BUTTON_PIN);
    ESP_ERROR_CHECK(dac_cw_generator_enable());
    initAdc();
    initWifi();

    if (flashStatus == NVS_FLASH_STATUS_ERASED) {
        setWifiConfig();
    }
}

void loop(void) {
    startWifi();

    if (waitForWifiConnection() == WIFI_WAIT_RESULT_OK) {
        if (wakeTriggeredByPin(RING_BUTTON_PIN)) {
            runRingTasks(&apiClientContext);
        } else {
            runPingTask(&apiClientContext);
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
