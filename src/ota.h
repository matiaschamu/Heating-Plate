#pragma once

void otaInit();   // conecta WiFi y arranca OTA (bloquea hasta conectar)
void otaHandle(); // llamar en cada loop()
