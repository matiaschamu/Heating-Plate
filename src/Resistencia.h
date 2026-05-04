#pragma once
#include <Arduino.h>

void resistenciaInit();
void resistenciaSet(uint8_t power);  // 0-100 %
uint8_t resistenciaGet();
