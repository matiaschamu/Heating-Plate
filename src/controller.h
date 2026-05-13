#pragma once
#include <Arduino.h>

// Modos de operación del calentador
enum AppMode : uint8_t { OFF, POWER, POWER_TIMER, TEMP, TEMP_TIMER };

struct AppState {
    AppMode  mode;
    bool     outputOn;
    uint16_t powerWatts;     // 100-2000
    uint16_t tempSetpoint;   // 0-650 °C
    float    currentTemp;
    uint32_t timerSetSecs;   // configuración
    uint32_t timerSecs;      // restante; 0 si no está corriendo
    uint8_t  resistorDuty;   // salida actual de la resistencia 0-100 %
};

void controllerGetState(AppState& out);

// Todas devuelven true si el cambio se aplicó (input válido / cambio permitido)
bool controllerSetMode(AppMode m);
bool controllerSetOutputOn(bool on);
bool controllerSetPowerWatts(uint16_t w);
bool controllerSetTempSetpoint(uint16_t t);
bool controllerSetTimerSecs(uint32_t s);
