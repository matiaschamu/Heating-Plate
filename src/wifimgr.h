#pragma once
#include <Arduino.h>
#include <IPAddress.h>

// Intenta conectarse al último WiFi guardado en NVS. Bloquea hasta ~15 s.
// Devuelve true si se conectó, false si no había credenciales o falló.
// Durante el intento parpadea LED 1 para indicación visual.
bool      wifiInit();

// Levanta el AP "Alien Tech" (sin contraseña). Llamar si wifiInit() falló.
void      wifiStartAP();

// Guarda SSID/password en NVS para el próximo arranque.
void      wifiSaveCredentials(const String& ssid, const String& pass);

// Borra las credenciales guardadas. Llamar antes de wifiInit() para forzar AP.
void      wifiClearCredentials();

// IP actual: del STA si está conectado, del AP si está en modo AP.
IPAddress wifiGetIP();
bool      wifiIsAPMode();

// Muestra la IP actual en el display, octeto por octeto, una vez.
void      wifiShowCurrentIp();
