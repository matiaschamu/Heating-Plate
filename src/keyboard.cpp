#include "keyboard.h"

#define KEY_POWER      22
#define KEY_FUNCION    23
#define DEBOUNCE_MS    50
#define BOTH_WINDOW_MS 150   // ventana para detectar pulsación simultánea

enum KeyPhase : uint8_t { IDLE, DEBOUNCING, CONFIRMED, ACTIVE };

struct Key {
    uint8_t           pin;
    volatile uint32_t isrTime;
    KeyPhase          phase;
    uint32_t          confirmedAt;
};

static Key keys[2] = {
    { KEY_POWER,   0, IDLE, 0 },
    { KEY_FUNCION, 0, IDLE, 0 },
};

static void IRAM_ATTR onKeyPower()   { keys[0].isrTime = millis(); }
static void IRAM_ATTR onKeyFuncion() { keys[1].isrTime = millis(); }

void keyboardInit() {
    for (auto& k : keys) pinMode(k.pin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(KEY_POWER),   onKeyPower,   FALLING);
    attachInterrupt(digitalPinToInterrupt(KEY_FUNCION), onKeyFuncion, FALLING);
}

KeyEvent keyboardGetEvent() {
    uint32_t now = millis();

    for (auto& k : keys) {
        switch (k.phase) {
            case IDLE:
                if (k.isrTime != 0) k.phase = DEBOUNCING;
                break;
            case DEBOUNCING:
                if (now - k.isrTime >= DEBOUNCE_MS) {
                    k.isrTime = 0;
                    if (digitalRead(k.pin) == LOW) {
                        k.phase       = CONFIRMED;
                        k.confirmedAt = now;
                    } else {
                        k.phase = IDLE;
                    }
                }
                break;
            case CONFIRMED:
            case ACTIVE:
                if (digitalRead(k.pin) == HIGH) {
                    k.phase   = IDLE;
                    k.isrTime = 0;
                }
                break;
        }
    }

    // Ambas CONFIRMED dentro de la ventana → simultánea
    if (keys[0].phase == CONFIRMED && keys[1].phase == CONFIRMED) {
        uint32_t t0   = keys[0].confirmedAt;
        uint32_t t1   = keys[1].confirmedAt;
        uint32_t diff = t0 > t1 ? t0 - t1 : t1 - t0;
        if (diff <= BOTH_WINDOW_MS) {
            keys[0].phase = ACTIVE;
            keys[1].phase = ACTIVE;
            return KEY_BOTH_PRESS;
        }
    }

    // Tecla individual: espera BOTH_WINDOW_MS antes de emitir
    for (int i = 0; i < 2; i++) {
        if (keys[i].phase == CONFIRMED && now - keys[i].confirmedAt > BOTH_WINDOW_MS) {
            keys[i].phase = ACTIVE;
            return i == 0 ? KEY_POWER_PRESS : KEY_FUNCION_PRESS;
        }
    }

    return KEY_NONE;
}
