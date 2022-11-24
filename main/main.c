#include "adc.h"
#include "api.h"
#include "battery.h"
#include "flash.h"
#include "log.h"
#include "pin.h"
#include "provisioning.h"
#include "sleep.h"
#include "tasks.h"
#include "wifi.h"

#include <driver/dac.h>
#include <esp_err.h>
#include <esp_event.h>

static ApiClientContext apiClientContext = {
    .serverUrl = DEFAULT_API_SERVER_URL};

#define LOG_TAG  "main"

void setup(void) {
    NvsFlashStatus flashStatus = initFlash();
    LOGD(LOG_TAG, "NVS flash status: %d", flashStatus);

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    initSleep(1 << RING_BUTTON_PIN);
    ESP_ERROR_CHECK(dac_cw_generator_enable());
    initAdc();
    initWifi();
    runFirstTimeProvisioning();
}

void loop(void) {
    bool wokenByRingButton = wakeTriggeredByPin(RING_BUTTON_PIN);
    startWifi();

    if (waitForWifiConnection() == WIFI_WAIT_RESULT_OK) {
        if (wokenByRingButton) {
            runRingTasks(&apiClientContext);
        } else {
            runHeartbeatTask(&apiClientContext, false);
        }
    } else {
        LOGE(LOG_TAG, "Cannot run tasks because there is no WiFi connection.");
    }

    stopWifi();
    handleOnDemandHeartbeatSequence(&apiClientContext);
    lightSleepNow();
}

void app_main(void) {
    setup();

    while (true) {
        loop();
    }
}
