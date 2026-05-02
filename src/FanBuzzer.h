#pragma once
#include <Arduino.h>

void fanInit();
void fanSet(bool on);
bool fanGet();

// Hace sonar el buzzer durante `ms` milisegundos (no bloqueante)
void buzzerBeep(uint32_t ms);

// Llamar en cada loop() para gestionar el fin del beep
void fanBuzzerHandle();
