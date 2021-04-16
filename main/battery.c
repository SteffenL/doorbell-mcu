#include "battery.h"
#include "adc.h"
#include "pin.h"

// Should be less than rated battery voltage in mV
static const uint32_t HIGH_BATTERY_VOLTAGE = 3600;
static const uint32_t MODERATE_BATTERY_VOLTAGE = 3500;
static const uint32_t LOW_BATTERY_VOLTAGE = 3400;
static const uint32_t BATTERY_VOLTAGE_MULTIPLIER = 2;
static const int MAX_BATTERY_VOLTAGE_SAMPLES = 128;
static const char* BATTERY_LEVEL_STRINGS[] = {
    "high", "moderate", "low", "critical"};

uint32_t getCurrentBatteryVoltage(void) {
    uint32_t readVoltage =
        sampleVoltage(BATTERY_ADC_CHANNEL, MAX_BATTERY_VOLTAGE_SAMPLES);
    return readVoltage * BATTERY_VOLTAGE_MULTIPLIER;
}

BatteryLevel getBatteryLevel(uint32_t voltage) {
    if (voltage >= HIGH_BATTERY_VOLTAGE) {
        return BATTERY_LEVEL_HIGH;
    } else if (voltage >= MODERATE_BATTERY_VOLTAGE) {
        return BATTERY_LEVEL_MODERATE;
    } else if (voltage >= LOW_BATTERY_VOLTAGE) {
        return BATTERY_LEVEL_LOW;
    } else {
        return BATTERY_LEVEL_CRITICAL;
    }
}

const char* getBatteryLevelString(BatteryLevel level) {
    const unsigned int stringsCount =
        sizeof(BATTERY_LEVEL_STRINGS) / sizeof(BATTERY_LEVEL_STRINGS[0]);

    if (level >= BATTERY_LEVEL_MAX_VALUE || level >= stringsCount) {
        return "unknown";
    }

    return BATTERY_LEVEL_STRINGS[level];
}

void getBatteryInfo(BatteryInfo* info) {
    uint32_t voltage = getCurrentBatteryVoltage();
    info->level = getBatteryLevel(voltage);
    info->voltage = voltage;
}
