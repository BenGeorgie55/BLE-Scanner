#pragma once
#include <Arduino.h>
#include "esp32-hal-rgb-led.h"

/*
 * ESP32 S3 Light - onboard RGB LED controller.
 *
 * Consumer LED policy:
 *   - BLUE / GREEN / ORANGE / PURPLE are deliberately DIM.
 *   - RED urgent alerts bypass the dimmer and use full brightness.
 *   - RED urgent alerts are strobed non-blockingly by the main loop.
 *
 * Most ESP32-S3 DevKit-style boards expose RGB_BUILTIN. If your board does not,
 * GPIO48 is the default fallback; change STATUS_LED_PIN for your exact board.
 */

#ifndef STATUS_LED_PIN
  #ifdef RGB_BUILTIN
    #define STATUS_LED_PIN RGB_BUILTIN
  #else
    #define STATUS_LED_PIN 48
  #endif
#endif

// Dim brightness for all non-red status colours, 0..255.
#ifndef STATUS_LED_DIM_BRIGHTNESS
#define STATUS_LED_DIM_BRIGHTNESS 24
#endif

// Red urgent alert strobe: 250 ms ON + 250 ms OFF = about 2 flashes/second.
#ifndef STATUS_RED_STROBE_HALF_PERIOD_MS
#define STATUS_RED_STROBE_HALF_PERIOD_MS 250UL
#endif

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

static inline uint8_t liteScaleDimLed(uint8_t value) {
    return (uint8_t)(((uint16_t)value * STATUS_LED_DIM_BRIGHTNESS) / 255U);
}

static inline void liteLedRgbDim(uint8_t r, uint8_t g, uint8_t b) {
    rgbLedWrite(STATUS_LED_PIN,
                liteScaleDimLed(r),
                liteScaleDimLed(g),
                liteScaleDimLed(b));
}

static inline void liteLedOff()        { rgbLedWrite(STATUS_LED_PIN, 0, 0, 0); }
static inline void liteLedBlue()       { liteLedRgbDim(0, 0, 255); }
static inline void liteLedGreen()      { liteLedRgbDim(0, 255, 0); }
static inline void liteLedOrange()     { liteLedRgbDim(255, 120, 0); }
static inline void liteLedPurple()     { liteLedRgbDim(180, 0, 255); }

// Full-brightness RED deliberately bypasses STATUS_LED_DIM_BRIGHTNESS.
static inline void liteLedRedFull()    { rgbLedWrite(STATUS_LED_PIN, 255, 0, 0); }

// Non-blocking urgent red strobe. Call repeatedly from loop().
static inline void liteLedRedStrobe(uint32_t nowMs) {
    if (((nowMs / STATUS_RED_STROBE_HALF_PERIOD_MS) & 1U) == 0U) {
        liteLedRedFull();
    } else {
        liteLedOff();
    }
}

static inline void liteLedShowBase(LiteBaseLedMode mode) {
    switch (mode) {
        case LITE_LED_PRIVATE: liteLedGreen();   break;
        case LITE_LED_PUBLIC:  liteLedPurple();  break;
        case LITE_LED_FATAL:   liteLedRedFull(); break;
        case LITE_LED_BOOT:
        default:               liteLedBlue();    break;
    }
}
