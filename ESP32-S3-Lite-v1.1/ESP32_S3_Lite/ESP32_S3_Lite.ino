/*
 * ============================================================================
 * ESP32 S3 Lite
 * ==========================================================================
 * ============================================================================
 */

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <esp_system.h>
#include <esp_attr.h>
#include <string.h>

#include "status_led.h"
#include "ble_private_scan.h"
#include "lite_counters.h"
#include "lite_persistent_log.h"

#define SERIAL_BAUD                115200
#define BLE_SCAN_TIME              2
#define BLE_SCAN_CYCLE_MS          5000UL
#define BLE_SCAN_INTERVAL_MS       100
#define BLE_SCAN_WINDOW_MS         95

#define LOGGING_CONFIDENCE_MIN     40
#define POSSIBLE_CONFIDENCE_MIN    60
#define HIGH_ALERT_CONFIDENCE      97
#define ALERT_HOLD_MS              10000UL

#define PRIVATE_START_MAX_FAILURES 3
#define PRIVATE_FAILURE_FLASH_MS   3000UL
#define PRIVATE_FAILURE_FLASH_STEP 200UL

#define TIER_HIGH                  0
#define TIER_MEDIUM                1
#define TIER_LOW                   2

#include "high_confidence_matches.h"
#include "medium_confidence_matches.h"
#include "low_confidence_matches.h"
#include "false_positives.h"
#include "known_device_types.h"
#include "known_devices.h"
#include "camera_audio_matches.h"

struct DetectionResult {
    bool        matched;
    bool        detected;
    const char* company;
    const char* product;
    const char* reason;
    bool        hasCamera;
    bool        cameraKnown;
    uint8_t     tier;
    uint8_t     confidence;
    const GlassesConfidenceRule* matchedRule;
};

struct CameraAudioResult {
    bool        matched;
    const char* label;
    CameraAudioCategory category;
    const char* reason;
    uint8_t     confidence;
    const CameraAudioRule* matchedRule;
};

enum LiteAlertKind : uint8_t {
    ALERT_NONE = 0,
    ALERT_GLASSES_POSSIBLE,
    ALERT_CAMERA_AUDIO,
    ALERT_GLASSES_HIGH
};

struct LiteAlertState {
    LiteAlertKind kind;
    uint8_t confidence;
    uint32_t expiresAt;
};

BLEScan* pBLEScan = nullptr;
volatile bool scanFinishedFlag = false;

bool privateModeActive = false;
bool publicFallbackActive = false;
esp_bd_addr_t scannerPrivateAddress = {0};
uint32_t scannerPrivateAddressNextRotateMs = 0;
uint32_t scannerPrivateAddressIntervalMs = 0;

uint32_t lastScanStartMs = 0;
bool firstScanPending = true;
uint32_t lastCounterPrintMs = 0;
bool logCommandPending = false;
String serialCommandBuffer;

portMUX_TYPE alertMux = portMUX_INITIALIZER_UNLOCKED;
LiteAlertState alertState = { ALERT_NONE, 0, 0 };

RTC_NOINIT_ATTR uint32_t liteRetryMagic;
RTC_NOINIT_ATTR uint8_t litePrivateFailureCount;
static constexpr uint32_t LITE_RETRY_MAGIC = 0x53334711UL; // "S3G1.1"

// -----------------------------------------------------------------------------
// Small helpers
// -----------------------------------------------------------------------------

bool containsIgnoreCase(const String& value, const char* needle) {
    if (needle == nullptr || !value.length()) return false;
    String a = value;
    String b = String(needle);
    a.toLowerCase();
    b.toLowerCase();
    return a.indexOf(b) >= 0;
}

bool startsWithIgnoreCase(const String& value, const char* prefix) {
    if (prefix == nullptr || !value.length()) return false;
    String a = value;
    String b = String(prefix);
    a.toLowerCase();
    b.toLowerCase();
    return a.startsWith(b);
}

String bytesToHex(const uint8_t* data, size_t len) {
    static const char HEX_DIGITS[] = "0123456789ABCDEF";
    String out;
    out.reserve(len * 2);
    for (size_t i = 0; i < len; ++i) {
        out += HEX_DIGITS[(data[i] >> 4) & 0x0F];
        out += HEX_DIGITS[data[i] & 0x0F];
    }
    return out;
}

String getManufacturerHex(BLEAdvertisedDevice& device) {
    if (!device.haveManufacturerData()) return "";
    String raw = device.getManufacturerData();
    if (raw.length() == 0) return "";
    return bytesToHex((const uint8_t*)raw.c_str(), raw.length());
}

uint16_t getCompanyId(BLEAdvertisedDevice& device) {
    if (!device.haveManufacturerData()) return 0xFFFF;
    String raw = device.getManufacturerData();
    if (raw.length() < 2) return 0xFFFF;
    return (uint8_t)raw[0] | ((uint16_t)(uint8_t)raw[1] << 8);
}

const char* tierName(uint8_t tier) {
    switch (tier) {
        case TIER_HIGH: return "HIGH-RULE";
        case TIER_MEDIUM: return "MEDIUM";
        case TIER_LOW: return "LOW";
        default: return "UNKNOWN";
    }
}

// -----------------------------------------------------------------------------
// Smart-glasses matcher - preserves CYD v6.3 semantics
// -----------------------------------------------------------------------------

bool confidenceRuleMatches(const GlassesConfidenceRule& rule,
                           BLEAdvertisedDevice& device,
                           const String& address,
                           const String& name,
                           uint16_t companyId,
                           const String& manufacturerHex) {
    bool hasCriterion = false;

    if (rule.companyId >= 0) {
        hasCriterion = true;
        if (companyId == 0xFFFF || companyId != (uint16_t)rule.companyId) return false;
    }

    if (rule.macExact != nullptr) {
        hasCriterion = true;
        if (!address.equalsIgnoreCase(rule.macExact)) return false;
    }

    if (rule.manufacturerHex != nullptr) {
        hasCriterion = true;
        String hay = manufacturerHex;
        String needle = String(rule.manufacturerHex);
        hay.replace("_", "");
        needle.replace("_", "");
        hay.toUpperCase();
        needle.toUpperCase();
        if (hay.indexOf(needle) < 0) return false;
    }

    if (rule.serviceUuid16 != 0) {
        hasCriterion = true;
        BLEUUID target(rule.serviceUuid16);
        if (!device.isAdvertisingService(target)) return false;
    }

    if (rule.namePattern != nullptr) {
        hasCriterion = true;
        if (!name.length()) return false;
        bool nameMatch = rule.exactName
            ? name.equalsIgnoreCase(rule.namePattern)
            : containsIgnoreCase(name, rule.namePattern);
        if (!nameMatch) return false;
    }

    if (rule.oui24 != 0) {
        hasCriterion = true;

        // Arduino-ESP32 3.3.11 uses NimBLE on ESP32-S3. BLEAddress::getNative()
        // stores NimBLE address bytes in host/native little-endian order, while
        // the human MAC/OUI order is the reverse. Keep the BLEAddress object
        // alive while reading its native buffer and extract the first three
        // human-readable MAC bytes as native[5], native[4], native[3].
        BLEAddress addrObj = device.getAddress();
        const uint8_t* mac = addrObj.getNative();
        uint32_t oui = ((uint32_t)mac[5] << 16) |
                       ((uint32_t)mac[4] << 8) |
                       ((uint32_t)mac[3]);
        if (oui != rule.oui24) return false;
    }

    return hasCriterion;
}

void considerRuleArray(const GlassesConfidenceRule* rules,
                       uint8_t tier,
                       BLEAdvertisedDevice& device,
                       const String& address,
                       const String& name,
                       uint16_t companyId,
                       const String& manufacturerHex,
                       DetectionResult& best) {
    for (int i = 0; rules[i].company != nullptr; ++i) {
        const GlassesConfidenceRule& rule = rules[i];
        if (!confidenceRuleMatches(rule, device, address, name, companyId, manufacturerHex)) continue;

        // Equal scores keep the earlier rule. This preserves the v6.3 ordering
        // inside HIGH: exact confirmed MAC -> fingerprint -> multi-signal.
        if (best.matched && rule.confidence <= best.confidence) continue;

        best.matched = true;
        best.detected = (rule.confidence >= POSSIBLE_CONFIDENCE_MIN);
        best.company = rule.company;
        best.product = rule.product;
        best.reason = rule.reason ? rule.reason : "Confidence rule match";
        best.hasCamera = rule.hasCamera;
        best.cameraKnown = rule.cameraKnown;
        best.tier = tier;
        best.confidence = rule.confidence;
        best.matchedRule = &rule;
    }
}

bool evaluateConfidenceRules(BLEAdvertisedDevice& device, DetectionResult& result) {
    result = {};

    // Raw observed address exists only in this callback path. It is used for
    // exact-MAC/OUI rule evaluation and is never retained or printed.
    String address = device.getAddress().toString().c_str();
    String name = device.haveName() ? String(device.getName().c_str()) : String("");
    name.trim();
    uint16_t companyId = getCompanyId(device);
    String manufacturerHex = getManufacturerHex(device);

    considerRuleArray(HIGH_CONFIDENCE_RULES, TIER_HIGH,
                      device, address, name, companyId, manufacturerHex, result);
    considerRuleArray(MEDIUM_CONFIDENCE_RULES, TIER_MEDIUM,
                      device, address, name, companyId, manufacturerHex, result);
    considerRuleArray(LOW_CONFIDENCE_RULES, TIER_LOW,
                      device, address, name, companyId, manufacturerHex, result);

    return result.matched;
}

bool matchFalsePositive(const String& name,
                        const char*& assumedDevice,
                        const char*& assumedType,
                        const char** reason) {
    assumedDevice = "";
    assumedType = "";
    if (reason != nullptr) *reason = "";
    if (!name.length()) return false;

    for (int i = 0; FALSE_POSITIVE_RULES[i].namePattern != nullptr; ++i) {
        if (!containsIgnoreCase(name, FALSE_POSITIVE_RULES[i].namePattern)) continue;
        assumedDevice = FALSE_POSITIVE_RULES[i].assumedDevice ? FALSE_POSITIVE_RULES[i].assumedDevice : "";
        assumedType = FALSE_POSITIVE_RULES[i].assumedType ? FALSE_POSITIVE_RULES[i].assumedType : "";
        if (reason != nullptr) {
            *reason = FALSE_POSITIVE_RULES[i].reason ? FALSE_POSITIVE_RULES[i].reason : "";
        }
        return true;
    }
    return false;
}

// -----------------------------------------------------------------------------
// Known-device annotation engine
// -----------------------------------------------------------------------------
// Kept in a header to avoid Arduino automatic-prototype ordering issues with
// KnownDeviceResult. Annotation semantics are unchanged.
#include "known_device_lookup.h"

// -----------------------------------------------------------------------------
// Consumer camera/audio review engine - explicit clues only, always 90%
// -----------------------------------------------------------------------------

bool cameraAudioNameMatches(const String& name,
                            const char* pattern,
                            CameraAudioNameMode mode) {
    if (pattern == nullptr || !name.length()) return false;
    switch (mode) {
        case CAMERA_AUDIO_PREFIX: return startsWithIgnoreCase(name, pattern);
        case CAMERA_AUDIO_EXACT: return name.equalsIgnoreCase(pattern);
        case CAMERA_AUDIO_CONTAINS:
        default: return containsIgnoreCase(name, pattern);
    }
}

bool evaluateCameraAudio(BLEAdvertisedDevice& device, CameraAudioResult& result) {
    result = {};
    String name = device.haveName() ? String(device.getName().c_str()) : String("");
    name.trim();
    if (!name.length()) return false;

    for (int i = 0; CAMERA_AUDIO_RULES[i].label != nullptr; ++i) {
        const CameraAudioRule& rule = CAMERA_AUDIO_RULES[i];
        if (!cameraAudioNameMatches(name, rule.namePattern, rule.nameMode)) continue;
        if (result.matched && rule.confidence <= result.confidence) continue;

        result.matched = true;
        result.label = rule.label;
        result.category = rule.category;
        result.confidence = rule.confidence;
        result.reason = rule.reason ? rule.reason : "Camera/audio advertised-name rule";
        result.matchedRule = &rule;
    }

    return result.matched && result.confidence >= CAMERA_AUDIO_ALERT_CONFIDENCE;
}

// -----------------------------------------------------------------------------
// Persistent-log classification/signature helpers
// -----------------------------------------------------------------------------

void buildGlassesRuleSignature(const GlassesConfidenceRule* rule,
                               char* out,
                               size_t outSize) {
    if (out == nullptr || outSize == 0) return;
    out[0] = '\0';
    if (rule == nullptr) {
        snprintf(out, outSize, "GLASSES_RULE");
        return;
    }

    if (rule->manufacturerHex != nullptr) {
        snprintf(out, outSize, "MFG:%.16s", rule->manufacturerHex);
    } else if (rule->serviceUuid16 != 0) {
        snprintf(out, outSize, "UUID:%04X", (unsigned)rule->serviceUuid16);
    } else if (rule->oui24 != 0) {
        snprintf(out, outSize, "OUI:%06lX", (unsigned long)rule->oui24);
    } else if (rule->macExact != nullptr) {
        // Never copy the exact MAC rule value into an observation record.
        snprintf(out, outSize, "EXACT_MAC_RULE");
    } else if (rule->namePattern != nullptr) {
        snprintf(out, outSize, "NAME:%.17s", rule->namePattern);
    } else if (rule->companyId >= 0) {
        snprintf(out, outSize, "CID:%04X", (unsigned)((uint16_t)rule->companyId));
    } else {
        snprintf(out, outSize, "GLASSES_RULE");
    }
}

void buildCameraAudioSignature(const CameraAudioRule* rule,
                               char* out,
                               size_t outSize) {
    if (out == nullptr || outSize == 0) return;
    out[0] = '\0';
    if (rule != nullptr && rule->namePattern != nullptr) {
        snprintf(out, outSize, "NAME:%.17s", rule->namePattern);
    } else {
        snprintf(out, outSize, "CAM_AUDIO_RULE");
    }
}

uint8_t persistentTierForGlasses(uint8_t tier) {
    return tier;
}

// -----------------------------------------------------------------------------
// LED alert arbitration
// -----------------------------------------------------------------------------

uint8_t alertPriority(LiteAlertKind kind) {
    switch (kind) {
        case ALERT_GLASSES_HIGH: return 3;
        case ALERT_CAMERA_AUDIO: return 2;
        case ALERT_GLASSES_POSSIBLE: return 1;
        case ALERT_NONE:
        default: return 0;
    }
}

bool raiseAlert(LiteAlertKind kind, uint8_t confidence) {
    uint32_t now = millis();
    bool newTriggerEpisode = false;

    portENTER_CRITICAL(&alertMux);

    bool expired = (alertState.kind == ALERT_NONE) ||
                   ((int32_t)(now - alertState.expiresAt) >= 0);
    uint8_t currentPriority = expired ? 0 : alertPriority(alertState.kind);
    uint8_t newPriority = alertPriority(kind);

    if (expired || newPriority > currentPriority) {
        // A fresh episode starts when no alert is active or when a higher
        // priority alert takes over. This is what increments the aggregate
        // LED-trigger counter. Repeated advertisements that merely refresh the
        // same active alert do NOT inflate the trigger total.
        alertState.kind = kind;
        alertState.confidence = confidence;
        alertState.expiresAt = now + ALERT_HOLD_MS;
        newTriggerEpisode = true;
    } else if (kind == alertState.kind) {
        // Same alert kind: refresh the 10-second hold without counting a new
        // LED trigger episode. No device identity is retained.
        alertState.confidence = confidence;
        alertState.expiresAt = now + ALERT_HOLD_MS;
    }

    portEXIT_CRITICAL(&alertMux);
    return newTriggerEpisode;
}

LiteAlertState currentAlertSnapshot() {
    LiteAlertState copy;
    portENTER_CRITICAL(&alertMux);
    copy = alertState;
    portEXIT_CRITICAL(&alertMux);
    return copy;
}

void updateStatusLed() {
    LiteAlertState current = currentAlertSnapshot();
    uint32_t now = millis();

    if (current.kind != ALERT_NONE && (int32_t)(now - current.expiresAt) < 0) {
        if (current.kind == ALERT_GLASSES_HIGH || current.kind == ALERT_CAMERA_AUDIO) {
            liteLedRedStrobe(now);
        } else {
            liteLedOrange();
        }
        return;
    }

    // Clear expired alert state.
    if (current.kind != ALERT_NONE) {
        portENTER_CRITICAL(&alertMux);
        if ((int32_t)(millis() - alertState.expiresAt) >= 0) {
            alertState.kind = ALERT_NONE;
            alertState.confidence = 0;
            alertState.expiresAt = 0;
        }
        portEXIT_CRITICAL(&alertMux);
    }

    if (privateModeActive) liteLedGreen();
    else if (publicFallbackActive) liteLedPurple();
    else liteLedBlue();
}

// -----------------------------------------------------------------------------
// PRIVATE NRPA / PUBLIC fallback — ESP32-S3 NimBLE implementation
// -----------------------------------------------------------------------------

void initRtcRetryCounter() {
    if (liteRetryMagic != LITE_RETRY_MAGIC) {
        liteRetryMagic = LITE_RETRY_MAGIC;
        litePrivateFailureCount = 0;
    }
}

void purpleFailureThenRestart(bool incrementPrivateFailure) {
    if (incrementPrivateFailure && litePrivateFailureCount < 255) {
        ++litePrivateFailureCount;
    }

    liteCountersIncrement(LITE_COUNT_PURPLE_PRIVATE_FAIL);
    liteCountersPersistNow();
    lite11NotePrivateFailure();
    lite11PrepareExpectedRestart();

    Serial.printf("PRIVACY ADDRESS FAILURE | private failures=%u/%u | rebooting\n",
                  (unsigned)litePrivateFailureCount,
                  (unsigned)PRIVATE_START_MAX_FAILURES);

    uint32_t started = millis();
    bool on = true;
    while ((uint32_t)(millis() - started) < PRIVATE_FAILURE_FLASH_MS) {
        if (on) liteLedPurple();
        else liteLedOff();
        on = !on;
        delay(PRIVATE_FAILURE_FLASH_STEP);
    }
    liteLedPurple();
    delay(100);
    ESP.restart();
    while (true) delay(1000);
}

uint32_t choosePrivateAddressRotationIntervalMs() {
    const uint32_t span = BLE_PRIVATE_ADDR_ROTATE_MAX_MS - BLE_PRIVATE_ADDR_ROTATE_MIN_MS;
    return BLE_PRIVATE_ADDR_ROTATE_MIN_MS + (esp_random() % (span + 1UL));
}

bool privateAddressStillConfirmed() {
    if (!privateModeActive) return false;
    return bleNimblePrivateNrpaConfirmed(scannerPrivateAddress);
}

bool applyFreshPrivateNrpa() {
    if (pBLEScan == nullptr || pBLEScan->isScanning()) return false;

    esp_bd_addr_t candidate = {0};
    if (!bleNimbleGenerateNrpa(candidate)) return false;

    // NimBLE path: install the generated NRPA, select RANDOM as the scanner's
    // own-address type, then verify that the configured random identity is an
    // NRPA and matches the generated bytes before permitting a scan.
    if (!bleNimbleApplyPrivateNrpa(candidate)) return false;

    bool confirmed = false;
    uint32_t started = millis();
    while ((uint32_t)(millis() - started) < BLE_PRIVATE_ADDR_CONFIRM_MS) {
        if (bleNimblePrivateNrpaConfirmed(candidate)) {
            confirmed = true;
            break;
        }
        delay(2);
    }
    if (!confirmed) return false;

    memcpy(scannerPrivateAddress, candidate, sizeof(scannerPrivateAddress));
    privateModeActive = true;
    publicFallbackActive = false;
    litePrivateFailureCount = 0;
    scannerPrivateAddressIntervalMs = choosePrivateAddressRotationIntervalMs();
    scannerPrivateAddressNextRotateMs = millis() + scannerPrivateAddressIntervalMs;

    Serial.printf("PRIVATE NRPA ACTIVE | next rotation in %lu ms\n",
                  (unsigned long)scannerPrivateAddressIntervalMs);
    return true;
}

bool applyPublicFallback() {
    if (pBLEScan == nullptr || pBLEScan->isScanning()) return false;

    if (!bleNimbleApplyPublicAddress()) return false;

    bool confirmed = false;
    uint32_t started = millis();
    while ((uint32_t)(millis() - started) < BLE_PRIVATE_ADDR_CONFIRM_MS) {
        if (bleNimblePublicAddressConfirmed()) {
            confirmed = true;
            break;
        }
        delay(2);
    }
    if (!confirmed) return false;

    privateModeActive = false;
    publicFallbackActive = true;
    scannerPrivateAddressNextRotateMs = 0;
    Serial.println("PUBLIC BLE ADDRESS FALLBACK ACTIVE");
    return true;
}

void initialiseScannerAddressMode() {
    if (litePrivateFailureCount >= PRIVATE_START_MAX_FAILURES) {
        Serial.printf("PRIVATE NRPA failed %u times; entering PUBLIC fallback\n",
                      (unsigned)litePrivateFailureCount);
        if (!applyPublicFallback()) {
            // Public-address setup also failed. Keep purple failure semantics and
            // reboot; the retained count keeps the next boot in PUBLIC fallback.
            purpleFailureThenRestart(false);
        }
        liteCountersIncrement(LITE_COUNT_PURPLE_PUBLIC_MODE);
        liteCountersPersistNow();
        lite11NotePublicFallback();
        lite11FlushPending(true);
        liteLedPurple();
        return;
    }

    if (!applyFreshPrivateNrpa()) {
        purpleFailureThenRestart(true);
    }

    liteCountersIncrement(LITE_COUNT_GREEN_PRIVATE_READY);
    liteCountersPersistNow();
    lite11NotePrivateStartSuccess();
    lite11FlushPending(true);
    liteLedGreen();
}

void updatePrivateRotation(uint32_t now) {
    if (!privateModeActive || pBLEScan == nullptr || pBLEScan->isScanning()) return;
    if (scannerPrivateAddressNextRotateMs == 0) return;
    if ((int32_t)(now - scannerPrivateAddressNextRotateMs) < 0) return;

    if (!applyFreshPrivateNrpa()) {
        Serial.println("PRIVATE NRPA rotation failed");
        purpleFailureThenRestart(true);
    }
}

bool scannerAddressModeConfirmed() {
    if (privateModeActive) return privateAddressStillConfirmed();
    if (publicFallbackActive) return bleNimblePublicAddressConfirmed();
    return false;
}

// -----------------------------------------------------------------------------
// BLE callbacks
// -----------------------------------------------------------------------------

class LiteScanCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        DetectionResult glasses = {};
        bool anyGlassesRule = evaluateConfidenceRules(advertisedDevice, glasses);
        bool glassesLoggable = anyGlassesRule && glasses.confidence >= LOGGING_CONFIDENCE_MIN;
        bool glassesDetected = anyGlassesRule && glasses.detected;

        String name = advertisedDevice.haveName()
            ? String(advertisedDevice.getName().c_str())
            : String("");
        name.trim();

        KnownDeviceResult knownDevice = {};
        bool knownDeviceMatched = evaluateKnownDevice(advertisedDevice, knownDevice);

        bool falsePositive = false;
        if (glassesLoggable) {
            const char* assumedDevice = "";
            const char* assumedType = "";
            const char* reason = "";
            falsePositive = matchFalsePositive(name, assumedDevice, assumedType, &reason);
        }

        CameraAudioResult cameraAudio = {};
        bool cameraAudioMatched = evaluateCameraAudio(advertisedDevice, cameraAudio);

        uint8_t actualAlertFlag = LITE11_ALERT_NONE;
        bool newAlertEpisode = false;

        // Operational alert priority remains unchanged from Lite v1.
        if (glassesDetected && !falsePositive && glasses.confidence >= HIGH_ALERT_CONFIDENCE) {
            actualAlertFlag = LITE11_ALERT_HIGH_GLASSES;
            newAlertEpisode = raiseAlert(ALERT_GLASSES_HIGH, glasses.confidence);
            if (newAlertEpisode) liteCountersIncrement(LITE_COUNT_RED_HIGH_GLASSES);
        } else if (cameraAudioMatched) {
            actualAlertFlag = LITE11_ALERT_CAMERA_AUDIO;
            newAlertEpisode = raiseAlert(ALERT_CAMERA_AUDIO, cameraAudio.confidence);
            if (newAlertEpisode) liteCountersIncrement(LITE_COUNT_RED_CAMERA_AUDIO);
        } else if (glassesDetected && !falsePositive) {
            actualAlertFlag = LITE11_ALERT_POSSIBLE_GLASSES;
            newAlertEpisode = raiseAlert(ALERT_GLASSES_POSSIBLE, glasses.confidence);
            if (newAlertEpisode) liteCountersIncrement(LITE_COUNT_ORANGE_POSSIBLE);
        }

        if (newAlertEpisode) lite11NoteAlertEpisode();

        // Persist only >=40% classifications. Ordinary BLE traffic remains
        // transient and never enters the field-test log.
        if (glassesLoggable || cameraAudioMatched) {
            BLEAddress addressObj = advertisedDevice.getAddress();
            uint64_t deviceHash = lite11HashObservedAddress(addressObj.getNative());

            uint8_t classFlags = 0;
            if (glassesLoggable) classFlags |= LITE11_CLASS_GLASSES;
            if (cameraAudioMatched) classFlags |= LITE11_CLASS_CAMERA_AUDIO;
            if (falsePositive) classFlags |= LITE11_CLASS_FALSE_POSITIVE_SUPPRESSED;
            if (knownDeviceMatched) classFlags |= LITE11_CLASS_KNOWN_DEVICE;

            bool selectCameraAudio = false;
            if (cameraAudioMatched) {
                if (!glassesLoggable) selectCameraAudio = true;
                else if (glasses.confidence < HIGH_ALERT_CONFIDENCE &&
                         cameraAudio.confidence >= glasses.confidence) selectCameraAudio = true;
            }

            char signature[24] = {0};
            const char* classification = "UNCLASSIFIED";
            uint8_t confidence = 0;
            uint8_t tier = 0xFF;
            uint8_t source = LITE11_SOURCE_GLASSES;
            uint16_t relevantUuid = 0;

            if (selectCameraAudio) {
                classification = cameraAudio.label ? cameraAudio.label : "CAMERA/AUDIO REVIEW";
                confidence = cameraAudio.confidence;
                tier = 0xFF;
                source = LITE11_SOURCE_CAMERA_AUDIO;
                buildCameraAudioSignature(cameraAudio.matchedRule, signature, sizeof(signature));
            } else if (glassesLoggable) {
                classification = glasses.product ? glasses.product : "SMART-GLASSES CANDIDATE";
                confidence = glasses.confidence;
                tier = persistentTierForGlasses(glasses.tier);
                source = LITE11_SOURCE_GLASSES;
                relevantUuid = glasses.matchedRule ? glasses.matchedRule->serviceUuid16 : 0;
                buildGlassesRuleSignature(glasses.matchedRule, signature, sizeof(signature));
            } else {
                classification = cameraAudio.label ? cameraAudio.label : "CAMERA/AUDIO REVIEW";
                confidence = cameraAudio.confidence;
                source = LITE11_SOURCE_CAMERA_AUDIO;
                buildCameraAudioSignature(cameraAudio.matchedRule, signature, sizeof(signature));
            }

            // Annotation-only known-device identity may make a non-alert >=40%
            // record easier to interpret. It never changes confidence, rule
            // signature, alert status, or whether the record qualifies for logging.
            if (knownDeviceMatched && actualAlertFlag == LITE11_ALERT_NONE &&
                knownDevice.assumedDeviceId != nullptr && knownDevice.assumedDeviceId[0] != '\0') {
                classification = knownDevice.assumedDeviceId;
            }

            Lite11ObservationInput observation = {};
            observation.deviceHash = deviceHash;
            observation.uptimeMs = millis();
            observation.rssi = (int8_t)advertisedDevice.getRSSI();
            observation.companyId = getCompanyId(advertisedDevice);
            observation.relevantUuid16 = relevantUuid;
            observation.confidence = confidence;
            observation.tier = tier;
            observation.source = source;
            observation.classFlags = classFlags;
            observation.alertFlags = actualAlertFlag;
            observation.falsePositiveSuppressed = falsePositive;
            observation.classification = classification;
            observation.signature = signature;
            lite11ObserveCandidate(observation);
        }
    }
};

void onBLEScanComplete(BLEScanResults results) {
    (void)results;
    scanFinishedFlag = true;
}

// -----------------------------------------------------------------------------
// Serial command input
// -----------------------------------------------------------------------------

void pollSerialCommand() {
    while (Serial.available() > 0) {
        char c = (char)Serial.read();
        if (c == '\r' || c == '\n') {
            serialCommandBuffer.trim();
            if (serialCommandBuffer.length()) {
                String cmd = serialCommandBuffer;
                cmd.toUpperCase();
                if (cmd == "LOG") {
                    logCommandPending = true;
                } else {
                    Serial.println("UNKNOWN COMMAND - use LOG");
                }
            }
            serialCommandBuffer = "";
        } else if (serialCommandBuffer.length() < 31) {
            serialCommandBuffer += c;
        }
    }
}

// -----------------------------------------------------------------------------
// Arduino setup/loop
// -----------------------------------------------------------------------------

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(250);

    initRtcRetryCounter();
    bool counterNvsReady = liteCountersBegin();
    bool persistentLogReady = lite11PersistentBegin();

    pinMode(STATUS_LED_PIN, OUTPUT);
    liteLedBlue();
    liteCountersIncrement(LITE_COUNT_BLUE_BOOT);
    liteCountersPersistNow();

    Serial.println();
    Serial.println("============================================");
    Serial.println(" ESP32 S3 Light");
    Serial.println(" Consumer field-test build - >=40% deduplicated LittleFS logging");
    Serial.println("============================================");
    Serial.printf("PRIVATE failure count: %u/%u\n",
                  (unsigned)litePrivateFailureCount,
                  (unsigned)PRIVATE_START_MAX_FAILURES);
    Serial.printf("Aggregate counter NVS: %s\n", counterNvsReady ? "READY" : "RAM-ONLY FALLBACK");
    Serial.printf("Persistent field-test log: %s\n", persistentLogReady ? "READY" : "UNAVAILABLE");
    Serial.printf("Session ID: %lu | lifetime boot: %lu\n",
                  (unsigned long)lite11CurrentSessionId(),
                  (unsigned long)lite11LifetimeBootCount());

    if (!BLEDevice::init("")) {
        Serial.println("BLE NimBLE initialization failed");
        purpleFailureThenRestart(true);
    }

    Serial.printf("BLE backend: %s\n", BLEDevice::getBLEStackString().c_str());

    pBLEScan = BLEDevice::getScan();
    if (pBLEScan == nullptr) {
        Serial.println("BLE scanner allocation failed");
        purpleFailureThenRestart(true);
    }

    pBLEScan->setAdvertisedDeviceCallbacks(new LiteScanCallbacks(), true);
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(BLE_SCAN_INTERVAL_MS);
    pBLEScan->setWindow(BLE_SCAN_WINDOW_MS);

    initialiseScannerAddressMode();

    Serial.println("Scan timing: active ~2 s every ~5 s, interval/window 100/95 ms");
    Serial.println("Alerts: LOW 1-59 none | 60-96 dim orange | 97-100 full-red strobe | camera/audio 90 full-red strobe");
    Serial.println("LOGGING: >=40% candidates only, per-session deduplicated, no raw observed MAC persistence.");
    Serial.println("SERIAL COMMAND: LOG");

    firstScanPending = true;
    lastCounterPrintMs = millis() - 1000UL; // print the first counter block immediately
    updateStatusLed();
}

void loop() {
    uint32_t now = millis();

    pollSerialCommand();
    updatePrivateRotation(now);
    updateStatusLed();

    if (pBLEScan != nullptr && !pBLEScan->isScanning()) {
        // LittleFS writes are kept out of the active BLE scan window.
        lite11FlushPending(false);

        if (logCommandPending) {
            const char* modeText = privateModeActive
                ? "PRIVATE_NRPA"
                : (publicFallbackActive ? "PUBLIC_FALLBACK" : "STARTING");
            lite11DumpAll(modeText);
            liteCountersPrintSerial(modeText);
            logCommandPending = false;
            now = millis();
            lastCounterPrintMs = now;
        }

        bool scanDue = firstScanPending ||
                       ((uint32_t)(now - lastScanStartMs) >= BLE_SCAN_CYCLE_MS);

        if (scanDue) {
            if (!scannerAddressModeConfirmed()) {
                Serial.println("BLE own-address verification failed before scan");
                if (privateModeActive) purpleFailureThenRestart(true);
                else purpleFailureThenRestart(false);
            }

            firstScanPending = false;
            lastScanStartMs = now;
            scanFinishedFlag = false;

            bool started = pBLEScan->start(BLE_SCAN_TIME, onBLEScanComplete, false);
            if (!started) {
                Serial.println("BLE scan failed to start");
            }
        }
    }

    if (scanFinishedFlag) {
        scanFinishedFlag = false;
        if (pBLEScan != nullptr) pBLEScan->clearResults();
        lite11FlushPending(false);
    }

    // Anonymous aggregate LED-trigger totals are printed once every second.
    // This report contains no observed-device identity or match detail.
    if ((uint32_t)(now - lastCounterPrintMs) >= 1000UL) {
        const char* modeText = privateModeActive
            ? "PRIVATE_NRPA"
            : (publicFallbackActive ? "PUBLIC_FALLBACK" : "STARTING");
        liteCountersPrintSerial(modeText);
        lastCounterPrintMs = now;
    }

    // NVS writes are deliberately NOT tied to the one-second Serial report.
    // Active alert totals are checkpointed at most once every five minutes.
    liteCountersCheckpointIfDue(now);

    delay(10);
}
