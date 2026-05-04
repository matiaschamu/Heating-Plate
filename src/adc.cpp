#include "adc.h"

#define PIN_TEMP    36
#define PIN_VOLTAGE 39

#define ADC_MAX     4095.0f
#define TEMP_MAX    650.0f
#define VOLT_MAX    220.0f

#define SAMPLES     32      // promedio de lecturas por muestra
#define IIR_ALPHA   0.05f   // filtro IIR: valores bajos = más suavizado (0.05 ≈ τ 20 muestras)

static float filteredTemp    = 0;
static float filteredVoltage = 0;
static bool  initialized     = false;

static float readAvg(uint8_t pin) {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < SAMPLES; i++)
        sum += analogRead(pin);
    return (float)sum / SAMPLES;
}

void adcInit() {
    analogReadResolution(12);
    pinMode(PIN_TEMP,    INPUT);
    pinMode(PIN_VOLTAGE, INPUT);
    analogSetPinAttenuation(PIN_TEMP,    ADC_0db);    // 0-1.1 V, maxima precision
    analogSetPinAttenuation(PIN_VOLTAGE, ADC_11db);   // 0-3.3 V, rango completo

    // Precarga el filtro con la lectura real para evitar arranque desde 0
    float rawTemp    = (readAvg(PIN_TEMP)    / ADC_MAX) * TEMP_MAX;
    float rawVoltage = (readAvg(PIN_VOLTAGE) / ADC_MAX) * VOLT_MAX;
    filteredTemp     = rawTemp;
    filteredVoltage  = rawVoltage;
    initialized      = true;
}

float adcGetTemp() {
    float raw    = (readAvg(PIN_TEMP) / ADC_MAX) * TEMP_MAX;
    filteredTemp = filteredTemp + IIR_ALPHA * (raw - filteredTemp);
    return filteredTemp;
}

float adcGetVoltage() {
    float raw        = (readAvg(PIN_VOLTAGE) / ADC_MAX) * VOLT_MAX;
    filteredVoltage  = filteredVoltage + IIR_ALPHA * (raw - filteredVoltage);
    return filteredVoltage;
}
