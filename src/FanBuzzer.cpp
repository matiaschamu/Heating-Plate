#include "FanBuzzer.h"

#define FAN_PIN   5
#define LEDC_CH   0
#define LEDC_FREQ 3900
#define LEDC_RES  8        // 8 bits → duty 0-255

static bool     fanState    = false;
static uint32_t buzzerUntil = 0;    // millis() cuando debe terminar el beep, 0 = inactivo

static void applyFan() {
    ledcDetachPin(FAN_PIN);
    pinMode(FAN_PIN, OUTPUT);
    digitalWrite(FAN_PIN, fanState ? HIGH : LOW);
}

static void applyBuzzer() {
    ledcSetup(LEDC_CH, LEDC_FREQ, LEDC_RES);
    ledcAttachPin(FAN_PIN, LEDC_CH);
    ledcWrite(LEDC_CH, 128);   // 50% duty cycle
}

void fanInit() {
    applyFan();
}

void fanSet(bool on) {
    fanState = on;
    if (buzzerUntil == 0)   // el buzzer tiene prioridad mientras suena
        applyFan();
}

bool fanGet() {
    return fanState;
}

void buzzerBeep(uint32_t ms) {
    buzzerUntil = millis() + ms;
    applyBuzzer();
}

void fanBuzzerHandle() {
    if (buzzerUntil != 0 && millis() >= buzzerUntil) {
        buzzerUntil = 0;
        applyFan();   // restaura el estado del fan al terminar
    }
}
