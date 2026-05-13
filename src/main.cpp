#include <Arduino.h>
#include "display.h"
#include "encoder.h"
#include "keyboard.h"
#include "wifimgr.h"
#include "web.h"
#include "ota.h"
#include "FanBuzzer.h"
#include "Resistencia.h"
#include "adc.h"
#include "temperatura.h"
#include "controller.h"

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
static float    currentVoltage  = 0.0f;
static uint32_t voltageLastMs   = 0;

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
        case TEMP_TIMER:
            resistenciaSet((uint8_t)temperaturaGetOutput());
            break;
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
            if (outputOn) {
                if (inTimerMode()) {
                    timerSecs   = timerSetSecs;
                    timerLastMs = millis();
                }
                if (appMode == TEMP || appMode == TEMP_TIMER)
                    temperaturaPidReset();
            }
            buzzerBeep(outputOn ? 200 : 100);
            break;

        default:
            break;
    }
}

// ── API del controller (consumida por la web) ─────────────────────────────────
void controllerGetState(AppState& out) {
    out.mode         = appMode;
    out.outputOn     = outputOn;
    out.powerWatts   = powerWatts;
    out.tempSetpoint = tempSetpoint;
    out.currentTemp  = currentTemp;
    out.timerSetSecs = timerSetSecs;
    out.timerSecs    = (outputOn && inTimerMode()) ? timerSecs : 0;
    out.resistorDuty = resistenciaGet();
    out.voltage      = currentVoltage;
}

bool controllerSetMode(AppMode m) {
    if (m > TEMP_TIMER) return false;
    if (m == OFF) {
        appMode         = OFF;
        outputOn        = false;
        showCurrentTemp = false;
        resistenciaSet(0);
        fanSet(false);
        return true;
    }
    bool    wasOn   = outputOn;
    AppMode oldMode = appMode;
    appMode         = m;
    showTimerView   = true;
    showCurrentTemp = false;

    // Si seguimos prendidos y cambiamos a un modo TEMP desde otro, reset PID
    if (wasOn && (m == TEMP || m == TEMP_TIMER) &&
        oldMode != TEMP && oldMode != TEMP_TIMER) {
        temperaturaPidReset();
    }
    return true;
}

bool controllerSetOutputOn(bool on) {
    if (appMode == OFF) return false;
    if (outputOn == on) return true;
    outputOn = on;
    if (on) {
        if (inTimerMode()) {
            timerSecs   = timerSetSecs;
            timerLastMs = millis();
        }
        if (appMode == TEMP || appMode == TEMP_TIMER)
            temperaturaPidReset();
    }
    return true;
}

bool controllerSetPowerWatts(uint16_t w) {
    if (w < 100 || w > 2000) return false;
    powerWatts = (w / 100) * 100;   // redondea al múltiplo de 100 más cercano por abajo
    return true;
}

bool controllerSetTempSetpoint(uint16_t t) {
    if (t > 650) return false;
    tempSetpoint = t;
    return true;
}

bool controllerSetTimerSecs(uint32_t s) {
    if (s < 5 || s > 99 * 60) return false;
    if (outputOn && inTimerMode() && timerSecs > 0)
        timerSecs = s;
    else
        timerSetSecs = s;
    return true;
}

// ── Setup / Loop ──────────────────────────────────────────────────────────────
void setup() {
    delay(2000);
    Serial.begin(115200);
    displayInit();
    keyboardInit();

    // Si ambos botones están pulsados al arrancar → borrar WiFi guardado
    // para poder reconfigurarlo desde el AP "Alien Tech".
    delay(80);  // pequeño debounce
    if (keyboardBothPressedRaw()) {
        wifiClearCredentials();
        displayDashes();
        ledPower = true; ledTemp = true; ledTimer = true;
        while (keyboardBothPressedRaw()) delay(10);   // esperar suelte
    }

    displayDashes();

    // Flujo WiFi: intentar conectar al WiFi guardado; si falla, levantar AP.
    if (wifiInit()) {
        wifiShowCurrentIp();
        otaInit();
        webStartControlUI();
    } else {
        wifiStartAP();
        wifiShowCurrentIp();
        webStartCaptivePortal();
    }

    encoderInit();
    fanInit();
    resistenciaInit();
    adcInit();
    temperaturaInit();

    displayDashes();
    ledPower = false;
    ledTemp  = false;
    ledTimer = false;
}

void loop() {
    otaHandle();
    webHandle();
    fanBuzzerHandle();

    temperaturaUpdate(tempSetpoint);
    currentTemp = temperaturaGetCurrent();

    if (millis() - voltageLastMs >= 200) {
        voltageLastMs   = millis();
        currentVoltage  = adcGetVoltage();
    }

    handleTimer();
    handleKeyEvent(keyboardGetEvent());
    handleEncoder(encoderGetDelta());

    applyOutput();
    updateDisplay();
}
