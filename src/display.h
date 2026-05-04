#pragma once
#include <Arduino.h>

// Estado de elementos especiales (modificables directamente desde main)
extern bool showColon;
extern bool showDP2;
extern bool ledPower;
extern bool ledTemp;
extern bool ledTimer;

void displayInit();
void refreshDisplay();

void displayNumber(uint16_t num);
void displayTime(uint8_t h, uint8_t m);
void displayOff();
void displayDashes();            // muestra "----"
void displayCoolingAnimation();  // animación de enfriamiento (A → G → D)
