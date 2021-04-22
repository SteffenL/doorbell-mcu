#include "provisioning.h"
#include "log.h"
#include "sleep.h"
#include "wifi.h"

#include <esp_err.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <string.h>
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>

#define TAG "ftprov"

typedef enum {
    PROVISIONING_RESULT_FAIL,
    PROVISIONING_RESULT_CANCELED,
    PROVISIONING_RESULT_COMPLETED,
    PROVISIONING_RESULT_ALREADY_COMPLETED
} ProvisioningResult;

const char* PROVISIONING_ERROR_REASON_AUTH =
    "WiFi station authentication failed";
const char* PROVISIONING_ERROR_REASON_AP_NOT_FOUND =
    "WiFi access point not found";
const char* PROVISIONING_ERROR_REASON_UNKNOWN = "Unknown reason";

const uint8_t SERVICE_UUID[] = {0xea, 0x06, 0x88, 0x7b, 0xd1, 0x3a, 0xf7, 0x4f,
                                0x82, 0xa7, 0x79, 0xb7, 0x47, 0x4d, 0x39, 0x38};

// From ESP32 provisioning manager example code
static void getDeviceServiceName(char* serviceName, size_t max) {
    uint8_t eth_mac[6];
    const char* ssid_prefix = "PROV_";
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(
        serviceName, max, "%s%02X%02X%02X", ssid_prefix, eth_mac[3], eth_mac[4],
        eth_mac[5]);
}

void generateCode(char* code, size_t size) {
    esp_fill_random(code, size - 1);
    assert(code && size > 1);
    for (size_t i = 0; i < size - 1; ++i) {
        code[i] = '0' + (code[i] % 10);
    }
    code[size - 1] = 0;
}

const char*
getProvisioningErrorReasonString(wifi_prov_sta_fail_reason_t reason) {
    switch (reason) {
    case WIFI_PROV_STA_AUTH_ERROR:
        return PROVISIONING_ERROR_REASON_AUTH;
    case WIFI_PROV_STA_AP_NOT_FOUND:
        return PROVISIONING_ERROR_REASON_AP_NOT_FOUND;
    default:
        return PROVISIONING_ERROR_REASON_UNKNOWN;
    }
}

void provisioningEventHandler(
    void* event_handler_arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void* event_data) {

    if (event_base != WIFI_PROV_EVENT) {
        return;
    }

    switch (event_id) {
    case WIFI_PROV_START:
        LOGD(TAG, "Provisioning started");
        break;
    case WIFI_PROV_CRED_RECV:
        LOGD(TAG, "Received WiFi credentials");
        break;
    case WIFI_PROV_CRED_FAIL: {
        wifi_prov_sta_fail_reason_t* reason =
            (wifi_prov_sta_fail_reason_t*)event_data;
        const char* reasonString = getProvisioningErrorReasonString(*reason);
        LOGD(TAG, "Failed to receive WiFi credentials: %s", reasonString);
        break;
    }
    case WIFI_PROV_CRED_SUCCESS:
        LOGD(TAG, "Provisioning succeeded");
        break;
    case WIFI_PROV_END:
        LOGD(TAG, "Provisioning ended");
        break;
    }
}

void initProvisioning(void) {
    wifi_prov_mgr_config_t config = {
        .scheme = wifi_prov_scheme_ble,
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM};

    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
}

void deinitProvisioning(void) { wifi_prov_mgr_deinit(); }

ProvisioningResult startProvisioning(void) {
    ProvisioningResult result = PROVISIONING_RESULT_FAIL;
    esp_err_t error = ESP_FAIL;
    initProvisioning();

    do {
        bool provisioned = false;
        if ((error = wifi_prov_mgr_is_provisioned(&provisioned)) != ESP_OK) {
            break;
        }

        if (provisioned) {
            result = PROVISIONING_RESULT_ALREADY_COMPLETED;
            break;
        }

        esp_event_handler_instance_t provisioningEventInstance = NULL;
        if ((error = esp_event_handler_instance_register(
                 WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &provisioningEventHandler,
                 NULL, &provisioningEventInstance)) != ESP_OK) {
            break;
        }

        char serviceName[12] = {0};
        getDeviceServiceName(serviceName, sizeof(serviceName));

        uint8_t serviceUuid[sizeof(SERVICE_UUID)];
        memcpy(serviceUuid, SERVICE_UUID, sizeof(serviceUuid));
        if ((error = wifi_prov_scheme_ble_set_service_uuid(serviceUuid)) !=
            ESP_OK) {
            break;
        }

        char proofOfPossession[9];
        generateCode(proofOfPossession, sizeof(proofOfPossession));
        LOGI(TAG, "Proof of possession: %s", proofOfPossession);

        LOGD(TAG, "Starting provisioning");
        if ((error = wifi_prov_mgr_start_provisioning(
                 WIFI_PROV_SECURITY_1, proofOfPossession, serviceName, NULL)) !=
            ESP_OK) {
            break;
        }
        LOGD(TAG, "Waiting for provisioning to complete");
        wifi_prov_mgr_wait();

        if ((error = esp_event_handler_instance_unregister(
                 WIFI_PROV_EVENT, ESP_EVENT_ANY_ID,
                 provisioningEventInstance)) != ESP_OK) {
            break;
        }

        result = PROVISIONING_RESULT_COMPLETED;
    } while (0);

    deinitProvisioning();

    switch (result) {
    case PROVISIONING_RESULT_FAIL:
        LOGE(TAG, "Provisioning failed with error %d", error);
        break;
    case PROVISIONING_RESULT_CANCELED:
        LOGD(TAG, "Provisioning canceled");
        break;
    case PROVISIONING_RESULT_COMPLETED:
        LOGD(TAG, "Provisioning completed successfully");
        break;
    case PROVISIONING_RESULT_ALREADY_COMPLETED:
        LOGD(TAG, "Provisioning already completed");
        break;
    }

    return result;
}

void runFirstTimeProvisioning(void) {
    ProvisioningResult provisioningResult = startProvisioning();

    switch (provisioningResult) {
    case PROVISIONING_RESULT_ALREADY_COMPLETED:
        break;
    case PROVISIONING_RESULT_COMPLETED:
        // Restarting after provisioning somehow reduces the time it takes to
        // establish subsequent WiFi connections.
        stopWifi();
        esp_restart();
        break;
    default:
        // If provisioning is not complete then we can't proceed.
        deepSleepNow();
        return;
    }
}
