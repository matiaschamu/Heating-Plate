#include "ota.h"
#include <ArduinoOTA.h>

static bool s_inited = false;

void otaInit() {
    ArduinoOTA.setHostname("AlienTech");
    ArduinoOTA.begin();
    s_inited = true;
}

void otaHandle() {
    if (s_inited) ArduinoOTA.handle();
}
