#pragma once

#include <esp_err.h>
#include <stdint.h>

typedef struct {
    uint32_t batteryLevel;
    uint32_t batteryVoltage;
} DeviceHealth;

typedef struct {
    const char* serverUrl;
} ApiClientContext;

esp_err_t ApiClient_ring(ApiClientContext* context);
esp_err_t ApiClient_ping(ApiClientContext* context, DeviceHealth* health);
