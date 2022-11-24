#pragma once

#include <stdbool.h>
#include <stdint.h>

void delayMs(uint32_t time);
void yield();
void initSleep(uint64_t wakeupPinMask);
bool wakeTriggeredByPin(uint8_t pin);
void lightSleepNow(void);
void deepSleepNow(void);
