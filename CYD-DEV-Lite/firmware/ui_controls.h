#pragma once

/*
 * ============================================================================
 * CYD v6.2 — USER-EDITABLE UI / BOOT BUTTON CONTROLS
 * ============================================================================
 *
 * Keep the on-screen SYS STAT instructions and these thresholds synchronized.
 * The main sketch uses these values for both gesture recognition and the text
 * shown to the operator.
 *
 * Scanner page:
 *   short BOOT       -> SYS STAT
 *   hold 2 sec       -> manual 5-before + 5-after capture
 *
 * SYS STAT page:
 *   short BOOT       -> Scanner
 *   2x short BOOT    -> Sentry Mode
 *   hold 4 to <6 sec -> Self Test (starts on release)
 *   hold >=6 sec     -> SD safe eject / remount
 *
 * Sentry:
 *   2x short BOOT    -> exit Sentry Mode
 * ============================================================================
 */

#define PAGE_BUTTON_PIN                    0
#define PAGE_BUTTON_DEBOUNCE_MS          250UL
#define PAGE_BUTTON_DOUBLE_PRESS_MS      500UL

// A press shorter than this is considered a short BOOT press/tap.
#define PAGE_BUTTON_SHORT_MAX_MS        2000UL

// Scanner manual context capture begins while BOOT remains held this long.
#define SCANNER_MANUAL_LOG_HOLD_MS      2000UL

// SYS STAT Self Test starts on RELEASE after a hold in [4 sec, 6 sec).
#define SYS_STAT_SELF_TEST_HOLD_MS      4000UL

// SYS STAT SD safe eject/remount fires immediately once this duration is met.
#define SYS_STAT_SD_EJECT_HOLD_MS       6000UL

#if PAGE_BUTTON_DOUBLE_PRESS_MS >= PAGE_BUTTON_SHORT_MAX_MS
#error "Double-press window must be shorter than the short-press maximum"
#endif

#if SYS_STAT_SELF_TEST_HOLD_MS <= PAGE_BUTTON_SHORT_MAX_MS
#error "SYS STAT Self Test hold must be longer than a short BOOT press"
#endif

#if SYS_STAT_SD_EJECT_HOLD_MS <= SYS_STAT_SELF_TEST_HOLD_MS
#error "SYS STAT SD eject hold must be longer than the Self Test hold"
#endif
