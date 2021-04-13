#include "api.h"

#include <esp_http_client.h>
#include <string.h>

#ifndef SERVER_URL
#error SERVER_URL must be defined.
#endif

static const int HTTP_TIMEOUT_IN_MS = 5000;
// static const int HTTP_MAX_CONTENT_LENGTH = 65536;
static const char* API_CONTENT_TYPE = "application/json";

esp_err_t httpEventHandler(esp_http_client_event_t* evt) { return ESP_OK; }

esp_err_t httpRequest(
    const char* url,
    esp_http_client_method_t method,
    const char* requestContentType,
    const char* requestContent,
    uint32_t requestContentLength) {

    esp_http_client_config_t config = {
        .url = url,
        .method = method,
        .timeout_ms = HTTP_TIMEOUT_IN_MS,
        .event_handler = httpEventHandler,
        .user_data = NULL};
    esp_http_client_handle_t client = esp_http_client_init(&config);
    printf("URL: %s\n", url);

    if (requestContentType) {
        printf("Setting content type: %s\n", requestContentType);
        ESP_ERROR_CHECK(esp_http_client_set_header(
            client, "Content-Type", requestContentType));
    }

    if (requestContent) {
        printf("Setting content: %s\n", requestContent);
        // Number of digits in uint32 + null character
        /*char requestContentLengthString[11];
        memset(
            requestContentLengthString, 0, sizeof(requestContentLengthString));
        snprintf(
            requestContentLengthString, sizeof(requestContentLengthString),
            "%u", requestContentLength);
        ESP_ERROR_CHECK(esp_http_client_set_header(
            client, "Content-Length", requestContentLengthString));*/
        ESP_ERROR_CHECK(esp_http_client_set_post_field(
            client, requestContent, requestContentLength));
    }

    esp_err_t error = ESP_FAIL;

    // error = esp_http_client_open(client, requestContentLength);
    // error = esp_http_client_write(client, requestContent,
    // requestContentLength);
    error = esp_http_client_perform(client);

    if (error == ESP_OK) {
        int statusCode = esp_http_client_get_status_code(client);
        int contentLength = esp_http_client_get_content_length(client);
        printf("HTTP %d, %d\n", statusCode, contentLength);
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

esp_err_t ApiClient_request(
    ApiClientContext* context,
    const char* path,
    esp_http_client_method_t method,
    const char* content,
    uint32_t contentLength) {

    const char* url = createUrl(context->serverUrl, path, malloc);

    if (!url) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t error =
        httpRequest(url, method, API_CONTENT_TYPE, content, contentLength);
    freeUrl(url, free);
    return error;
}

esp_err_t ApiClient_ring(ApiClientContext* context) {
    return ApiClient_request(context, "/ring", HTTP_METHOD_POST, NULL, 0);
}

esp_err_t ApiClient_ping(ApiClientContext* context, DeviceHealth* health) {
    const char* jsonFormat = "{\"battery\":{\"level\":%u,\"voltage\":%u}}";
    char requestBody[1024];
    memset(requestBody, 0, sizeof(requestBody));
    int requestBodyLength = snprintf(
        requestBody, sizeof(requestBody), jsonFormat, health->batteryLevel,
        health->batteryVoltage);
    return ApiClient_request(
        context, "/ping", HTTP_METHOD_POST, requestBody, requestBodyLength);
}
