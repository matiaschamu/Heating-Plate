#include "display.h"
#include "driver/timer.h"

// ── Pines ─────────────────────────────────────────────────────────────────────
// Comunes: seleccionan el dígito activo (dígito 1 = izquierda)
static uint8_t COM[4] = {33, 26, 14, 13};

// Pines de segmento — la dirección determina qué segmento:
//   pin(-) con COM(+)  →  A  B  C  D      índice: 0  1  2  3
//   pin(+) con COM(-)  →  E  F  G  —      índice: 0  1  2  —
static uint8_t SEG[4] = {32, 25, 27, 12};

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
static volatile uint8_t dispSegs[4] = {0, 0, 0, 0};

volatile bool showColon = false;
volatile bool showDP2   = false;
volatile bool ledPower  = false;
volatile bool ledTemp   = false;
volatile bool ledTimer  = false;

// ── Macros de GPIO ultrarrápidos y seguros para ISR ───────────────────────────
static inline void IRAM_ATTR pinHiZ(uint8_t pin) {
    if (pin < 32) GPIO.enable_w1tc = (1UL << pin);
    else GPIO.enable1_w1tc.val = (1UL << (pin - 32));
}

static inline void IRAM_ATTR pinHigh(uint8_t pin) {
    if (pin < 32) {
        GPIO.out_w1ts = (1UL << pin);
        GPIO.enable_w1ts = (1UL << pin);
    } else {
        GPIO.out1_w1ts.val = (1UL << (pin - 32));
        GPIO.enable1_w1ts.val = (1UL << (pin - 32));
    }
}

static inline void IRAM_ATTR pinLow(uint8_t pin) {
    if (pin < 32) {
        GPIO.out_w1tc = (1UL << pin);
        GPIO.enable_w1ts = (1UL << pin);
    } else {
        GPIO.out1_w1tc.val = (1UL << (pin - 32));
        GPIO.enable1_w1ts.val = (1UL << (pin - 32));
    }
}

// ── Helpers de bajo nivel ─────────────────────────────────────────────────────
static void IRAM_ATTR allHiZ() {
    for (uint8_t i = 0; i < 4; i++) {
        pinHiZ(COM[i]);
        pinHiZ(SEG[i]);
    }
}

static void IRAM_ATTR drivePair(uint8_t anodePin, uint8_t cathodePin) {
    allHiZ();
    pinHigh(anodePin);
    pinLow(cathodePin);
}

// ── Refresh ───────────────────────────────────────────────────────────────────
// 13 slots × 500 µs = ~154 Hz de refresco total.
// Cada dígito ocupa 2 slots:
//   Fase 0 — COM es ánodo (+):  enciende A, B, C, D
//   Fase 1 — COM es cátodo (-): enciende E, F, G
// Slots 8-12: colon, DP, LEDs especiales.
//
// Tablas movidas a scope de archivo para que la ISR no dependa
// de los guardas C++11 de inicialización de estáticas locales (no IRAM).
static const uint8_t cathBits[4] = {SEG_A, SEG_B, SEG_C, SEG_D};
static const uint8_t anodBits[3] = {SEG_E, SEG_F, SEG_G};

static bool IRAM_ATTR displayTimerCallback(void*) {
    static uint8_t slot = 0;

    allHiZ();

    if (slot < 8) {
        uint8_t d     = slot >> 1;
        uint8_t phase = slot & 1;
        uint8_t segs  = dispSegs[d];

        if (phase == 0) {
            pinHigh(COM[d]);
            for (uint8_t s = 0; s < 4; s++) {
                if (segs & cathBits[s]) {
                    pinLow(SEG[s]);
                }
            }
        } else {
            pinLow(COM[d]);
            for (uint8_t s = 0; s < 3; s++) {
                if (segs & anodBits[s]) {
                    pinHigh(SEG[s]);
                }
            }
        }
    } else if (slot == 8  && showColon) { drivePair(12, 13); }
    else if   (slot == 9  && showDP2)   { drivePair(12, 26); }
    else if   (slot == 10 && ledPower)  { drivePair(33, 26); }
    else if   (slot == 11 && ledTemp)   { drivePair(26, 33); }
    else if   (slot == 12 && ledTimer)  { drivePair(13, 26); }

    if (++slot > 12) slot = 0;
    return false;   // sin yield a tarea de mayor prioridad
}

// ── API pública ───────────────────────────────────────────────────────────────
//
// Usa grupo 0 / timer 1 (Resistencia.cpp ocupa grupo 0 / timer 0).
// Misma API IDF que Resistencia para evitar mezclar wrappers Arduino + IDF
// sobre el mismo periférico.
#define DISP_TIMER_GROUP   TIMER_GROUP_0
#define DISP_TIMER_IDX     TIMER_1
#define DISP_TIMER_DIVIDER 80          // 80 MHz / 80 = 1 MHz → 1 µs por tick
#define DISP_TIMER_PERIOD  500         // µs entre slots → ~154 Hz de refresco

void displayInit() {
    // Configuración inicial del GPIO mux (una sola vez)
    for (uint8_t i = 0; i < 4; i++) {
        pinMode(COM[i], OUTPUT);
        pinMode(SEG[i], OUTPUT);
    }
    allHiZ();

    timer_config_t cfg = {};
    cfg.alarm_en    = TIMER_ALARM_EN;
    cfg.counter_en  = TIMER_PAUSE;
    cfg.intr_type   = TIMER_INTR_LEVEL;
    cfg.counter_dir = TIMER_COUNT_UP;
    cfg.auto_reload = TIMER_AUTORELOAD_EN;
    cfg.divider     = DISP_TIMER_DIVIDER;

    timer_init(DISP_TIMER_GROUP, DISP_TIMER_IDX, &cfg);
    timer_set_counter_value(DISP_TIMER_GROUP, DISP_TIMER_IDX, 0ULL);
    timer_set_alarm_value(DISP_TIMER_GROUP, DISP_TIMER_IDX, DISP_TIMER_PERIOD);
    timer_enable_intr(DISP_TIMER_GROUP, DISP_TIMER_IDX);
    timer_isr_callback_add(DISP_TIMER_GROUP, DISP_TIMER_IDX, displayTimerCallback, nullptr, 0);
    timer_start(DISP_TIMER_GROUP, DISP_TIMER_IDX);
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
