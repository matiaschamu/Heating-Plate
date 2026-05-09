#include "encoder.h"

#define ENC_A 21
#define ENC_B 16

static volatile int8_t encDelta = 0;
static volatile uint32_t pendingAt = 0; // micros() del último flanco, 0 = sin pendiente

static void IRAM_ATTR onFallA()
{
    pendingAt = micros(); // solo apunta el momento; no bloquea nada
}

void encoderInit()
{
    pinMode(ENC_A, INPUT);
    pinMode(ENC_B, INPUT);
    attachInterrupt(digitalPinToInterrupt(ENC_A), onFallA, FALLING);
}

int8_t encoderGetDelta()
{
    // Procesar flanco pendiente si ya pasaron 2 ms (señal asentada)
    uint32_t t = pendingAt;
    if (t != 0 && micros() - t >= 2000)
    {
        pendingAt = 0;
        if (digitalRead(ENC_A) == LOW)
        { // sigue bajo → flanco real
            if (digitalRead(ENC_B) == HIGH)
            {
                encDelta--;
                Serial.print("Baja");
            }
            else
            {
                encDelta++;
                Serial.print("Sube");
            }
        }
        // si está HIGH → era ruido, se descarta sin haber esperado nada en la ISR
    }

    noInterrupts();
    int8_t d = encDelta;
    encDelta = 0;
    interrupts();
    return d;
}
