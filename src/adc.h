#pragma once
#include <Arduino.h>

void  adcInit();
float adcGetVoltage();  // VAC — 0 V = 0 VAC, 3.3 V = 220 VAC
