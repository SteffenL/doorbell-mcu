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

esp_err_t networkConnectionHandler(void) {
    if (waitForWifiConnection() == WIFI_WAIT_RESULT_OK) {
        return ESP_OK;
    }
    LOGE(LOG_TAG, "Unable to establish a WiFi connection.");
    return ESP_FAIL;
}

void setup(void) {
    NvsFlashStatus flashStatus = initFlash();
    LOGD(LOG_TAG, "NVS flash status: %d", flashStatus);

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    initSleep(1 << RING_BUTTON_PIN);
    ESP_ERROR_CHECK(dac_cw_generator_enable());
    initAdc();
    initWifi();
    runFirstTimeProvisioning();
    ApiClient_setNetworkConnectHandler(networkConnectionHandler);
}

void loop(void) {
    bool wokenByRingButton = wakeTriggeredByPin(RING_BUTTON_PIN);
    startWifi();

    if (wokenByRingButton) {
        runRingTasks(&apiClientContext);
    } else {
        runHeartbeatTask(&apiClientContext, false);
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
