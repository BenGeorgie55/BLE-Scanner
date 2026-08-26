#pragma once

/*
 * ============================================================================
 * CYD v6.2 — USER-EDITABLE SENTRY MODE CONFIGURATION
 * ============================================================================
 *
 * Edit the values in the USER SETTINGS sections below. The main .ino should
 * not need to be changed for normal Sentry tuning.
 *
 * IMPORTANT:
 * - The normal wake sample and ACTIVE learning window deliberately use the
 *   SAME measurement duration. This keeps baseline comparisons valid.
 * - SENTRY_ACTIVE_MINUTES controls the fixed learning period. Activity can
 *   NEVER extend that timer; this file changes only its configured duration.
 * - Brightness values are logical 0..255 values.
 * ============================================================================
 */

// ---------------------------------------------------------------------------
// USER SETTINGS — SENTRY TIMING
// ---------------------------------------------------------------------------
// Time between ordinary Sentry activity checks.
#define SENTRY_SLEEP_MINUTES                 30UL

// Length of one BLE activity measurement window.
// Used BOTH for the ordinary wake sample and each ACTIVE-learning bucket.
#define SENTRY_MEASUREMENT_WINDOW_SECONDS    10UL

// Fixed continuous-learning period after a significant activity rise.
// The timer starts once and is never extended by later activity.
#define SENTRY_ACTIVE_MINUTES                60UL

// How long the Sentry dashboard stays visible after a touch.
#define SENTRY_SCREEN_WAKE_SECONDS            5UL

// ---------------------------------------------------------------------------
// USER SETTINGS — ACTIVITY TRIGGER
// ---------------------------------------------------------------------------
// Percentage of baseline required for the percentage-based trigger.
// 150 = 1.50 x baseline, 175 = 1.75 x baseline, 200 = 2.00 x baseline.
#define SENTRY_TRIGGER_PERCENT              150UL

// Absolute increase required by the second trigger calculation.
// Actual trigger threshold = max(percent threshold, baseline + this value).
#define SENTRY_TRIGGER_MIN_INCREASE           5U

// ---------------------------------------------------------------------------
// USER SETTINGS — DISPLAY / LED
// ---------------------------------------------------------------------------
// XPT2046 touch interrupt pin on the standard ESP32-2432S028R CYD.
#define TOUCH_IRQ_PIN                         36

// Duration of the ENTERING SENTRY MODE transition.
#define SENTRY_ENTER_NOTICE_MS               900UL

// TFT backlight during the entry transition only. 0 = off, 255 = full.
#define SENTRY_ENTER_BACKLIGHT_PWM            64

// Dim red rear RGB LED level while Sentry Mode is active. 0..255.
#define SENTRY_RED_BRIGHTNESS                  4

// Sentry dashboard refresh interval. Wake/legal gates use the physical BOOT button.
#define SENTRY_DASH_REFRESH_MS               250UL

// ---------------------------------------------------------------------------
// ADVANCED USER SETTINGS — MEMORY / DEVICE CAPACITY
// ---------------------------------------------------------------------------
// Pseudonymous identities retained only for Sentry session/dashboard stats.
// This does not cap the 10-second activity measurement window below.
#define MAX_SENTRY_UNIQUE_DEVICES             100

// Pseudonymous identities retained inside one activity measurement window.
// Keep this larger so baseline/trigger measurements remain accurate in busy
// BLE environments even though the dashboard/session table is smaller.
#define MAX_SENTRY_WINDOW_UNIQUE_DEVICES      400

// ---------------------------------------------------------------------------
// DERIVED VALUES — DO NOT NORMALLY EDIT
// ---------------------------------------------------------------------------
#if SENTRY_SLEEP_MINUTES == 0
#error "SENTRY_SLEEP_MINUTES must be greater than zero"
#endif

#if SENTRY_MEASUREMENT_WINDOW_SECONDS == 0
#error "SENTRY_MEASUREMENT_WINDOW_SECONDS must be greater than zero"
#endif

#if SENTRY_ACTIVE_MINUTES == 0
#error "SENTRY_ACTIVE_MINUTES must be greater than zero"
#endif

#if SENTRY_SCREEN_WAKE_SECONDS == 0
#error "SENTRY_SCREEN_WAKE_SECONDS must be greater than zero"
#endif

#if SENTRY_TRIGGER_PERCENT < 100
#error "SENTRY_TRIGGER_PERCENT must be at least 100"
#endif

#if SENTRY_ENTER_BACKLIGHT_PWM > 255
#error "SENTRY_ENTER_BACKLIGHT_PWM must be 0..255"
#endif

#if SENTRY_RED_BRIGHTNESS > 255
#error "SENTRY_RED_BRIGHTNESS must be 0..255"
#endif

#if (((SENTRY_ACTIVE_MINUTES * 60UL) % SENTRY_MEASUREMENT_WINDOW_SECONDS) != 0)
#error "Sentry ACTIVE duration must divide evenly into measurement windows"
#endif

#define SENTRY_SLEEP_MS \
    (SENTRY_SLEEP_MINUTES * 60UL * 1000UL)

#define SENTRY_SAMPLE_MS \
    (SENTRY_MEASUREMENT_WINDOW_SECONDS * 1000UL)

#define SENTRY_ACTIVITY_WINDOW_MS \
    SENTRY_SAMPLE_MS

#define SENTRY_ACTIVE_MS \
    (SENTRY_ACTIVE_MINUTES * 60UL * 1000UL)

#define SENTRY_WAKE_MS \
    (SENTRY_SCREEN_WAKE_SECONDS * 1000UL)

#define SENTRY_ACTIVE_SAMPLE_CAPACITY \
    ((SENTRY_ACTIVE_MINUTES * 60UL) / SENTRY_MEASUREMENT_WINDOW_SECONDS)


// Default v6.2 values resolve to:
//   sleep: 30 minutes
//   measurement window: 10 seconds
//   ACTIVE learning: 60 minutes
//   screen wake: 5 seconds
//   trigger: max(1.50 x baseline, baseline + 5)
//   ACTIVE learning windows: 360
