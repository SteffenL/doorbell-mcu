#include "api.h"
#include "adc.h"

#include <esp_http_client.h>
#include <string.h>

static const int HTTP_TIMEOUT_IN_MS = 5000;
// static const int HTTP_MAX_CONTENT_LENGTH = 65536;
static const char* API_CONTENT_TYPE = "application/flatmap";

extern const uint8_t serverCertPemStart[] asm("_binary_server_cert_pem_start");
extern const uint8_t serverCertPemEnd[] asm("_binary_server_cert_pem_end");

esp_err_t httpEventHandler(esp_http_client_event_t* evt) { return ESP_OK; }

esp_err_t httpRequest(
    const char* url,
    esp_http_client_method_t method,
    const char* requestContentType,
    const char* requestContent,
    uint32_t requestContentLength,
    char* responseBody,
    size_t responseBodySize,
    size_t* responseBodyBytesRead) {

    esp_http_client_config_t config;
    memset(&config, 0, sizeof(esp_http_client_config_t));

    config.url = url;
    config.method = method;
    config.timeout_ms = HTTP_TIMEOUT_IN_MS;
    config.event_handler = httpEventHandler;
    config.user_data = NULL;
    config.skip_cert_common_name_check = true;
    config.cert_pem = (const char*)serverCertPemStart;

    esp_http_client_handle_t client = esp_http_client_init(&config);

    if (requestContentType) {
        ESP_ERROR_CHECK(esp_http_client_set_header(
            client, "Content-Type", requestContentType));
    }

    if (requestContent) {
        ESP_ERROR_CHECK(esp_http_client_set_post_field(
            client, requestContent, requestContentLength));
    }

    esp_err_t error = ESP_FAIL;

    error = esp_http_client_perform(client);

    if (error == ESP_OK) {
        int statusCode = esp_http_client_get_status_code(client);
        int contentLength = esp_http_client_get_content_length(client);

        if (contentLength > 0 && responseBody && responseBodySize > 0) {
            int bytesToRead = contentLength + 1 > responseBodySize
                                  ? responseBodySize - 1
                                  : contentLength;
            int bytesRead = esp_http_client_read_response(
                client, responseBody, bytesToRead);
            responseBody[bytesRead] = 0;

            if (responseBodyBytesRead) {
                *responseBodyBytesRead = bytesRead;
            }
        }
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

typedef void (*ParseFlatmapCallback)(
    const char* key, const char* value, void* userData);

void parseFlatmap(
    const char* input,
    size_t length,
    ParseFlatmapCallback callback,
    void* userData) {
    if (!input || length == 0 || !callback) {
        return;
    }

    char* buffer = malloc(length);

    for (size_t i = 0; i < length;) {
        char* key = &buffer[i];
        char* keyEnd = key;
        while (i < length && input[i] != '=') {
            buffer[i] = input[i];
            ++i;
            ++keyEnd;
        }

        buffer[i] = 0;
        ++i;

        char* value = &buffer[i];
        char* valueEnd = value;
        while (i < length && input[i] != '\n') {
            buffer[i] = input[i];
            ++i;
            ++valueEnd;
        }

        buffer[i] = 0;
        ++i;

        callback(key, value, userData);
    }

    buffer[length - 1] = 0;

    free(buffer);
}

typedef struct {
    char updateVersion[32];
    char updatePath[256];
} HeartbeatResponse;

void parseHeartbeatFlatmapCallback(
    const char* key, const char* value, void* userData) {

    if (!userData) {
        return;
    }

    HeartbeatResponse* response = (HeartbeatResponse*)userData;

    if (strcmp(key, "update.version") == 0) {
        const int targetSize = sizeof(response->updateVersion) /
                               sizeof(response->updateVersion[0]);
        response->updateVersion[0] = 0;
        strncpy(response->updateVersion, value, targetSize);
        response->updateVersion[targetSize - 1] = 0;
        return;
    }

    if (strcmp(key, "update.path") == 0) {
        const int targetSize =
            sizeof(response->updatePath) / sizeof(response->updatePath[0]);
        response->updatePath[0] = 0;
        strncpy(response->updatePath, value, targetSize);
        response->updatePath[targetSize - 1] = 0;
        return;
    }
}

esp_err_t ApiClient_request(
    ApiClientContext* context,
    const char* path,
    esp_http_client_method_t method,
    const char* content,
    uint32_t contentLength,
    char* responseBody,
    size_t responseBodySize,
    size_t* responseBodyBytesRead) {

    const char* url = createUrl(context->serverUrl, path, malloc);

    if (!url) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t error = httpRequest(
        url, method, API_CONTENT_TYPE, content, contentLength, responseBody,
        responseBodySize, responseBodyBytesRead);

    freeUrl(url, free);
    return error;
}

esp_err_t ApiClient_ring(ApiClientContext* context) {
    return ApiClient_request(
        context, "/ring", HTTP_METHOD_POST, NULL, 0, NULL, 0, NULL);
}

esp_err_t ApiClient_heartbeat(
    ApiClientContext* context,
    DeviceHealth* health,
    FirmwareUpdateAvailableCallback firmwareUpdateAvailableCallback,
    void* userData) {

    const char* requestBodyFormat = "battery.level=%s\n"
                                    "battery.voltage=%u\n"
                                    "firmware.version=%s\n";
    char requestBody[1024] = {0};

    int requestBodyLength = snprintf(
        requestBody, sizeof(requestBody), requestBodyFormat,
        health->battery.level, health->battery.voltage,
        health->firmware.version);

    char responseBody[1024] = {0};
    size_t responseBodyLength = 0;

    esp_err_t error = ApiClient_request(
        context, "/heartbeat", HTTP_METHOD_POST, requestBody, requestBodyLength,
        responseBody, sizeof(responseBody), &responseBodyLength);

    if (error == ESP_OK && firmwareUpdateAvailableCallback) {
        HeartbeatResponse heartbeatResponse;
        memset(&heartbeatResponse, 0, sizeof(heartbeatResponse));
        parseFlatmap(
            responseBody, responseBodyLength, parseHeartbeatFlatmapCallback,
            &heartbeatResponse);
        if (heartbeatResponse.updateVersion[0] &&
            heartbeatResponse.updatePath[0]) {
            firmwareUpdateAvailableCallback(
                heartbeatResponse.updateVersion, heartbeatResponse.updatePath,
                userData);
        }
    }

    return error;
}

esp_err_t ApiClient_url(
    ApiClientContext* context, const char* path, char* url, size_t urlSize) {
    if (!path || !url || urlSize == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    const char* url_ = createUrl(context->serverUrl, path, malloc);

    if (!url_) {
        return ESP_ERR_NO_MEM;
    }

    url[0] = 0;
    strncpy(url, url_, urlSize - 1);

    freeUrl(url_, free);
    return ESP_OK;
}

const char* ApiClient_getServerCertificate() {
    return (const char*)serverCertPemStart;
}
