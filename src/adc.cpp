#include "adc.h"

// Temperatura: MAX6675 (SPI bit-bang, solo lectura)
#define MAX6675_CS   5
#define MAX6675_SCK  18
#define MAX6675_SO   19

// Tensión: ADC interno
#define PIN_VOLTAGE  39

#define ADC_MAX     4095.0f
#define VOLT_MAX    220.0f

#define SAMPLES     32      // promedio de lecturas del ADC (tensión)
#define IIR_ALPHA   0.05f   // filtro IIR para tensión

// MAX6675 necesita ~220 ms entre conversiones; no leer más rápido
#define MAX6675_MIN_PERIOD_MS  250

static float    filteredVoltage = 0;
static uint32_t lastTempReadMs  = 0;
static float    lastTempRaw     = 0;     // última lectura válida del MAX6675

static float readVoltageAvg() {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < SAMPLES; i++)
        sum += analogRead(PIN_VOLTAGE);
    return (float)sum / SAMPLES;
}

// Devuelve temperatura en °C, o -1 si el termopar está desconectado
static float readMax6675() {
    digitalWrite(MAX6675_CS, LOW);
    delayMicroseconds(10);

    uint16_t value = 0;
    for (int i = 15; i >= 0; i--) {
        digitalWrite(MAX6675_SCK, LOW);
        delayMicroseconds(2);
        if (digitalRead(MAX6675_SO)) value |= (1 << i);
        digitalWrite(MAX6675_SCK, HIGH);
        delayMicroseconds(2);
    }

    digitalWrite(MAX6675_CS, HIGH);

    if (value & (1 << 2)) return -1.0f;     // bit 2 = termopar abierto
    return ((value >> 3) & 0x0FFF) * 0.25f; // 12 bits, 0.25 °C/LSB
}

void adcInit() {
    analogReadResolution(12);
    pinMode(PIN_VOLTAGE, INPUT);
    analogSetPinAttenuation(PIN_VOLTAGE, ADC_11db);   // 0-3.3 V

    pinMode(MAX6675_CS,  OUTPUT);
    pinMode(MAX6675_SCK, OUTPUT);
    pinMode(MAX6675_SO,  INPUT);
    digitalWrite(MAX6675_CS,  HIGH);
    digitalWrite(MAX6675_SCK, LOW);

    delay(250);   // primera conversión del MAX6675

    float rawTemp = readMax6675();
    if (rawTemp < 0) rawTemp = 0;
    lastTempRaw    = rawTemp;
    lastTempReadMs = millis();

    filteredVoltage = (readVoltageAvg() / ADC_MAX) * VOLT_MAX;
}

float adcGetTemp() {
    if (millis() - lastTempReadMs >= MAX6675_MIN_PERIOD_MS) {
        lastTempReadMs = millis();
        float r = readMax6675();
        if (r >= 0) lastTempRaw = r;       // si hay error, conserva la última válida
    }
    return lastTempRaw;
}

float adcGetVoltage() {
    float raw       = (readVoltageAvg() / ADC_MAX) * VOLT_MAX;
    filteredVoltage = filteredVoltage + IIR_ALPHA * (raw - filteredVoltage);
    return filteredVoltage;
}
