#pragma once

// Inicializa ArduinoOTA. Llamar solo cuando ya estás conectado a WiFi en
// modo STA. Expone el hostname mDNS "AlienTech.local".
void otaInit();

// Llamar en cada loop(). Procesa las solicitudes de OTA upload.
void otaHandle();
