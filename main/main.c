#include <driver/rtc_io.h>
#include <esp_event.h>
#include <esp_http_client.h>
#include <esp_sleep.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <stdio.h>
#include <string.h>

#ifndef WIFI_SSID
#error WIFI_SSID must be defined.
#endif

#ifndef WIFI_PASSPHRASE
#error WIFI_PASSPHRASE must be defined.
#endif

#ifndef SERVER_URL
#error SERVER_URL must be defined.
#endif

#define WIFI_MAX_RETRY 2
#define FAILULRE_WAKEUP_INTERVAL_IN_US 20 * 1000000LL
#define MAX_WAKEUP_INTERVAL_IN_US 6 * 60 * 60 * 1000000LL
#define HTTP_TIMEOUT_IN_MS 5000
#define RING_BUTTON_PIN GPIO_NUM_15
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static EventGroupHandle_t wifiEventGroup;
static esp_event_handler_instance_t anyWifiEventInstance;
static esp_event_handler_instance_t gotIpEventInstance;
static uint8_t wifiRetryCount = 0;

bool wakeTriggeredByPin(uint8_t pin) {
    return esp_sleep_get_ext1_wakeup_status() & (1ULL << pin);
}

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

void initSleep(void) {
    // Reduce deep sleep current
    ESP_ERROR_CHECK(
        esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF));
    ESP_ERROR_CHECK(rtc_gpio_isolate(GPIO_NUM_12));

    ESP_ERROR_CHECK(esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL));
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(MAX_WAKEUP_INTERVAL_IN_US));
    ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup(
        1 << RING_BUTTON_PIN, ESP_EXT1_WAKEUP_ANY_HIGH));
}

void initWifi(void) {
    wifiEventGroup = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifiInitConfig = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifiInitConfig));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiEventHandler, NULL,
        &anyWifiEventInstance));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifiEventHandler, NULL,
        &gotIpEventInstance));

    wifi_config_t wifiConfig = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSPHRASE,
            .scan_method = WIFI_FAST_SCAN,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
            .threshold = {.rssi = -127, .authmode = WIFI_AUTH_WPA2_PSK},
            .pmf_cfg = {.capable = true, .required = true}}};

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifiConfig));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void deinitWifi(void) {
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(
        IP_EVENT, IP_EVENT_STA_GOT_IP, gotIpEventInstance));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(
        WIFI_EVENT, ESP_EVENT_ANY_ID, anyWifiEventInstance));
    vEventGroupDelete(wifiEventGroup);
    esp_wifi_stop();
}

esp_err_t httpEventHandler(esp_http_client_event_t* evt) { return ESP_OK; }

esp_err_t httpRequest(const char* url, esp_http_client_method_t method) {
    esp_http_client_config_t config = {
        .url = url,
        .method = method,
        .timeout_ms = HTTP_TIMEOUT_IN_MS,
        .event_handler = httpEventHandler,
        .user_data = NULL};
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t error = esp_http_client_perform(client);

    if (error == ESP_OK) {
        // int statusCode = esp_http_client_get_status_code(client);
        // int contentLength = esp_http_client_get_content_length(client);
        // TODO
    }

    ESP_ERROR_CHECK(esp_http_client_cleanup(client));
    return error;
}

typedef void* (*AllocatorType)(size_t size);
typedef void (*DeallocatorType)(void* memory);

char* createUrl(const char* base, const char* path, AllocatorType allocator) {
    const unsigned int baseLength = strlen(base);
    const unsigned int pathLength = strlen(path);
    const unsigned int urlLength = baseLength + pathLength;
    const unsigned int urlBufferLength = urlLength + 1;

    char* url = (char*)allocator(urlBufferLength);
    assert(url);

    memset(url, 0, urlBufferLength);
    memcpy(url, base, baseLength);
    memcpy(url + baseLength, path, pathLength);
    url[urlLength] = 0;

    return url;
}

void freeUrl(const char* url, DeallocatorType deallocator) {
    deallocator((void*)url);
}

typedef struct {
    const char* serverUrl;
} ApiClient;

esp_err_t ApiClient_request(
    ApiClient* client, const char* path, esp_http_client_method_t method) {
    const char* url = createUrl(client->serverUrl, path, malloc);

    if (!url) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t error = httpRequest(url, method);
    freeUrl(url, free);
    return error;
}

esp_err_t ApiClient_ring(ApiClient* client) {
    return ApiClient_request(client, "/ring", HTTP_METHOD_POST);
}

esp_err_t ApiClient_ping(ApiClient* client) {
    return ApiClient_request(client, "/ping", HTTP_METHOD_POST);
}

ApiClient apiClient = {.serverUrl = SERVER_URL};

void app_main(void) {
    esp_err_t error = nvs_flash_init();
    if (error == ESP_ERR_NVS_NO_FREE_PAGES ||
        error == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        error = nvs_flash_init();
    }
    ESP_ERROR_CHECK(error);

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    initSleep();
    initWifi();

    EventBits_t bits = xEventGroupWaitBits(
        wifiEventGroup, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE,
        portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        if (wakeTriggeredByPin(RING_BUTTON_PIN)) {
            ApiClient_ring(&apiClient);
        } else {
            ApiClient_ping(&apiClient);
        }
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_ERROR_CHECK(
            esp_sleep_enable_timer_wakeup(FAILULRE_WAKEUP_INTERVAL_IN_US));
    }

    deinitWifi();
    esp_deep_sleep_start();
}
