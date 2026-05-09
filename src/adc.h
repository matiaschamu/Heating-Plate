#pragma once
#include <Arduino.h>

void adcInit();
float adcGetTemp();     // °C — leído del MAX6675 (CS=5, SCK=18, SO=19)
float adcGetVoltage();  // VAC — 0 V = 0 VAC, 3.3 V = 220 VAC
