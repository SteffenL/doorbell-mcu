#include "sleep.h"
#include "log.h"

#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAG "sleep"
#define MAX_WAKEUP_INTERVAL_IN_US 6 * 60 * 60 * 1000000LL

void delayMs(uint32_t time) { vTaskDelay(time / portTICK_PERIOD_MS); }
void yield() { vTaskDelay(1); }

void initSleep(uint64_t wakeupPinMask) {
    // Reduce deep sleep current
    ESP_ERROR_CHECK(
        esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF));
    ESP_ERROR_CHECK(rtc_gpio_isolate(GPIO_NUM_12));

    ESP_ERROR_CHECK(esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL));
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(MAX_WAKEUP_INTERVAL_IN_US));
    ESP_ERROR_CHECK(
        esp_sleep_enable_ext1_wakeup(wakeupPinMask, ESP_EXT1_WAKEUP_ANY_HIGH));
}

bool wakeTriggeredByPin(uint8_t pin) {
    return esp_sleep_get_ext1_wakeup_status() & (1ULL << pin);
}

void lightSleepNow(void) {
    LOGD(TAG, "Entering light sleep");
    ESP_ERROR_CHECK(esp_light_sleep_start());
}
void deepSleepNow(void) {
    LOGD(TAG, "Entering deep sleep");
    esp_deep_sleep_start();
}
