#pragma once
#include <Arduino.h>

// Estado de elementos especiales (modificables directamente desde main)
extern volatile bool showColon;
extern volatile bool showDP2;
extern volatile bool ledPower;
extern volatile bool ledTemp;
extern volatile bool ledTimer;

void displayInit();

void displayNumber(uint16_t num);
void displayTime(uint8_t h, uint8_t m);
void displayOff();
void displayDashes();            // muestra "----"
void displayCoolingAnimation();  // animación de enfriamiento (A → G → D)
