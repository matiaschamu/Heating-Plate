#pragma once
#include <Arduino.h>

void  temperaturaInit();

// Llamar cada loop. Internamente respeta el período mínimo del MAX6675 (250 ms).
// Retorna true cuando se tomó una nueva muestra y se recalculó el PID.
bool  temperaturaUpdate(float setpoint);

float temperaturaGetCurrent();   // °C medidos
float temperaturaGetOutput();    // salida PID 0-100 %

// Reinicia integral y derivativo — llamar al activar la salida en modo TEMP
void  temperaturaPidReset();

// Ajuste de ganancias en caliente (Kp en %/°C, Ki en %/(°C·s), Kd en %·s/°C)
void  temperaturaPidSetTunings(float kp, float ki, float kd);
