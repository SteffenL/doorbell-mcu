idf_component_register(
    SRCS "provisioning.c" "firmware.c" "battery.c" "adc.c" "tasks.c" "sleep.c" "flash.c" "api.c" "wifi.c" "main.c"
    INCLUDE_DIRS ""
    EMBED_TXTFILES "${project_dir}/certs/server.cert.pem"
)
