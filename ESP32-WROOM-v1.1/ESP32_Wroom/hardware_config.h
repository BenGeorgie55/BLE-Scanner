#pragma once

/*
 * ESP32 Wroom — NeoPixel-only hardware config
 * Target: classic ESP32-WROOM / ESP32 Dev Module
 */

#define STATUS_NEOPIXEL_PIN       4
#define STATUS_NEOPIXEL_COUNT     1

// Non-red colours are intentionally dim. Urgent red remains full brightness.
#define STATUS_LED_DIM_BRIGHTNESS 24
#define STATUS_RED_STROBE_HALF_PERIOD_MS 250UL
