#include "sleep.h"
#include "log.h"

#include <driver/rtc_io.h>
#include <driver/uart.h>
#include <esp_sleep.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define LOG_TAG "sleep"
#define MAX_WAKEUP_INTERVAL_IN_US 1 * 60 * 60 * 1000000LL

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
    // FIXME: Sometimes esp_sleep_get_ext1_wakeup_status() returns 0 even when
    // the wakeup cause was ESP_SLEEP_WAKEUP_EXT1 which causes this function to
    // incorrectly return false.
    // esp_sleep_get_wakeup_cause() returns ESP_SLEEP_WAKEUP_EXT1 so for
    // now just check the cause instead of the pin.
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    return cause == ESP_SLEEP_WAKEUP_EXT1;
    //return esp_sleep_get_ext1_wakeup_status() & (1ULL << pin);
}

void lightSleepNow(void) {
    LOGD(LOG_TAG, "Entering light sleep");
    // Flush UART TX FIFO before entering light sleep
    uart_wait_tx_idle_polling(CONFIG_ESP_CONSOLE_UART_NUM);
    ESP_ERROR_CHECK(esp_light_sleep_start());
}
void deepSleepNow(void) {
    LOGD(LOG_TAG, "Entering deep sleep");
    esp_deep_sleep_start();
}
