#include "Resistencia.h"

#define RESISTENCIA_PIN 18

static bool resistenciaState = false;

void resistenciaInit() {
    pinMode(RESISTENCIA_PIN, OUTPUT);
    digitalWrite(RESISTENCIA_PIN, LOW);
}

void resistenciaSet(bool on) {
    resistenciaState = on;
    digitalWrite(RESISTENCIA_PIN, on ? HIGH : LOW);
}

bool resistenciaGet() {
    return resistenciaState;
}
