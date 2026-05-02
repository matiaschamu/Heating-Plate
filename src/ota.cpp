#include "ota.h"
#include "credentials.h"
#include "display.h"

#include <WiFi.h>
#include <ArduinoOTA.h>

// ── Indicación visual del estado WiFi ─────────────────────────────────────────
//   LED 1 (ledPower)  destellando        →  buscando red
//   LED 2 (ledTemp)   fijo               →  negociando / asociando
//   LED 3 (ledTimer)  fijo               →  conectado (se apaga al terminar init)
//   Los tres fijos                       →  error (red no encontrada / fallo)

static void setLeds(bool p, bool t, bool ti) {
    ledPower = p;
    ledTemp  = t;
    ledTimer = ti;
}

void otaInit() {
    WiFi.mode(WIFI_STA);
    WiFi.config(
        IPAddress(192, 168, 1, 210),
        IPAddress(192, 168, 1,   1),
        IPAddress(255, 255, 255,  0)
    );
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t lastBlink = 0;
    bool     blinkOn   = false;

    while (WiFi.status() != WL_CONNECTED) {
        uint32_t     now    = millis();
        wl_status_t  status = WiFi.status();

        switch (status) {

            case WL_DISCONNECTED:
            case WL_IDLE_STATUS:
                // LED 1 destellando: buscando red
                if (now - lastBlink >= 300) {
                    blinkOn   = !blinkOn;
                    lastBlink = now;
                }
                setLeds(blinkOn, false, false);
                break;

            case WL_SCAN_COMPLETED:
                // LED 2 fijo: red encontrada, negociando asociación
                setLeds(false, true, false);
                break;

            case WL_NO_SSID_AVAIL:
            case WL_CONNECT_FAILED:
            case WL_CONNECTION_LOST:
            default:
                // Todos los LEDs fijos: error
                setLeds(true, true, true);
                break;
        }

        refreshDisplay();
        delay(10);
    }

    // LED 3 fijo: conectado — visible 1 segundo y luego se apaga
    setLeds(false, false, true);
    uint32_t t = millis();
    while (millis() - t < 1000) {
        refreshDisplay();
        delay(10);
    }
    setLeds(false, false, false);

    ArduinoOTA.setHostname("HeatingPlate");
    ArduinoOTA.begin();
}

void otaHandle() {
    ArduinoOTA.handle();
}
