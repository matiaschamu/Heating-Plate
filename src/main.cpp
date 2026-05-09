#include <Arduino.h>
#include "display.h"
#include "encoder.h"
#include "keyboard.h"
#include "ota.h"
#include "FanBuzzer.h"
#include "Resistencia.h"
#include "adc.h"

// ── Modos ─────────────────────────────────────────────────────────────────────
enum AppMode : uint8_t { OFF, POWER, POWER_TIMER, TEMP, TEMP_TIMER };

// ── Estado global ──────────────────────────────────────────────────────────────
static AppMode  appMode       = OFF;

static bool inTimerMode() { return appMode == POWER_TIMER || appMode == TEMP_TIMER; }
static bool     outputOn      = false;
static bool     showTimerView    = true;   // en modos timer: true=timer, false=potencia/temp
static bool     showCurrentTemp  = false;  // en modo TEMP: true=temperatura leida, false=setpoint
static bool     showTempInOff   = false;  // en OFF durante enfriamiento: muestra temperatura actual

static uint16_t powerWatts    = 1000;   // 100-2000 W, paso 100 W
static uint32_t timerSetSecs  = 10 * 60; // configuración del timer (segundos). Sube de a 5s, baja de a 60s
static uint32_t timerSecs     = 0;       // countdown en curso
static uint16_t tempSetpoint  = 100;    // 0-650 °C
static float    currentTemp   = 0;

static uint32_t timerLastMs   = 0;
static uint32_t tempLastMs    = 0;

// ── Salidas ───────────────────────────────────────────────────────────────────
static void applyOutput() {
    // Fan: activo mientras haya salida, y hasta que la placa baje de 50 °C
    if (outputOn) {
        fanSet(true);
    } else {
        if      (currentTemp >= 47.0f) fanSet(true);
        else if (currentTemp <  42.0f) fanSet(false);
        // entre 42 y 47: mantiene estado actual (histéresis)
    }

    if (!outputOn) { resistenciaSet(0); return; }

    switch (appMode) {
        case POWER:
        case POWER_TIMER:
            resistenciaSet((uint8_t)(powerWatts / 20));   // 100W=5% … 2000W=100%
            break;
        case TEMP:
        case TEMP_TIMER: {
            float error = (float)tempSetpoint - currentTemp;
            resistenciaSet((uint8_t)constrain((int)(error * 5.0f), 0, 100));
            break;
        }
        default:
            resistenciaSet(0);
    }
}

// ── Display ───────────────────────────────────────────────────────────────────
static void updateDisplay() {
    switch (appMode) {
        case OFF:
            if (currentTemp >= 35.0f) {
                if (showTempInOff)
                    displayNumber((uint16_t)currentTemp);
                else
                    displayCoolingAnimation();
            } else {
                showTempInOff = false;
                displayDashes();
            }
            showColon = false;
            break;

        case POWER:
            displayNumber(showCurrentTemp ? (uint16_t)currentTemp : powerWatts);
            showColon = false;
            break;

        case POWER_TIMER:
            if (showTimerView) {
                // Vista timer: MM:SS countdown o MM:00 si está detenido
                if (outputOn && timerSecs > 0)
                    displayTime((uint8_t)(timerSecs / 60), (uint8_t)(timerSecs % 60));
                else
                    displayTime((uint8_t)(timerSetSecs / 60), (uint8_t)(timerSetSecs % 60));
            } else {
                displayNumber(powerWatts);
                showColon = false;
            }
            break;

        case TEMP:
            displayNumber(showCurrentTemp ? (uint16_t)currentTemp : tempSetpoint);
            showColon = false;
            break;

        case TEMP_TIMER:
            if (showTimerView) {
                if (outputOn && timerSecs > 0)
                    displayTime((uint8_t)(timerSecs / 60), (uint8_t)(timerSecs % 60));
                else
                    displayTime((uint8_t)(timerSetSecs / 60), (uint8_t)(timerSetSecs % 60));
            } else {
                displayNumber(tempSetpoint);
                showColon = false;
            }
            break;
    }

    // Con salida activa los LEDs de modo destellan a 1 Hz
    bool ledOn = !outputOn || (millis() / 500) % 2 == 0;
    ledPower = ledOn && (appMode == POWER       || appMode == POWER_TIMER);
    ledTemp  = ledOn && (appMode == TEMP        || appMode == TEMP_TIMER);
    ledTimer = ledOn && (appMode == POWER_TIMER || appMode == TEMP_TIMER);
}

// ── Countdown ─────────────────────────────────────────────────────────────────
static void handleTimer() {
    if (!outputOn || !inTimerMode()) return;

    if (millis() - timerLastMs >= 1000) {
        timerLastMs = millis();
        if (timerSecs > 0) {
            timerSecs--;
        } else {
            outputOn = false;
            buzzerBeep(1000);
        }
    }
}

// ── Encoder ───────────────────────────────────────────────────────────────────
static void handleEncoder(int8_t delta) {
    if (appMode == OFF || delta == 0) return;

    buzzerBeep(20);

    switch (appMode) {
        case POWER:
            powerWatts = (uint16_t)constrain((int)powerWatts + delta * 100, 100, 2000);
            break;

        case POWER_TIMER:
            if (showTimerView) {
                int32_t step = delta > 0 ? delta * 60 : delta * 5;  // sube 60s (1 min), baja 5s
                auto timerWrap = [](int32_t val) -> uint32_t {
                    if (val > 99 * 60) return 5;          // rollover al máximo → mínimo
                    return (uint32_t)constrain(val, 5, 99 * 60);
                };
                if (outputOn && timerSecs > 0)
                    timerSecs    = timerWrap((int32_t)timerSecs    + step);
                else
                    timerSetSecs = timerWrap((int32_t)timerSetSecs + step);
            } else {
                powerWatts = (uint16_t)constrain((int)powerWatts + delta * 100, 100, 2000);
            }
            break;

        case TEMP:
            tempSetpoint = (uint16_t)constrain((int)tempSetpoint + delta * 5, 0, 650);
            break;

        case TEMP_TIMER:
            if (showTimerView) {
                int32_t step = delta > 0 ? delta * 60 : delta * 5;  // sube 60s (1 min), baja 5s
                auto timerWrap = [](int32_t val) -> uint32_t {
                    if (val > 99 * 60) return 5;          // rollover al máximo → mínimo
                    return (uint32_t)constrain(val, 5, 99 * 60);
                };
                if (outputOn && timerSecs > 0)
                    timerSecs    = timerWrap((int32_t)timerSecs    + step);
                else
                    timerSetSecs = timerWrap((int32_t)timerSetSecs + step);
            } else {
                tempSetpoint = (uint16_t)constrain((int)tempSetpoint + delta * 5, 0, 650);
            }
            break;

        default:
            break;
    }
}

// ── Eventos de teclado ────────────────────────────────────────────────────────
static void handleKeyEvent(KeyEvent ev) {
    switch (ev) {

        case KEY_POWER_PRESS:
            buzzerBeep(30);
            if (appMode == OFF) {
                appMode       = POWER;
                outputOn      = false;
                showTimerView = true;
                showTempInOff = false;
            } else {
                appMode         = OFF;
                outputOn        = false;
                showCurrentTemp = false;
                resistenciaSet(0);
                fanSet(false);
            }
            break;

        case KEY_FUNCION_PRESS:
            buzzerBeep(30);
            if (appMode == OFF) {
                if (currentTemp >= 35.0f)
                    showTempInOff = !showTempInOff;
                break;
            }

            if (outputOn) {
                // Con salida activa: alterna vista según el modo, resto bloqueado
                if (inTimerMode())
                    showTimerView = !showTimerView;
                else if (appMode == TEMP || appMode == POWER)
                    showCurrentTemp = !showCurrentTemp;
            } else {
                // Sin salida: cicla modos y resetea vistas
                showTimerView   = true;
                showCurrentTemp = false;
                switch (appMode) {
                    case POWER:       appMode = POWER_TIMER; break;
                    case POWER_TIMER: appMode = TEMP;        break;
                    case TEMP:        appMode = TEMP_TIMER;  break;
                    case TEMP_TIMER:  appMode = POWER;       break;
                    default:          break;
                }
            }
            break;

        case KEY_BOTH_PRESS:
            if (appMode == OFF) break;
            outputOn = !outputOn;
            if (outputOn && inTimerMode()) {
                timerSecs   = timerSetSecs;
                timerLastMs = millis();
            }
            buzzerBeep(outputOn ? 200 : 100);
            break;

        default:
            break;
    }
}

// ── Setup / Loop ──────────────────────────────────────────────────────────────
void setup() {
    delay(2000);
    Serial.begin(115200);
    displayInit();
    otaInit();
    encoderInit();
    keyboardInit();
    fanInit();
    resistenciaInit();
    adcInit();

    displayDashes();
    ledPower = false;
    ledTemp  = false;
    ledTimer = false;
}

void loop() {
    otaHandle();
    fanBuzzerHandle();

    if (millis() - tempLastMs >= 500) {
        tempLastMs  = millis();
        currentTemp = adcGetTemp();
    }

    handleTimer();
    handleKeyEvent(keyboardGetEvent());
    handleEncoder(encoderGetDelta());

    applyOutput();
    updateDisplay();
}
