#include "tasks.h"
#include "battery.h"
#include "firmware.h"
#include "pin.h"
#include "sleep.h"
#include "wifi.h"

#include <driver/dac.h>
#include <esp_event.h>
#include <esp_task.h>
#include <esp_timer.h>
#include <freertos/event_groups.h>

#define TASK_PRIORITY_HIGH 30
#define TASK_PRIORITY_MEDIUM 20
#define TASK_PRIORITY_LOW 10

#define RING_SOUND_COMPLETED_BIT BIT0
#define RING_API_CALL_COMPLETED_BIT BIT1
#define HEARTBEAT_COMPLETED_BIT BIT0

typedef struct {
    EventGroupHandle_t group;
    int bit;
    ApiClientContext* apiClientContext;
} RingTaskParam;

typedef struct {
    EventGroupHandle_t group;
    int bit;
    ApiClientContext* apiClientContext;
    bool applyFirmwareUpdate;
    bool restartAfterFirmwareUpdate;
} HeartbeatTaskParam;

void buzz(
    adc_channel_t channel,
    uint32_t frequency,
    int8_t offset,
    uint32_t durationInMs) {

    dac_cw_config_t cwConfig = {
        .en_ch = channel,
        .scale = DAC_CW_SCALE_1,
        .phase = DAC_CW_PHASE_0,
        .freq = frequency,
        .offset = offset};

    ESP_ERROR_CHECK(dac_cw_generator_config(&cwConfig));
    ESP_ERROR_CHECK(dac_output_enable(channel));
    delayMs(durationInMs);
    ESP_ERROR_CHECK(dac_output_disable(channel));
}

void ringSoundTask(RingTaskParam* parameter) {
    buzz(BUZZER_DAC_CNANNEL, 2500, 63, 500);
    buzz(BUZZER_DAC_CNANNEL, 2500, 63, 1000);

    // TODO: report error

    xEventGroupSetBits(parameter->group, parameter->bit);
    vTaskDelete(NULL);
}

void ringApiCallTask(RingTaskParam* parameter) {
    ApiClient_ring(parameter->apiClientContext);
    // TODO: report error
    xEventGroupSetBits(parameter->group, parameter->bit);
    vTaskDelete(NULL);
}

void firmwareUpdateAvailableCallback(
    const char* updateVersion, const char* updatePath, void* userData) {

    const HeartbeatTaskParam* taskParam = (HeartbeatTaskParam*)userData;
    char updateUrl[384] = {0};

    esp_err_t error = ApiClient_url(
        taskParam->apiClientContext, updatePath, updateUrl, sizeof(updateUrl));

    if (error != ESP_OK) {
        return;
    }

    updateUrl[sizeof(updateUrl) - 1] = 0;

    if (taskParam->applyFirmwareUpdate) {
        applyFirmwareUpdate(updateUrl, taskParam->restartAfterFirmwareUpdate);
    }
}

void heartbeatTask(HeartbeatTaskParam* parameter) {
    BatteryInfo batteryInfo;
    getBatteryInfo(&batteryInfo);

    char firmwareVersion[FIRMWARE_VERSION_MAX_LENGTH];
    getFirmwareVersion(firmwareVersion, sizeof(firmwareVersion));

    DeviceHealth deviceHealth = {
        .battery =
            {.level = getBatteryLevelString(batteryInfo.level),
             .voltage = batteryInfo.voltage},
        .firmware = {.version = firmwareVersion}};

    ApiClient_heartbeat(
        parameter->apiClientContext, &deviceHealth,
        firmwareUpdateAvailableCallback, parameter);

    xEventGroupSetBits(parameter->group, parameter->bit);
    vTaskDelete(NULL);
}

void runRingTasks(ApiClientContext* apiClientContext) {
    EventGroupHandle_t ringTasksEventGroup = xEventGroupCreate();

    RingTaskParam ringSoundTaskParam = {
        .group = ringTasksEventGroup,
        .bit = RING_SOUND_COMPLETED_BIT,
        .apiClientContext = NULL};

    RingTaskParam ringCallTaskParam = {
        .group = ringTasksEventGroup,
        .bit = RING_API_CALL_COMPLETED_BIT,
        .apiClientContext = apiClientContext};

    xTaskCreate(
        (TaskFunction_t)ringSoundTask, "Ring Sound", 1024, &ringSoundTaskParam,
        TASK_PRIORITY_HIGH, NULL);

    xTaskCreate(
        (TaskFunction_t)ringApiCallTask, "Ring API Call", 4096,
        &ringCallTaskParam, TASK_PRIORITY_HIGH - 1, NULL);

    xEventGroupWaitBits(
        ringTasksEventGroup,
        RING_SOUND_COMPLETED_BIT | RING_API_CALL_COMPLETED_BIT, pdFALSE, pdTRUE,
        portMAX_DELAY);

    vEventGroupDelete(ringTasksEventGroup);
}

void runHeartbeatTask(
    ApiClientContext* apiClientContext, bool applyFirmwareUpdate) {
    EventGroupHandle_t heartbeatTaskEventGroup = xEventGroupCreate();

    HeartbeatTaskParam heartbeatTaskParam = {
        .group = heartbeatTaskEventGroup,
        .bit = HEARTBEAT_COMPLETED_BIT,
        .apiClientContext = apiClientContext,
        .applyFirmwareUpdate = applyFirmwareUpdate,
        .restartAfterFirmwareUpdate = true};

    xTaskCreate(
        (TaskFunction_t)heartbeatTask, "Heartbeat", 8192, &heartbeatTaskParam,
        TASK_PRIORITY_MEDIUM, NULL);

    xEventGroupWaitBits(
        heartbeatTaskEventGroup, HEARTBEAT_COMPLETED_BIT, pdFALSE, pdTRUE,
        portMAX_DELAY);

    vEventGroupDelete(heartbeatTaskEventGroup);
}

void handleOnDemandHeartbeatSequence(ApiClientContext* apiClientContext) {
    // Handle secret ring button sequence to initiate hearbeat.
    // Useful for triggering firmware updates.

    if (!wakeTriggeredByPin(RING_BUTTON_PIN) ||
        gpio_get_level(RING_BUTTON_PIN) == 0) {
        return;
    }

    ESP_ERROR_CHECK(gpio_set_direction(BUZZER_PIN, GPIO_MODE_OUTPUT));
    bool cancel = false;
    int64_t startTime = esp_timer_get_time();

    // Button must be pressed for 5 seconds
    while (esp_timer_get_time() - startTime < 5000 * 1000) {
        if (gpio_get_level(RING_BUTTON_PIN) == 0) {
            cancel = true;
        } else {
            delayMs(10);
        }
    }

    if (cancel) {
        return;
    }

    buzz(BUZZER_DAC_CNANNEL, 2500, 63, 50);

    // Button must be pressed for 5 times within the next 5 seconds
    int buttonState = gpio_get_level(RING_BUTTON_PIN);
    startTime = esp_timer_get_time();
    int buttonPressCount = 0;

    while (esp_timer_get_time() - startTime < 5000 * 1000 &&
           buttonPressCount < 5) {
        if (buttonState == 0 && gpio_get_level(RING_BUTTON_PIN) == 1) {
            buttonState = 1;
            ++buttonPressCount;
        } else if (buttonState == 1 && gpio_get_level(RING_BUTTON_PIN) == 0) {
            buttonState = 0;
        }

        delayMs(10);
    }

    if (buttonPressCount >= 5) {
        for (int i = 0; i < 2; ++i) {
            buzz(BUZZER_DAC_CNANNEL, 2500, 63, 50);
            delayMs(100);
        }

        startWifi();

        if (waitForWifiConnection() == WIFI_WAIT_RESULT_OK) {
            runHeartbeatTask(apiClientContext, true);
        }

        stopWifi();
    }
}
