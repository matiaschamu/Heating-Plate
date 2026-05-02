#include "FanBuzzer.h"

#define FAN_PIN 5

static bool fanState = false;

void fanInit() {
    pinMode(FAN_PIN, OUTPUT);
    digitalWrite(FAN_PIN, LOW);
}

void fanSet(bool on) {
    fanState = on;
    digitalWrite(FAN_PIN, on ? HIGH : LOW);
}

bool fanGet() {
    return fanState;
}
