#include "debug.h"
#include "pin.h"

bool isDebugEnabled() {
    gpio_set_direction(DEBUG_ENABLE_PIN, GPIO_MODE_INPUT);
    return gpio_get_level(DEBUG_ENABLE_PIN) == 0;
}
