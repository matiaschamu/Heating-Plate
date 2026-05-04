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
