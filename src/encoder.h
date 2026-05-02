#pragma once
#include <Arduino.h>

void encoderInit();

// Retorna los pasos acumulados desde la última llamada (positivo = CW, negativo = CCW)
int8_t encoderGetDelta();
