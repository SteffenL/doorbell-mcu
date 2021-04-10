idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS ""
)

set(WIFI_SSID "" CACHE STRING "WiFi network name (SSID)")
set(WIFI_PASSPHRASE "" CACHE STRING "WiFi network passphrase")

target_compile_options(${COMPONENT_LIB} PRIVATE "-DWIFI_SSID=\"${WIFI_SSID}\"" "-DWIFI_PASSPHRASE=\"${WIFI_PASSPHRASE}\"")
