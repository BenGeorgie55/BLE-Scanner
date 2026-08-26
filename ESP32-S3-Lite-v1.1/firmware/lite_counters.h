#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <stdint.h>

/*
 * ESP32 S3 Glasses Scanner Lite v1.1 - anonymous aggregate LED counters.
 *
 * PRIVACY BOUNDARY
 * ----------------
 * This module persists ONLY aggregate LED/status trigger totals and the current
 * five-boot counter-window number. It must never receive or store an observed
 * BLE address, name, hash, Company ID, UUID, manufacturer payload, product
 * label, confidence value tied to a device, or per-device history.
 *
 * Counter window:
 *   - totals accumulate across five firmware boots
 *   - on the sixth boot, the previous window is cleared and a new 1/5 window
 *     begins automatically
 *
 * Flash-wear policy:
 *   - counters increment in RAM immediately
 *   - startup/base-state changes are saved immediately (very low frequency)
 *   - planned privacy-recovery reboots force a save before ESP.restart()
 *   - active alert counters are checkpointed at most once every five minutes
 *
 * The one-second Serial report reads the RAM totals and performs NO NVS write.
 */

#define LITE_COUNTER_WINDOW_BOOTS       5U
#define LITE_COUNTER_NVS_VERSION        1U
#define LITE_COUNTER_CHECKPOINT_MS      (5UL * 60UL * 1000UL)

#if LITE_COUNTER_WINDOW_BOOTS == 0
#error "LITE_COUNTER_WINDOW_BOOTS must be greater than zero"
#endif

enum LiteCounterKind : uint8_t {
    LITE_COUNT_BLUE_BOOT = 0,
    LITE_COUNT_GREEN_PRIVATE_READY,
    LITE_COUNT_ORANGE_POSSIBLE,
    LITE_COUNT_RED_HIGH_GLASSES,
    LITE_COUNT_RED_CAMERA_AUDIO,
    LITE_COUNT_PURPLE_PRIVATE_FAIL,
    LITE_COUNT_PURPLE_PUBLIC_MODE
};

struct LiteCounterSnapshot {
    uint8_t  bootWindow;
    uint32_t blueBoot;
    uint32_t greenPrivateReady;
    uint32_t orangePossible;
    uint32_t redHighGlasses;
    uint32_t redCameraAudio;
    uint32_t purplePrivateFail;
    uint32_t purplePublicMode;
};

static Preferences liteCounterPrefs;
static bool liteCounterPrefsReady = false;
static bool liteCounterDirty = false;
static uint32_t liteCounterLastCheckpointMs = 0;
static portMUX_TYPE liteCounterMux = portMUX_INITIALIZER_UNLOCKED;
static LiteCounterSnapshot liteCounterState = {};

static inline void liteCounterSaturatingIncrement(uint32_t& value) {
    if (value != UINT32_MAX) ++value;
}

static inline LiteCounterSnapshot liteCountersSnapshot() {
    LiteCounterSnapshot copy;
    portENTER_CRITICAL(&liteCounterMux);
    copy = liteCounterState;
    portEXIT_CRITICAL(&liteCounterMux);
    return copy;
}

static inline bool liteCountersAreDirty() {
    bool dirty;
    portENTER_CRITICAL(&liteCounterMux);
    dirty = liteCounterDirty;
    portEXIT_CRITICAL(&liteCounterMux);
    return dirty;
}

static inline void liteCountersMarkDirty() {
    portENTER_CRITICAL(&liteCounterMux);
    liteCounterDirty = true;
    portEXIT_CRITICAL(&liteCounterMux);
}

static inline bool liteCountersPersistNow() {
    if (!liteCounterPrefsReady) return false;

    LiteCounterSnapshot snap;
    portENTER_CRITICAL(&liteCounterMux);
    snap = liteCounterState;
    // Clear before writing. If a BLE callback increments a counter after this
    // unlock, that callback sets dirty=true again and the new value will be
    // included in the next checkpoint.
    liteCounterDirty = false;
    portEXIT_CRITICAL(&liteCounterMux);

    bool ok = true;
    ok &= (liteCounterPrefs.putUChar("ver", LITE_COUNTER_NVS_VERSION) == 1);
    ok &= (liteCounterPrefs.putUChar("boots", snap.bootWindow) == 1);
    ok &= (liteCounterPrefs.putUInt("blue", snap.blueBoot) == sizeof(uint32_t));
    ok &= (liteCounterPrefs.putUInt("green", snap.greenPrivateReady) == sizeof(uint32_t));
    ok &= (liteCounterPrefs.putUInt("orange", snap.orangePossible) == sizeof(uint32_t));
    ok &= (liteCounterPrefs.putUInt("rhigh", snap.redHighGlasses) == sizeof(uint32_t));
    ok &= (liteCounterPrefs.putUInt("rcam", snap.redCameraAudio) == sizeof(uint32_t));
    ok &= (liteCounterPrefs.putUInt("pfail", snap.purplePrivateFail) == sizeof(uint32_t));
    ok &= (liteCounterPrefs.putUInt("pub", snap.purplePublicMode) == sizeof(uint32_t));

    liteCounterLastCheckpointMs = millis();

    if (!ok) {
        // Retry later rather than losing the dirty indication.
        liteCountersMarkDirty();
    }
    return ok;
}

static inline bool liteCountersBegin() {
    liteCounterPrefsReady = liteCounterPrefs.begin("s3litecnt", false);

    LiteCounterSnapshot loaded = {};
    uint8_t storedVersion = 0;

    if (liteCounterPrefsReady) {
        storedVersion = liteCounterPrefs.getUChar("ver", 0);
        if (storedVersion == LITE_COUNTER_NVS_VERSION) {
            loaded.bootWindow          = liteCounterPrefs.getUChar("boots", 0);
            loaded.blueBoot            = liteCounterPrefs.getUInt("blue", 0);
            loaded.greenPrivateReady   = liteCounterPrefs.getUInt("green", 0);
            loaded.orangePossible      = liteCounterPrefs.getUInt("orange", 0);
            loaded.redHighGlasses      = liteCounterPrefs.getUInt("rhigh", 0);
            loaded.redCameraAudio      = liteCounterPrefs.getUInt("rcam", 0);
            loaded.purplePrivateFail   = liteCounterPrefs.getUInt("pfail", 0);
            loaded.purplePublicMode    = liteCounterPrefs.getUInt("pub", 0);
        } else {
            // Unknown/old schema: clear only this counter namespace.
            liteCounterPrefs.clear();
            loaded = {};
        }
    }

    // A complete five-boot window is retained through boot 5. Boot 6 starts a
    // fresh window at 1/5 with all aggregate totals cleared.
    if (loaded.bootWindow >= LITE_COUNTER_WINDOW_BOOTS) {
        loaded = {};
    }
    loaded.bootWindow++;

    portENTER_CRITICAL(&liteCounterMux);
    liteCounterState = loaded;
    liteCounterDirty = true;
    portEXIT_CRITICAL(&liteCounterMux);

    // Save the new boot-window number immediately when NVS is available.
    if (liteCounterPrefsReady) liteCountersPersistNow();
    return liteCounterPrefsReady;
}

static inline void liteCountersIncrement(LiteCounterKind kind) {
    portENTER_CRITICAL(&liteCounterMux);

    switch (kind) {
        case LITE_COUNT_BLUE_BOOT:
            liteCounterSaturatingIncrement(liteCounterState.blueBoot);
            break;
        case LITE_COUNT_GREEN_PRIVATE_READY:
            liteCounterSaturatingIncrement(liteCounterState.greenPrivateReady);
            break;
        case LITE_COUNT_ORANGE_POSSIBLE:
            liteCounterSaturatingIncrement(liteCounterState.orangePossible);
            break;
        case LITE_COUNT_RED_HIGH_GLASSES:
            liteCounterSaturatingIncrement(liteCounterState.redHighGlasses);
            break;
        case LITE_COUNT_RED_CAMERA_AUDIO:
            liteCounterSaturatingIncrement(liteCounterState.redCameraAudio);
            break;
        case LITE_COUNT_PURPLE_PRIVATE_FAIL:
            liteCounterSaturatingIncrement(liteCounterState.purplePrivateFail);
            break;
        case LITE_COUNT_PURPLE_PUBLIC_MODE:
            liteCounterSaturatingIncrement(liteCounterState.purplePublicMode);
            break;
        default:
            break;
    }

    liteCounterDirty = true;
    portEXIT_CRITICAL(&liteCounterMux);
}

static inline void liteCountersCheckpointIfDue(uint32_t nowMs) {
    if (!liteCounterPrefsReady || !liteCountersAreDirty()) return;
    if ((uint32_t)(nowMs - liteCounterLastCheckpointMs) < LITE_COUNTER_CHECKPOINT_MS) return;
    liteCountersPersistNow();
}

static inline uint32_t liteCountersTotalAlerts(const LiteCounterSnapshot& s) {
    // Saturate rather than wrap if a unit is run for an extreme period.
    uint64_t total = (uint64_t)s.orangePossible +
                     (uint64_t)s.redHighGlasses +
                     (uint64_t)s.redCameraAudio;
    return total > UINT32_MAX ? UINT32_MAX : (uint32_t)total;
}

static inline void liteCountersPrintSerial(const char* modeText) {
    LiteCounterSnapshot s = liteCountersSnapshot();

    Serial.println("ESP32 S3 GLASSES SCANNER LITE v1.1");
    Serial.printf("MODE: %s\n", modeText ? modeText : "STARTING");
    Serial.printf("COUNTER WINDOW: %u / %u BOOTS\n",
                  (unsigned)s.bootWindow,
                  (unsigned)LITE_COUNTER_WINDOW_BOOTS);
    Serial.printf("BLUE_BOOT: %lu\n", (unsigned long)s.blueBoot);
    Serial.printf("GREEN_PRIVATE_READY: %lu\n", (unsigned long)s.greenPrivateReady);
    Serial.printf("ORANGE_POSSIBLE: %lu\n", (unsigned long)s.orangePossible);
    Serial.printf("RED_HIGH_GLASSES: %lu\n", (unsigned long)s.redHighGlasses);
    Serial.printf("RED_CAMERA_AUDIO: %lu\n", (unsigned long)s.redCameraAudio);
    Serial.printf("PURPLE_PRIVATE_FAIL: %lu\n", (unsigned long)s.purplePrivateFail);
    Serial.printf("PURPLE_PUBLIC_MODE: %lu\n", (unsigned long)s.purplePublicMode);
    Serial.printf("TOTAL_ALERT_TRIGGERS: %lu\n",
                  (unsigned long)liteCountersTotalAlerts(s));
    Serial.println();
}
