#include "wifi.h"
#include "log.h"

#include <driver/adc.h>
#include <esp_event.h>
#include <esp_wifi.h>
#include <freertos/event_groups.h>

#define LOG_TAG "wifi"
#define WIFI_MAX_TRIES 5
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static EventGroupHandle_t wifiEventGroup = NULL;
static esp_event_handler_instance_t anyWifiEventInstance = NULL;
static esp_event_handler_instance_t gotIpEventInstance = NULL;
static uint8_t wifiConnectAttempts = 0;

void wifiEventHandler(
    void* event_handler_arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void* event_data) {

    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            wifiConnectAttempts = 1;
            LOGI(LOG_TAG, "Attempting to connect to WiFi (%d/%d).", wifiConnectAttempts, WIFI_MAX_TRIES);
                esp_wifi_connect();
        } else if (event_id == WIFI_EVENT_STA_STOP) {
            // Do nothing
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            // TODO: Throttle
            if (wifiConnectAttempts < WIFI_MAX_TRIES) {
                ++wifiConnectAttempts;
                LOGI(LOG_TAG, "Attempting to reconnect to WiFi (%d/%d).", wifiConnectAttempts, WIFI_MAX_TRIES);
                esp_wifi_connect();
            } else {
                LOGE(LOG_TAG, "Unable to connect to WiFi.");
                xEventGroupSetBits(wifiEventGroup, WIFI_FAIL_BIT);
            }
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        wifiConnectAttempts = 0;
        LOGI(LOG_TAG, "Connected to WiFi.");
        xEventGroupSetBits(wifiEventGroup, WIFI_CONNECTED_BIT);
    }
}

void setWifiConfig(void) {
    wifi_config_t wifiConfig;
    ESP_ERROR_CHECK(esp_wifi_get_config(ESP_IF_WIFI_STA, &wifiConfig));

    wifiConfig.sta.scan_method = WIFI_FAST_SCAN;
    wifiConfig.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
    wifiConfig.sta.threshold.rssi = -127;
    wifiConfig.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifiConfig.sta.pmf_cfg.capable = true;
    wifiConfig.sta.pmf_cfg.required = true;

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifiConfig));
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
    setWifiConfig();
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
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
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

    esp_err_t error = esp_wifi_disconnect();

    if (error != ESP_ERR_WIFI_NOT_STARTED) {
        ESP_ERROR_CHECK(error);
    }

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
