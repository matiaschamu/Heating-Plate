#include <Arduino.h>
#include "display.h"
#include "encoder.h"
#include "keyboard.h"
#include "ota.h"
#include "FanBuzzer.h"
#include "Resistencia.h"

// Tiempo actual expresado en minutos totales (0 = 00:00, 1439 = 23:59)
static int16_t timeMinutes = 12 * 60 + 34;

static void applyDelta(int8_t delta) {
    timeMinutes += delta;
    if (timeMinutes < 0)    timeMinutes += 1440;
    if (timeMinutes >= 1440) timeMinutes -= 1440;

    displayTime(timeMinutes / 60, timeMinutes % 60);
}

void setup() {
    delay(2000);
    Serial.begin(115200);
    displayInit();
    otaInit();
    encoderInit();
    keyboardInit();
    fanInit();
    resistenciaInit();

    displayTime(timeMinutes / 60, timeMinutes % 60);
    ledPower = true;
}

void loop() {
    otaHandle();
    fanBuzzerHandle();
    refreshDisplay();

    int8_t delta = encoderGetDelta();
    if (delta != 0) {
        applyDelta(delta);
        buzzerBeep(20);
    }

    delta = keyboardGetDelta();
    if (delta != 0) applyDelta(delta);

    fanSet(timeMinutes > 12 * 60 + 40);

    // 12:29 → 100%, 12:28 → 99%, ... se clampea en 0
    int16_t power = timeMinutes - (12 * 60 + 29 - 99);  // = timeMinutes - 649
    resistenciaSet(timeMinutes >= 12 * 60 + 30 ? 0 : (uint8_t)constrain(power, 0, 100));
}
