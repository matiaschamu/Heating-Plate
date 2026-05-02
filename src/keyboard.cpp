#include "keyboard.h"

#define KEY_INC       22
#define KEY_DEC       23

#define DEBOUNCE_MS   50
#define HOLD_MS       1000
#define REPEAT_MS     100

enum KeyPhase : uint8_t { IDLE, DEBOUNCING, PRESSED, REPEATING };

struct Key {
    uint8_t  pin;
    int8_t   step;
    volatile uint32_t isrTime;
    KeyPhase  phase;
    uint32_t  pressedAt;
    uint32_t  lastRepeatAt;
};

static Key keys[2] = {
    { KEY_INC, +10, 0, IDLE, 0, 0 },
    { KEY_DEC, -10, 0, IDLE, 0, 0 },
};

static void IRAM_ATTR onKeyInc() { keys[0].isrTime = millis(); }
static void IRAM_ATTR onKeyDec() { keys[1].isrTime = millis(); }

void keyboardInit() {
    for (auto& k : keys) {
        pinMode(k.pin, INPUT_PULLUP);
    }
    attachInterrupt(digitalPinToInterrupt(KEY_INC), onKeyInc, FALLING);
    attachInterrupt(digitalPinToInterrupt(KEY_DEC), onKeyDec, FALLING);
}

int8_t keyboardGetDelta() {
    int8_t   delta = 0;
    uint32_t now   = millis();

    for (auto& k : keys) {
        switch (k.phase) {

            case IDLE:
                if (k.isrTime != 0)
                    k.phase = DEBOUNCING;
                break;

            case DEBOUNCING:
                if (now - k.isrTime >= DEBOUNCE_MS) {
                    if (digitalRead(k.pin) == LOW) {
                        k.phase     = PRESSED;
                        k.pressedAt = now;
                        delta      += k.step;   // disparo inicial
                    } else {
                        k.phase = IDLE;         // era ruido
                    }
                    k.isrTime = 0;
                }
                break;

            case PRESSED:
                if (digitalRead(k.pin) == HIGH) {
                    k.phase   = IDLE;
                    k.isrTime = 0;
                } else if (now - k.pressedAt >= HOLD_MS) {
                    k.phase        = REPEATING;
                    k.lastRepeatAt = now;
                    delta         += k.step;    // primer repetición
                }
                break;

            case REPEATING:
                if (digitalRead(k.pin) == HIGH) {
                    k.phase   = IDLE;
                    k.isrTime = 0;
                } else if (now - k.lastRepeatAt >= REPEAT_MS) {
                    k.lastRepeatAt = now;
                    delta         += k.step;    // repetición rápida
                }
                break;
        }
    }

    return delta;
}
