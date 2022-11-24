#pragma once

#include <esp_log.h>

#define LOG_TAG_PREFIX "doorbell_"

#define MAKE_LOG_TAG(s) LOG_TAG_PREFIX s

#define LOG_LEVEL_E ESP_LOG_ERROR
#define LOG_LEVEL_W ESP_LOG_WARN
#define LOG_LEVEL_I ESP_LOG_INFO
#define LOG_LEVEL_D ESP_LOG_DEBUG
#define LOG_LEVEL_V ESP_LOG_VERBOSE

#define LOG(levelLetter, tag, format, ...)                                     \
    {                                                                          \
        esp_log_write(                                                         \
            LOG_LEVEL_##levelLetter, MAKE_LOG_TAG(tag),                        \
            LOG_FORMAT(levelLetter, format), esp_log_timestamp(),              \
            MAKE_LOG_TAG(tag), ##__VA_ARGS__);                                 \
    }

#define LOGE(tag, format, ...) LOG(E, tag, format, ##__VA_ARGS__)
#define LOGW(tag, format, ...) LOG(W, tag, format, ##__VA_ARGS__)
#define LOGI(tag, format, ...) LOG(I, tag, format, ##__VA_ARGS__)
#define LOGD(tag, format, ...) LOG(D, tag, format, ##__VA_ARGS__)
#define LOGV(tag, format, ...) LOG(V, tag, format, ##__VA_ARGS__)
