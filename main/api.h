#pragma once

#include <esp_err.h>
#include <stdint.h>

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

typedef struct {
    const char* serverUrl;
} ApiClientContext;

esp_err_t ApiClient_ring(ApiClientContext* context);
esp_err_t ApiClient_heartbeat(ApiClientContext* context, DeviceHealth* health);
