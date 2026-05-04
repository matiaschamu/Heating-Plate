#include "adc.h"

#define PIN_TEMP    36
#define PIN_VOLTAGE 39

#define ADC_MAX     4095.0f
#define TEMP_MAX    650.0f   // °C  a fondo de escala (3.3 V)
#define VOLT_MAX    220.0f   // VAC a fondo de escala (3.3 V)

#define SAMPLES     16       // promedio de lecturas para reducir ruido

static float readAvg(uint8_t pin) {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < SAMPLES; i++)
        sum += analogRead(pin);
    return (float)sum / SAMPLES;
}

void adcInit() {
    analogReadResolution(12);     // 0-4095
    analogSetAttenuation(ADC_11db); // rango 0-3.3 V
    pinMode(PIN_TEMP,    INPUT);
    pinMode(PIN_VOLTAGE, INPUT);
}

float adcGetTemp() {
    return (readAvg(PIN_TEMP) / ADC_MAX) * TEMP_MAX;
}

float adcGetVoltage() {
    return (readAvg(PIN_VOLTAGE) / ADC_MAX) * VOLT_MAX;
}
