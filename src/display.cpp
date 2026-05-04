#include "display.h"

// ── Pines ─────────────────────────────────────────────────────────────────────
// Comunes: seleccionan el dígito activo (dígito 1 = izquierda)
static const uint8_t COM[4] = {33, 26, 14, 13};

// Pines de segmento — la dirección determina qué segmento:
//   pin(-) con COM(+)  →  A  B  C  D      índice: 0  1  2  3
//   pin(+) con COM(-)  →  E  F  G  —      índice: 0  1  2  —
static const uint8_t SEG[4] = {32, 25, 27, 12};

// Elementos especiales (pares fijos, no dependen del dígito activo)
//   Colon :          →  GPIO12(+) / GPIO13(-)
//   Punto decimal D2 →  GPIO12(+) / GPIO26(-)
//   LED Power        →  GPIO33(+) / GPIO26(-)
//   LED Temperatura  →  GPIO26(+) / GPIO33(-)
//   LED Timer        →  GPIO13(+) / GPIO26(-)

// ── Codificación de segmentos ─────────────────────────────────────────────────
#define SEG_A  (1 << 0)
#define SEG_B  (1 << 1)
#define SEG_C  (1 << 2)
#define SEG_D  (1 << 3)
#define SEG_E  (1 << 4)
#define SEG_F  (1 << 5)
#define SEG_G  (1 << 6)

static const uint8_t FONT[10] = {
    SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F,           // 0
    SEG_B|SEG_C,                                      // 1
    SEG_A|SEG_B|SEG_D|SEG_E|SEG_G,                  // 2
    SEG_A|SEG_B|SEG_C|SEG_D|SEG_G,                  // 3
    SEG_B|SEG_C|SEG_F|SEG_G,                         // 4
    SEG_A|SEG_C|SEG_D|SEG_F|SEG_G,                  // 5
    SEG_A|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G,            // 6
    SEG_A|SEG_B|SEG_C,                               // 7
    SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G,     // 8
    SEG_A|SEG_B|SEG_C|SEG_D|SEG_F|SEG_G,            // 9
};

// ── Estado del display ────────────────────────────────────────────────────────
static uint8_t dispSegs[4] = {0, 0, 0, 0};

bool showColon = false;
bool showDP2   = false;
bool ledPower  = false;
bool ledTemp   = false;
bool ledTimer  = false;

// ── Helpers de bajo nivel ─────────────────────────────────────────────────────
static void allHiZ() {
    for (uint8_t i = 0; i < 4; i++) {
        pinMode(COM[i], INPUT);
        pinMode(SEG[i], INPUT);
    }
}

static void drivePair(uint8_t anodePin, uint8_t cathodePin) {
    allHiZ();
    pinMode(anodePin,   OUTPUT); digitalWrite(anodePin,   HIGH);
    pinMode(cathodePin, OUTPUT); digitalWrite(cathodePin, LOW);
}

// ── Refresh ───────────────────────────────────────────────────────────────────
// 13 slots × 500 µs = ~154 Hz de refresco total.
// Cada dígito ocupa 2 slots:
//   Fase 0 — COM es ánodo (+):  enciende A, B, C, D
//   Fase 1 — COM es cátodo (-): enciende E, F, G
// Slots 8-12: colon, DP, LEDs especiales.
void refreshDisplay() {
    static uint8_t  slot   = 0;
    static uint32_t lastUs = 0;

    uint32_t now = micros();
    if (now - lastUs < 500) return;
    lastUs = now;

    allHiZ();

    if (slot < 8) {
        uint8_t d     = slot >> 1;
        uint8_t phase = slot & 1;
        uint8_t segs  = dispSegs[d];

        if (phase == 0) {
            static const uint8_t cathBits[4] = {SEG_A, SEG_B, SEG_C, SEG_D};
            pinMode(COM[d], OUTPUT); digitalWrite(COM[d], HIGH);
            for (uint8_t s = 0; s < 4; s++) {
                if (segs & cathBits[s]) {
                    pinMode(SEG[s], OUTPUT); digitalWrite(SEG[s], LOW);
                }
            }
        } else {
            static const uint8_t anodBits[3] = {SEG_E, SEG_F, SEG_G};
            pinMode(COM[d], OUTPUT); digitalWrite(COM[d], LOW);
            for (uint8_t s = 0; s < 3; s++) {
                if (segs & anodBits[s]) {
                    pinMode(SEG[s], OUTPUT); digitalWrite(SEG[s], HIGH);
                }
            }
        }
    } else if (slot == 8  && showColon) { drivePair(12, 13); }
    else if   (slot == 9  && showDP2)   { drivePair(12, 26); }
    else if   (slot == 10 && ledPower)  { drivePair(33, 26); }
    else if   (slot == 11 && ledTemp)   { drivePair(26, 33); }
    else if   (slot == 12 && ledTimer)  { drivePair(13, 26); }

    if (++slot > 12) slot = 0;
}

// ── API pública ───────────────────────────────────────────────────────────────
void displayInit() {
    allHiZ();
}

void displayNumber(uint16_t num) {
    dispSegs[0] = FONT[(num / 1000) % 10];
    dispSegs[1] = FONT[(num / 100)  % 10];
    dispSegs[2] = FONT[(num / 10)   % 10];
    dispSegs[3] = FONT[ num         % 10];
}

void displayTime(uint8_t h, uint8_t m) {
    dispSegs[0] = FONT[h / 10];
    dispSegs[1] = FONT[h % 10];
    dispSegs[2] = FONT[m / 10];
    dispSegs[3] = FONT[m % 10];
    showColon   = true;
}

void displayOff() {
    for (uint8_t i = 0; i < 4; i++) dispSegs[i] = 0;
    showColon = showDP2 = false;
    allHiZ();
}

void displayDashes() {
    for (uint8_t i = 0; i < 4; i++) dispSegs[i] = SEG_G;
    showColon = false;
}

void displayCoolingAnimation() {
    // Tres frames: superior (A) → medio (G) → inferior (D), 400 ms cada uno
    static const uint8_t frames[3] = { SEG_A, SEG_G, SEG_D };
    uint8_t frame = (millis() / 400) % 3;
    for (uint8_t i = 0; i < 4; i++) dispSegs[i] = frames[frame];
    showColon = false;
}
