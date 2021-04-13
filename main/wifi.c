#include "wifi.h"

#include <driver/adc.h>
#include <esp_event.h>
#include <esp_wifi.h>
#include <freertos/event_groups.h>

#ifndef WIFI_SSID
#error WIFI_SSID must be defined.
#endif

#ifndef WIFI_PASSPHRASE
#error WIFI_PASSPHRASE must be defined.
#endif

#define WIFI_MAX_RETRY 2
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static EventGroupHandle_t wifiEventGroup = NULL;
static esp_event_handler_instance_t anyWifiEventInstance = NULL;
static esp_event_handler_instance_t gotIpEventInstance = NULL;
static uint8_t wifiRetryCount = 0;

void wifiEventHandler(
    void* event_handler_arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void* event_data) {

    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            esp_wifi_connect();
        } else if (event_id == WIFI_EVENT_STA_STOP) {
            // Do nothing
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            // TODO: Throttle
            if (wifiRetryCount < WIFI_MAX_RETRY) {
                esp_wifi_connect();
                ++wifiRetryCount;
            } else {
                xEventGroupSetBits(wifiEventGroup, WIFI_FAIL_BIT);
            }
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        wifiRetryCount = 0;
        xEventGroupSetBits(wifiEventGroup, WIFI_CONNECTED_BIT);
    }
}

void initWifi(void) {
    if (!wifiEventGroup) {
        wifiEventGroup = xEventGroupCreate();
    }

    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifiInitConfig = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifiInitConfig));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
}

void setWifiConfig(void) {
    wifi_config_t wifiConfig = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSPHRASE,
            .scan_method = WIFI_FAST_SCAN,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
            .threshold = {.rssi = -127, .authmode = WIFI_AUTH_WPA2_PSK},
            .pmf_cfg = {.capable = true, .required = true}}};

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifiConfig));
}

void deinitWifi(void) {
    if (wifiEventGroup) {
        vEventGroupDelete(wifiEventGroup);
        wifiEventGroup = NULL;
    }
}

void startWifi(void) {
    if (!anyWifiEventInstance) {
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiEventHandler, NULL,
            &anyWifiEventInstance));
    }

    if (!gotIpEventInstance) {
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            IP_EVENT, IP_EVENT_STA_GOT_IP, &wifiEventHandler, NULL,
            &gotIpEventInstance));
    }

    adc_power_on();
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MAX_MODEM));
}

void stopWifi(void) {
    if (gotIpEventInstance) {
        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(
            IP_EVENT, IP_EVENT_STA_GOT_IP, gotIpEventInstance));
        gotIpEventInstance = NULL;
    }

    if (anyWifiEventInstance) {
        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(
            WIFI_EVENT, ESP_EVENT_ANY_ID, anyWifiEventInstance));
        anyWifiEventInstance = NULL;
    }

    ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_ERROR_CHECK(esp_wifi_stop());
    adc_power_off();
}

WifiWaitResult waitForWifiConnection(void) {
    EventBits_t bits = xEventGroupWaitBits(
        wifiEventGroup, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdTRUE, pdFALSE,
        portMAX_DELAY);
    return (bits & WIFI_CONNECTED_BIT) ? WIFI_WAIT_RESULT_OK
                                       : WIFI_WAIT_RESULT_FAIL;
}
