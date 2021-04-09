#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

void app_main(void) {
    for (int i = 0;; i = (i + 1) % 10) {
        printf("[%d] Hello World\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
