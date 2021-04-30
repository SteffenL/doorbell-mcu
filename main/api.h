#pragma once

#include <esp_err.h>
#include <stdint.h>

#define DEFAULT_API_SERVER_URL "https://doorbell-server.local"

typedef struct {
    const char* level;
    uint32_t voltage;
} BatteryHealth;

typedef struct {
    const char* version;
} FirmwareInfo;

typedef struct {
    BatteryHealth battery;
    FirmwareInfo firmware;
} DeviceHealth;

typedef void (*FirmwareUpdateAvailableCallback)(
    const char* updateVersion, const char* updatePath, void* userData);

typedef struct {
    const char* serverUrl;
} ApiClientContext;

esp_err_t ApiClient_ring(ApiClientContext* context);
esp_err_t ApiClient_heartbeat(
    ApiClientContext* context,
    DeviceHealth* health,
    FirmwareUpdateAvailableCallback firmwareUpdateAvailableCallback,
    void* userData);
esp_err_t ApiClient_url(
    ApiClientContext* context, const char* path, char* url, size_t urlSize);
const char* ApiClient_getServerCertificate();
