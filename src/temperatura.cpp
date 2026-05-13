#include "temperatura.h"
#include "calibracion.h"

// ── MAX6675 (SPI bit-bang, solo lectura) ──────────────────────────────────────
#define MAX6675_CS   5
#define MAX6675_SCK  18
#define MAX6675_SO   19

// El MAX6675 necesita ~220 ms entre conversiones
#define MAX6675_MIN_PERIOD_MS  250

// Filtro IIR pasa-bajos para la temperatura: temp = α·raw + (1-α)·temp.
// α más bajo → más suave, más latencia. 0.3 da ~3-4 muestras (~1 s) de retraso,
// despreciable contra la inercia térmica de la placa.
#define TEMP_FILTER_ALPHA   0.3f

// ── Parámetros PID iniciales (ajustar según la placa real) ────────────────────
// Kp: % de salida por °C de error
// Ki: % de salida por °C·s acumulados (corrige offset en estado estacionario)
// Kd: % de salida por °C/s de cambio en la medida (amortigua oscilaciones)
#define PID_KP_DEFAULT   3.0f
#define PID_KI_DEFAULT   0.08f
#define PID_KD_DEFAULT   8.0f

// ── Estado ────────────────────────────────────────────────────────────────────
static float    lastTemp    = 0.0f;
static uint32_t lastReadMs     = 0;

static float    pidOutput      = 0.0f;
static float    pid_kp         = PID_KP_DEFAULT;
static float    pid_ki         = PID_KI_DEFAULT;
static float    pid_kd         = PID_KD_DEFAULT;
static float    pid_integral   = 0.0f;
static float    pid_prevMeas   = 0.0f;
static uint32_t pid_lastMs     = 0;
static bool     pid_firstRun   = true;

// ── MAX6675 ───────────────────────────────────────────────────────────────────
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

    if (value & (1 << 2)) return -1.0f;  // bit 2: termopar abierto
    return ((value >> 3) & 0x0FFF) * 0.25f;
}

// ── PID (derivada sobre la medida para evitar kick al cambiar setpoint) ───────
static void pidCompute(float setpoint, float dt) {
    float error = setpoint - lastTemp;

    float derivative = pid_firstRun
        ? 0.0f
        : -(lastTemp - pid_prevMeas) / dt;

    pid_firstRun = false;
    pid_prevMeas = lastTemp;

    float newIntegral = pid_integral + error * dt;
    float out = pid_kp * error + pid_ki * newIntegral + pid_kd * derivative;
    out = constrain(out, 0.0f, 100.0f);

    // Anti-windup: solo integra cuando la salida no está saturada
    if (out > 0.0f && out < 100.0f)
        pid_integral = newIntegral;

    pidOutput = out;
}

// ── API pública ───────────────────────────────────────────────────────────────
void temperaturaInit() {
    pinMode(MAX6675_CS,  OUTPUT);
    pinMode(MAX6675_SCK, OUTPUT);
    pinMode(MAX6675_SO,  INPUT);
    digitalWrite(MAX6675_CS,  HIGH);
    digitalWrite(MAX6675_SCK, LOW);

    delay(250);  // primera conversión del MAX6675

    float raw = readMax6675();
    lastTemp  = (raw >= 0.0f) ? calibrarTemperatura(raw) : 0.0f;
    lastReadMs   = millis();
    pid_prevMeas = lastTemp;
    pid_lastMs   = millis();
}

bool temperaturaUpdate(float setpoint) {
    uint32_t now = millis();
    if (now - lastReadMs < MAX6675_MIN_PERIOD_MS) return false;

    float raw = readMax6675();
    if (raw >= 0.0f) {
        float real = calibrarTemperatura(raw);
        lastTemp = TEMP_FILTER_ALPHA * real + (1.0f - TEMP_FILTER_ALPHA) * lastTemp;
    }

    float dt = (now - pid_lastMs) / 1000.0f;
    if (dt <= 0.0f) dt = 0.5f;

    lastReadMs = now;
    pid_lastMs = now;

    pidCompute(setpoint, dt);
    return true;
}

float temperaturaGetCurrent() { return lastTemp; }
float temperaturaGetOutput()  { return pidOutput;   }

void temperaturaPidReset() {
    pid_integral = 0.0f;
    pid_prevMeas = lastTemp;
    pid_firstRun = true;
    pid_lastMs   = millis();
}

void temperaturaPidSetTunings(float kp, float ki, float kd) {
    pid_kp = kp;
    pid_ki = ki;
    pid_kd = kd;
}
