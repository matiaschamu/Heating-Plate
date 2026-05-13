#include "wifimgr.h"
#include "display.h"

#include <WiFi.h>
#include <Preferences.h>

#define PREF_NAMESPACE           "wifi"
#define PREF_KEY_SSID            "ssid"
#define PREF_KEY_PASS            "pass"

#define AP_SSID                  "Alien Tech"
#define WIFI_CONNECT_TIMEOUT_MS  15000
#define IP_OCTET_SHOW_MS         700
#define IP_OCTET_GAP_MS          150

static bool s_apMode = false;

// ── NVS ───────────────────────────────────────────────────────────────────────
void wifiSaveCredentials(const String& ssid, const String& pass) {
    Preferences p;
    p.begin(PREF_NAMESPACE, false);
    p.putString(PREF_KEY_SSID, ssid);
    p.putString(PREF_KEY_PASS, pass);
    p.end();
}

void wifiClearCredentials() {
    Preferences p;
    p.begin(PREF_NAMESPACE, false);
    p.clear();
    p.end();
}

static bool loadCredentials(String& ssid, String& pass) {
    Preferences p;
    p.begin(PREF_NAMESPACE, true);
    ssid = p.getString(PREF_KEY_SSID, "");
    pass = p.getString(PREF_KEY_PASS, "");
    p.end();
    return ssid.length() > 0;
}

// ── Conexión ──────────────────────────────────────────────────────────────────
bool wifiInit() {
    String ssid, pass;
    if (!loadCredentials(ssid, pass)) return false;

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    uint32_t start     = millis();
    uint32_t lastBlink = 0;
    bool     blinkOn   = false;

    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > WIFI_CONNECT_TIMEOUT_MS) {
            WiFi.disconnect(true);
            return false;
        }
        uint32_t now = millis();
        if (now - lastBlink >= 300) {
            blinkOn   = !blinkOn;
            lastBlink = now;
            ledPower = blinkOn;
            ledTemp  = false;
            ledTimer = false;
        }
        delay(10);
    }

    // LED 3 fijo brevemente como señal de "conectado"
    ledPower = false; ledTemp = false; ledTimer = true;
    delay(300);
    ledTimer = false;

    s_apMode = false;
    return true;
}

void wifiStartAP() {
    s_apMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID);   // sin contraseña

    // Los tres LEDs fijos = modo AP esperando configuración
    ledPower = true; ledTemp = true; ledTimer = true;
}

IPAddress wifiGetIP() {
    return s_apMode ? WiFi.softAPIP() : WiFi.localIP();
}

bool wifiIsAPMode() {
    return s_apMode;
}

// ── Mostrar IP en el display ──────────────────────────────────────────────────
void wifiShowCurrentIp() {
    IPAddress ip = wifiGetIP();
    for (int i = 0; i < 4; i++) {
        displayNumber((uint16_t)ip[i]);
        uint32_t t = millis();
        while (millis() - t < IP_OCTET_SHOW_MS) delay(5);

        displayOff();
        t = millis();
        while (millis() - t < IP_OCTET_GAP_MS) delay(5);
    }
}
