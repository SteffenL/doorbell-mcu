#pragma once

#include <stdint.h>

typedef enum {
    BATTERY_LEVEL_HIGH,
    BATTERY_LEVEL_MODERATE,
    BATTERY_LEVEL_LOW,
    BATTERY_LEVEL_CRITICAL,
    BATTERY_LEVEL_MAX_VALUE
} BatteryLevel;

typedef struct {
    BatteryLevel level;
    uint32_t voltage;
} BatteryInfo;

void getBatteryInfo(BatteryInfo* info);
const char* getBatteryLevelString(BatteryLevel level);
