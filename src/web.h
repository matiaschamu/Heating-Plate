#pragma once

// Arranca el WebServer con la UI de control del calentador (modo STA).
// Endpoints:
//   GET  /              → UI HTML
//   GET  /api/state     → JSON con estado actual
//   POST /api/mode      v=0..4
//   POST /api/output    v=0|1
//   POST /api/temp      v=0..650
//   POST /api/power     v=100..2000
//   POST /api/timer     v=5..5940
//   POST /api/reset     → ESP.restart()
void webStartControlUI();

// Arranca el WebServer + DNSServer como portal cautivo (modo AP).
// Sirve un formulario para cargar SSID/password de la red WiFi destino.
void webStartCaptivePortal();

// Llamar en cada loop(). Maneja el servidor HTTP y, si aplica, el DNS hijack.
void webHandle();
