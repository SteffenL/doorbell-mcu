#pragma once

#include <stddef.h>
#include <stdint.h>

#define FIRMWARE_VERSION_MAX_LENGTH 32

size_t getFirmwareVersion(char* version, size_t versionSize);
