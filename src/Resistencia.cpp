#include "Resistencia.h"
#include "driver/timer.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define TRIAC_PIN        4
#define ZEROCROSS_PIN    17
#define TRIAC_PULSE_HIGH 1000
#define TRIAC_PULSE_LOW   100
#define HALF_CYCLE_US   10000

#define HW_TIMER_GROUP   TIMER_GROUP_0
#define HW_TIMER_IDX     TIMER_0
#define HW_TIMER_DIVIDER 80

static volatile uint8_t resistenciaPower = 0;

static inline uint32_t pulseDuration() {
    return resistenciaPower >= 50 ? TRIAC_PULSE_HIGH : TRIAC_PULSE_LOW;
}

static bool IRAM_ATTR timerISR(void*) {
    timer_pause(HW_TIMER_GROUP, HW_TIMER_IDX);

    gpio_set_level((gpio_num_t)TRIAC_PIN, 1);
    delayMicroseconds(pulseDuration());
    gpio_set_level((gpio_num_t)TRIAC_PIN, 0);

    gpio_intr_enable((gpio_num_t)ZEROCROSS_PIN);

    return false;
}

static void IRAM_ATTR onZeroCross() {
    int64_t t0 = esp_timer_get_time();

    if (resistenciaPower == 0) return;

    if (resistenciaPower == 100) {
        gpio_set_level((gpio_num_t)TRIAC_PIN, 1);
        delayMicroseconds(pulseDuration());
        gpio_set_level((gpio_num_t)TRIAC_PIN, 0);
        return;
    }

    gpio_intr_disable((gpio_num_t)ZEROCROSS_PIN);

    uint32_t nominalUs   = (uint32_t)(100 - resistenciaPower) * (HALF_CYCLE_US / 100);
    int64_t  elapsed     = esp_timer_get_time() - t0;
    uint32_t correctedUs = (elapsed < nominalUs) ? (uint32_t)(nominalUs - elapsed) : 50;

    timer_set_counter_value(HW_TIMER_GROUP, HW_TIMER_IDX, 0ULL);
    timer_set_alarm_value(HW_TIMER_GROUP, HW_TIMER_IDX, correctedUs);
    timer_set_alarm(HW_TIMER_GROUP, HW_TIMER_IDX, TIMER_ALARM_EN);
    timer_start(HW_TIMER_GROUP, HW_TIMER_IDX);
}

void resistenciaInit() {
    gpio_reset_pin((gpio_num_t)TRIAC_PIN);
    gpio_set_direction((gpio_num_t)TRIAC_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)TRIAC_PIN, 0);

    gpio_reset_pin((gpio_num_t)ZEROCROSS_PIN);
    gpio_set_direction((gpio_num_t)ZEROCROSS_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode((gpio_num_t)ZEROCROSS_PIN, GPIO_PULLUP_ONLY);

    timer_config_t cfg = {};
    cfg.alarm_en    = TIMER_ALARM_DIS;
    cfg.counter_en  = TIMER_PAUSE;
    cfg.intr_type   = TIMER_INTR_LEVEL;
    cfg.counter_dir = TIMER_COUNT_UP;
    cfg.auto_reload = TIMER_AUTORELOAD_DIS;
    cfg.divider     = HW_TIMER_DIVIDER;
    timer_init(HW_TIMER_GROUP, HW_TIMER_IDX, &cfg);
    timer_set_counter_value(HW_TIMER_GROUP, HW_TIMER_IDX, 0ULL);
    timer_enable_intr(HW_TIMER_GROUP, HW_TIMER_IDX);
    timer_isr_callback_add(HW_TIMER_GROUP, HW_TIMER_IDX, timerISR, nullptr, 0);

    attachInterrupt(digitalPinToInterrupt(ZEROCROSS_PIN), onZeroCross, CHANGE);
}

void resistenciaSet(uint8_t power) {
    resistenciaPower = power > 100 ? 100 : power;
}

uint8_t resistenciaGet() {
    return resistenciaPower;
}
