#include "firmware.h"

#include <esp_ota_ops.h>
#include <string.h>

size_t getFirmwareVersion(char* version, size_t versionSize) {
    const esp_partition_t* runningPartition = esp_ota_get_running_partition();
    esp_app_desc_t runningAppInfo;
    ESP_ERROR_CHECK(
        esp_ota_get_partition_description(runningPartition, &runningAppInfo));
    size_t versionLength = strlen(runningAppInfo.version);
    versionLength =
        versionLength >= versionSize ? versionSize - 1 : versionLength;
    memset(version, 0, versionSize);
    strncpy(version, runningAppInfo.version, versionLength);
    return versionLength;
}
