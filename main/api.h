#pragma once

#include <esp_err.h>
#include <stdint.h>

typedef struct {
    const char* level;
    uint32_t voltage;
} BatteryHealth;

typedef struct {
    BatteryHealth battery;
} DeviceHealth;

typedef struct {
    const char* serverUrl;
} ApiClientContext;

esp_err_t ApiClient_ring(ApiClientContext* context);
esp_err_t ApiClient_ping(ApiClientContext* context, DeviceHealth* health);
