#include <Arduino.h>
#include "display.h"
#include "encoder.h"

// Tiempo actual expresado en minutos totales (0 = 00:00, 1439 = 23:59)
static int16_t timeMinutes = 12 * 60 + 34;

static void applyDelta(int8_t delta) {
    timeMinutes += delta;
    if (timeMinutes < 0)    timeMinutes += 1440;
    if (timeMinutes >= 1440) timeMinutes -= 1440;

    displayTime(timeMinutes / 60, timeMinutes % 60);
}

void setup() {
    Serial.begin(115200);
    displayInit();
    encoderInit();

    displayTime(timeMinutes / 60, timeMinutes % 60);
    ledPower = true;
}

void loop() {
    refreshDisplay();

    int8_t delta = encoderGetDelta();
    if (delta != 0) applyDelta(delta);
}
