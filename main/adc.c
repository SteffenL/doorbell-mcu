#include "adc.h"
#include "pin.h"

#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <math.h>
#include <string.h>

static const adc_bits_width_t ADC_BIT_WIDTH = ADC_WIDTH_BIT_12;
static const adc_atten_t ADC_ATTENUATION = ADC_ATTEN_DB_6;
// Approximate ADC voltage reference in mV
static const uint32_t ADC_VREF = 1100;

static esp_adc_cal_characteristics_t* adcCalChar;

void initAdc(void) {
    adcCalChar = malloc(sizeof(esp_adc_cal_characteristics_t));
    memset(adcCalChar, 0, sizeof(esp_adc_cal_characteristics_t));

    esp_adc_cal_characterize(
        ADC_UNIT_1, ADC_ATTENUATION, ADC_BIT_WIDTH, ADC_VREF, adcCalChar);

    ESP_ERROR_CHECK(adc1_config_width(ADC_BIT_WIDTH));
    ESP_ERROR_CHECK(
        adc1_config_channel_atten(BATTERY_ADC_CHANNEL, ADC_ATTENUATION));
}

uint32_t sampleVoltage(int channel, uint32_t sampleCount) {
    uint32_t readings = 0;

    for (uint32_t i = 0; i < sampleCount; ++i) {
        readings += adc1_get_raw((adc1_channel_t)channel);
    }

    double reading = (double)readings / sampleCount;
    double voltage = esp_adc_cal_raw_to_voltage(reading, adcCalChar);

    return voltage;
}
