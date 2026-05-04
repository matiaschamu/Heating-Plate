#pragma once
#include <Arduino.h>

void adcInit();
float adcGetTemp();     // °C  — 0 V = 0°C, 3.3 V = 650°C
float adcGetVoltage();  // VAC — 0 V = 0 VAC, 3.3 V = 220 VAC
