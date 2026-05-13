#include "adc.h"

#define PIN_VOLTAGE  39
#define ADC_MAX     4095.0f
// Tensión a fondo de escala del ADC (3.3 V) según calibración con multímetro.
// Refinada en zona operativa: real=215 V → mostraba 220 V → 301.4 · 215/220 ≈ 294.5
#define VOLT_MAX    294.5f
#define SAMPLES     32
#define IIR_ALPHA   0.05f

static float filteredVoltage = 0;

static float readVoltageAvg() {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < SAMPLES; i++)
        sum += analogRead(PIN_VOLTAGE);
    return (float)sum / SAMPLES;
}

void adcInit() {
    analogReadResolution(12);
    pinMode(PIN_VOLTAGE, INPUT);
    analogSetPinAttenuation(PIN_VOLTAGE, ADC_11db);
    filteredVoltage = (readVoltageAvg() / ADC_MAX) * VOLT_MAX;
}

float adcGetVoltage() {
    float raw       = (readVoltageAvg() / ADC_MAX) * VOLT_MAX;
    filteredVoltage = filteredVoltage + IIR_ALPHA * (raw - filteredVoltage);
    return filteredVoltage;
}
