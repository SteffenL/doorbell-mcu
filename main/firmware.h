#pragma once

#include <esp_err.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FIRMWARE_VERSION_MAX_LENGTH 32

size_t getFirmwareVersion(char* version, size_t versionSize);
esp_err_t applyFirmwareUpdate(const char* url, bool restart);
