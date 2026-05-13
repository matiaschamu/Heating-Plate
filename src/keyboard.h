#pragma once
#include <Arduino.h>

enum KeyEvent : uint8_t {
    KEY_NONE,
    KEY_POWER_PRESS,
    KEY_FUNCION_PRESS,
    KEY_BOTH_PRESS,
};

void keyboardInit();
KeyEvent keyboardGetEvent();

// Lectura cruda de ambos botones (LOW = pulsado). Llamar después de
// keyboardInit() y de un pequeño debounce. Útil al inicio del setup().
bool keyboardBothPressedRaw();
