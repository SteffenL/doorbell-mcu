#pragma once

#include <stdint.h>

void initAdc(void);
uint32_t sampleVoltage(int channel, uint32_t sampleCount);
