#pragma once
#include <Arduino.h>

void keyboardInit();

// Retorna +10, -10, o 0 según la tecla presionada desde la última llamada
int8_t keyboardGetDelta();
