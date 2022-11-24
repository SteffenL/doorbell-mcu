#include "firmware.h"
#include "api.h"

#include <esp_https_ota.h>
#include <esp_ota_ops.h>
#include <string.h>

static const int HTTP_TIMEOUT_IN_MS = 5000;

esp_err_t firmwareHttpEventHandler(esp_http_client_event_t* evt) {
    return ESP_OK;
}

size_t getFirmwareVersion(char* version, size_t versionSize) {
    const esp_app_desc_t* appDescription = esp_ota_get_app_description();
    size_t versionLength = strlen(appDescription->version);
    versionLength =
        versionLength >= versionSize ? versionSize - 1 : versionLength;
    memset(version, 0, versionSize);
    strncpy(version, appDescription->version, versionLength);
    return versionLength;
}

esp_err_t applyFirmwareUpdate(const char* url, bool restart) {
    esp_http_client_config_t config;
    memset(&config, 0, sizeof(esp_http_client_config_t));

    config.url = url;
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = HTTP_TIMEOUT_IN_MS;
    config.event_handler = firmwareHttpEventHandler;
    config.user_data = NULL;
    // TODO: Should be set to false in production code
    config.skip_cert_common_name_check = true;
    config.cert_pem = ApiClient_getServerCertificate();

    esp_err_t error = esp_https_ota(&config);

    if (error == ESP_OK && restart) {
        esp_restart();
    }

    return ESP_OK;
}
