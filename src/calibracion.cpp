#include "calibracion.h"
#include <stddef.h>

// ── Tabla de calibración ──────────────────────────────────────────────────────
// Cada par: (lectura cruda del MAX6675, temperatura real medida en °C).
// Los puntos deben estar ordenados de menor a mayor en `raw`.
//
// Para recalibrar: poná el equipo en cada setpoint, esperá que se estabilice,
// medí con un termómetro patrón sobre la placa y reemplazá los pares.
struct CalibPoint { float raw; float real; };

static const CalibPoint CALIB_TABLE[] = {
    {  35.0f,  58.0f },
    {  50.0f,  98.0f },
    {  75.0f, 150.0f },
    { 100.0f, 205.0f },
    { 125.0f, 250.0f },
    { 150.0f, 300.0f },
    { 175.0f, 340.0f },
    { 200.0f, 373.0f },
    { 225.0f, 405.0f },
    { 240.0f, 418.0f },
};

static const size_t CALIB_N = sizeof(CALIB_TABLE) / sizeof(CALIB_TABLE[0]);

static float interpolate(const CalibPoint& a, const CalibPoint& b, float x) {
    float slope = (b.real - a.real) / (b.raw - a.raw);
    return a.real + slope * (x - a.raw);
}

float calibrarTemperatura(float raw) {
    // Por debajo del primer punto: extrapola con la pendiente del primer segmento
    if (raw <= CALIB_TABLE[0].raw)
        return interpolate(CALIB_TABLE[0], CALIB_TABLE[1], raw);

    // Por encima del último punto: extrapola con la pendiente del último segmento
    if (raw >= CALIB_TABLE[CALIB_N - 1].raw)
        return interpolate(CALIB_TABLE[CALIB_N - 2], CALIB_TABLE[CALIB_N - 1], raw);

    // Buscar el segmento que contiene `raw`
    for (size_t i = 0; i < CALIB_N - 1; i++) {
        if (raw >= CALIB_TABLE[i].raw && raw <= CALIB_TABLE[i + 1].raw)
            return interpolate(CALIB_TABLE[i], CALIB_TABLE[i + 1], raw);
    }

    return raw;  // unreachable
}
