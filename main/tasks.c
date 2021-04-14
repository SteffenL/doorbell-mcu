#include "tasks.h"
#include "pin.h"
#include "sleep.h"

#include <driver/dac.h>
#include <esp_event.h>
#include <esp_task.h>
#include <freertos/event_groups.h>

#define RING_SOUND_COMPLETED_BIT BIT0
#define RING_API_CALL_COMPLETED_BIT BIT1

typedef struct {
    EventGroupHandle_t group;
    int bit;
    ApiClientContext* apiClientContext;
} RingTaskParam;

void ringSoundTask(RingTaskParam* parameter) {
    esp_err_t error = ESP_OK;

    do {
        dac_cw_config_t cwConfig1 = {
            .en_ch = BUZZER_DAC_CNANNEL,
            .scale = DAC_CW_SCALE_1,
            .phase = DAC_CW_PHASE_0,
            .freq = 2500,
            .offset = 0};

        if ((error = dac_cw_generator_config(&cwConfig1)) != ESP_OK) {
            break;
        }

        if ((error = dac_output_enable(BUZZER_DAC_CNANNEL)) != ESP_OK) {
            break;
        }

        delayMs(500);

        if ((error = dac_output_disable(BUZZER_DAC_CNANNEL)) != ESP_OK) {
            break;
        }

        dac_cw_config_t cwConfig2 = {
            .en_ch = BUZZER_DAC_CNANNEL,
            .scale = DAC_CW_SCALE_1,
            .phase = DAC_CW_PHASE_0,
            .freq = 2000,
            .offset = 0};

        if ((error = dac_cw_generator_config(&cwConfig2)) != ESP_OK) {
            break;
        }

        if ((error = dac_output_enable(BUZZER_DAC_CNANNEL)) != ESP_OK) {
            break;
        }

        delayMs(1000);

        if ((error = dac_output_disable(BUZZER_DAC_CNANNEL)) != ESP_OK) {
            break;
        }
    } while (0);

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
        1, NULL);

    xTaskCreate(
        (TaskFunction_t)ringApiCallTask, "Ring API Call", 2048,
        &ringCallTaskParam, 1, NULL);

    xEventGroupWaitBits(
        ringTasksEventGroup,
        RING_SOUND_COMPLETED_BIT | RING_API_CALL_COMPLETED_BIT, pdFALSE, pdTRUE,
        portMAX_DELAY);

    vEventGroupDelete(ringTasksEventGroup);
}
