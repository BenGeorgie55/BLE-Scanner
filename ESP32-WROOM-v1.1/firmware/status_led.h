#pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "hardware_config.h"

#if STATUS_LED_DIM_BRIGHTNESS > 255
#error "STATUS_LED_DIM_BRIGHTNESS must be 0..255"
#endif
#if STATUS_RED_STROBE_HALF_PERIOD_MS == 0
#error "STATUS_RED_STROBE_HALF_PERIOD_MS must be greater than zero"
#endif

enum LiteBaseLedMode : uint8_t {
    LITE_LED_BOOT = 0,
    LITE_LED_PRIVATE,
    LITE_LED_PUBLIC,
    LITE_LED_FATAL
};

static Adafruit_NeoPixel liteStatusPixel(
    STATUS_NEOPIXEL_COUNT,
    STATUS_NEOPIXEL_PIN,
    NEO_GRB + NEO_KHZ800
);

static inline uint8_t liteScaleDimLed(uint8_t value) {
    return (uint8_t)(((uint16_t)value * STATUS_LED_DIM_BRIGHTNESS) / 255U);
}
static inline void liteLedBegin() {
    liteStatusPixel.begin();
    liteStatusPixel.clear();
    liteStatusPixel.show();
}
static inline void liteLedWrite(uint8_t r, uint8_t g, uint8_t b) {
    liteStatusPixel.setPixelColor(0, liteStatusPixel.Color(r, g, b));
    liteStatusPixel.show();
}
static inline void liteLedRgbDim(uint8_t r, uint8_t g, uint8_t b) {
    liteLedWrite(liteScaleDimLed(r), liteScaleDimLed(g), liteScaleDimLed(b));
}
static inline void liteLedOff()     { liteLedWrite(0, 0, 0); }
static inline void liteLedBlue()    { liteLedRgbDim(0, 0, 255); }
static inline void liteLedGreen()   { liteLedRgbDim(0, 255, 0); }
static inline void liteLedOrange()  { liteLedRgbDim(255, 120, 0); }
static inline void liteLedPurple()  { liteLedRgbDim(180, 0, 255); }
static inline void liteLedRedFull() { liteLedWrite(255, 0, 0); }

static inline void liteLedRedStrobe(uint32_t nowMs) {
    if (((nowMs / STATUS_RED_STROBE_HALF_PERIOD_MS) & 1U) == 0U) liteLedRedFull();
    else liteLedOff();
}
static inline void liteLedShowBase(LiteBaseLedMode mode) {
    switch (mode) {
        case LITE_LED_PRIVATE: liteLedGreen(); break;
        case LITE_LED_PUBLIC: liteLedPurple(); break;
        case LITE_LED_FATAL: liteLedRedFull(); break;
        case LITE_LED_BOOT:
        default: liteLedBlue(); break;
    }
}
