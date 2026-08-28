/*
 * CYD Dev Lite — 
 * Target: ESP32-2432S028R ("Cheap Yellow Display" / CYD)
 */

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Preferences.h>
#include <esp_system.h>
#include "tft_setup.h"
#include <TFT_eSPI.h>

// CYD Dev Lite display validation: this build MUST use ILI9341.
#ifndef ILI9341_DRIVER
#error "TFT SETUP NOT LOADED: expected ILI9341_DRIVER from tft_setup.h"
#endif
#if TFT_MOSI != 13
#error "TFT SETUP WRONG: TFT_MOSI must be GPIO13"
#endif
#if TFT_SCLK != 14
#error "TFT SETUP WRONG: TFT_SCLK must be GPIO14"
#endif
#if TFT_CS != 15
#error "TFT SETUP WRONG: TFT_CS must be GPIO15"
#endif
#if TFT_DC != 2
#error "TFT SETUP WRONG: TFT_DC must be GPIO2"
#endif
#if TFT_BL != 21
#error "TFT SETUP WRONG: TFT_BL must be GPIO21"
#endif
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "ble_private_scan.h"

/*
 * ESP-GlassHole — Configuration
 */

// ============================================================
// BLE Scan Settings
// ============================================================
#define BLE_SCAN_TIME          2        // BLE scan duration in seconds
#define BLE_SCAN_CYCLE_MS      5000UL   // Start a new scan every 5 seconds
#define BLE_SCAN_INTERVAL_MS   100    // BLEScan::setInterval() takes milliseconds
#define BLE_SCAN_WINDOW_MS      95    // BLEScan::setWindow() takes milliseconds (95% duty)

// ============================================================
// CYD Dev Lite — Local BLE Scanner Address Privacy
// ============================================================
// Rotation timing and the Arduino BLEScan own-address adapter live in
// ble_private_scan.h. Active Scan Requests use PRIVATE NRPA / RANDOM.

// ============================================================
// RSSI / Distance
// ============================================================
// v6.1 has no RSSI or distance floor for matching or alerts. RSSI is retained
// only as captured signal data and as the input to the rough metre estimate.

// ============================================================
// v6.1 Confidence / Alert Policy
// ============================================================
// LOW 1..59   : Company-ID/manufacturer clue only, log/annotate, NO alert.
// 60..96      : POSSIBLE smart glasses, ORANGE LED + orange TFT banner.
// 97..100     : HIGH alert, solid RED LED + red tiled TFT warnings.
#define POSSIBLE_CONFIDENCE_MIN 60
#define HIGH_ALERT_CONFIDENCE    97

// ============================================================
// Serial Output
// ============================================================
#define SERIAL_BAUD            115200
#define STATUS_INTERVAL_MS     60000UL // Periodic serial status every 60 seconds
#define SERIAL_STATS_INTERVAL_MS 10000UL // Human-readable SYS STAT mirror every 10 seconds
#define SD_CHECK_INTERVAL_MS   10000UL // Verify/remount SD every 10 seconds between BLE scans

// ============================================================
// Notification Cooldown
// ============================================================
// Don't re-alert for the same device within this window.
#define DETECTION_COOLDOWN_MS  20000UL // 20-second cooldown per detected device

// ============================================================
// Device Tracking
// ============================================================
#define MAX_TRACKED_DEVICES    36      // Cooldown history for detected devices
#define MAX_KNOWN_NON_GLASSES_DEVICES 512  // Unique known non-glasses session identities

/*
 * ESP-GlassHole — AR/Smart Glasses Detection Database
 *
 * Company IDs from Bluetooth SIG Assigned Numbers registry.
 * OUI prefixes from IEEE MA-L registry + field captures.
 * Device name patterns from BLE advertisement analysis.
 *
 * Sources:
 *   - Bluetooth SIG: bluetooth.com/specifications/assigned-numbers/
 *   - Nordic Semiconductor DB: github.com/NordicSemiconductor/bluetooth-numbers-database
 *   - yj_nearbyglasses: github.com/yjeanrenaud/yj_nearbyglasses
 *   - glass-detect: github.com/sh4d0wm45k/glass-detect
 *   - banrays: github.com/NullPxl/banrays
 *   - ouispy-detector: github.com/colonelpanichacks/ouispy-detector
 *
 * License: AGPL-3.0 (inherited from upstream)
 */

// ============================================================
// v6.1 confidence rule files. The legacy v3.4 glasses_config.h and
// generic_keyword_matches.h are intentionally not compiled in v6.1, preventing
// old tier rules from double-triggering or overriding the confidence engine.
#define TIER_HIGH   0
#define TIER_MEDIUM 1
#define TIER_LOW    2

#include "high_confidence_matches.h"
#include "medium_confidence_matches.h"
#include "low_confidence_matches.h"
#include "ble_reference_data.h"
#include "known_devices.h"
#include "false_positives.h"
#include "suspicious_devices.h"
#include "mics_and_cams.h"

struct DetectionResult {
    bool        matched;       // any confidence rule matched, including LOW
    bool        detected;      // smart-glasses alert candidate: confidence >= 60
    const char* company;
    const char* product;
    const char* reason;
    bool        hasCamera;
    bool        cameraKnown;   // false = display/log CAMERA UNKNOWN
    uint8_t     tier;          // rule-file tier: HIGH / MEDIUM / LOW
    uint8_t     confidence;    // 0..100
};

struct SuspiciousMatchResult {
    bool        matched;
    const char* category;
    const char* deviceLabel;
    const char* reason;
    uint8_t     confidence;
    char        reasonBuf[160];
};

struct MicCamMatchResult {
    bool        matched;
    const char* deviceLabel;
    MicCamCategory category;
    const char* reason;
    uint8_t     confidence;
};

// Keep this type before the sketch's first function definition. Arduino's
// automatic prototype generator needs to know the return type of
// classifyAssumedDevice() before it emits that prototype.
struct AssumedDeviceInfo {
    const char* deviceId;
    const char* deviceType;
    bool falsePositive;
};

// Explicit declaration avoids depending on Arduino's auto-prototype generator
// for a function returning the custom BleServiceReference type.
const BleServiceReference* serviceReferenceForUuidString(const String& serviceUUIDs);

// ---------------- CYD hardware ----------------
#define BOARD_TYPE "ESP32-2432S028R CYD"

// Common CYD microSD / VSPI pins
#define SD_SCK   18
#define SD_MISO  19
#define SD_MOSI  23
#define SD_CS     5

// Persistent UI: scanner status stays visible continuously.
#define WARNING_BASE_MS 10000UL
#define MAX_WARNING_DEVICES    20      // Safety ceiling for simultaneous warning tiles

// CYD onboard RGB LED channels are active-low.
#define CYD_LED_RED_PIN    4
#define CYD_LED_GREEN_PIN 16
#define CYD_LED_BLUE_PIN  17

// v6.2 BOOT/UI gesture thresholds live in ui_controls.h.
// Keep control timing and on-screen instructions synchronized there.
#include "ui_controls.h"

// v6.2 Sentry Mode user-tunable settings live in sentry_config.h.
// Keep Sentry timing/threshold/brightness edits there so the main scanner logic
// remains untouched. Derived millisecond values and learning capacity are also
// calculated in that header.
#include "sentry_config.h"

// Manual BLE context logging from the scanner page.
// A manual event captures observations strictly by arrival order:
// the 5 immediately before the BOOT hold and the next 5 afterward.
// The BEFORE ring is frozen while an event is active/pending.
#define MANUAL_LOG_NOTICE_MS          2200UL
#define MANUAL_CONTEXT_BEFORE_COUNT       5
#define MANUAL_CONTEXT_AFTER_COUNT        5

// v6.2 session / SD safety
#define DEVICE_NAME               "CYD"
#define SESSION_RESET_MS          86400000UL  // 24 hours of powered runtime

// v6.1 privacy: raw BLE MAC addresses are used only transiently while an
// advertisement is being evaluated. Persistent logs and long-lived runtime
// structures use a session-scoped pseudonymous device MAC hash instead. The per-session
// salt is generated in RAM and is never written to SD/NVS.
#define DEVICE_MAC_HASH_LEN     26  // "MAC-HASH-" + 16 hex + NUL
#define SESSION_LABEL_LEN            32
#define APPROX_DISTANCE_MIN_M       0.1f
#define APPROX_DISTANCE_MAX_M      99.0f

TFT_eSPI tft = TFT_eSPI();
SPIClass sdSPI(VSPI);
BLEScan* pBLEScan = nullptr;

// v6.2 CYD-originating BLE privacy state. This is the scanner's own RF
// address and is entirely separate from hashes derived from observed devices.
esp_bd_addr_t scannerPrivateAddress = {0};
bool scannerPrivateAddressReady = false;
char scannerPrivateAddressText[18] = "--:--:--:--:--:--";
uint32_t scannerPrivateAddressRotationCount = 0;
uint32_t scannerPrivateAddressIntervalMs = 0;
uint32_t scannerPrivateAddressNextRotateMs = 0;

enum SDStatus {
    SD_STATUS_NOT_DETECTED = 0,
    SD_STATUS_DETECTED,
    SD_STATUS_FAIL,
    SD_STATUS_EJECTED
};

SDStatus sdStatus = SD_STATUS_NOT_DETECTED;
bool sdOK = false;
bool sdSPIStarted = false;
bool sdSafelyEjected = false;
// After SAFE EJECT, automatic remount is allowed only after the scanner has
// first observed the card physically absent. This prevents a safe-ejected card
// that is still sitting in the socket from immediately remounting itself.
bool sdRemovalSeenAfterSafeEject = false;
uint32_t lastSDCheckMs = 0;
// 512-byte read-only probe buffer used to verify the mounted card really
// responds over SPI. SD.cardType() alone is cached by the Arduino-ESP32 SD
// implementation and is not sufficient to prove a mounted card is still in.
uint8_t sdPresenceProbeBuffer[512];
SemaphoreHandle_t sdMutex = nullptr;

Preferences sessionPrefs;
uint32_t sessionNumber = 0;
char sessionId[SESSION_LABEL_LEN] = "UNSET";
uint32_t sessionPeriodStartedMs = 0;
uint32_t sessionDeviceSaltA = 0;
uint32_t sessionDeviceSaltB = 0;

bool screenAwake = false;
uint32_t screenWakeStarted = 0;
uint32_t totalAdvertisements = 0;        // all BLE advertisements received
uint32_t totalUniqueAdvertisements = 0;  // unique advertisement payloads written to SD
uint32_t totalScans = 0;
uint32_t totalDetections = 0;
uint32_t totalFalsePositivesSuppressed = 0;
uint32_t totalSuspiciousDeviceReviews = 0;
uint32_t totalMicCamReviews = 0;

// Session-local hashed BLE identities matched by known_devices.h or explicit
// false-positive identities. Raw MAC addresses are not retained here.
uint32_t knownNonGlassesDeviceHashes[MAX_KNOWN_NON_GLASSES_DEVICES] = {0};
uint16_t knownNonGlassesDeviceCount = 0;

// v6.1 Sentry Mode session counters. "Individual" means one session-local
// hashed BLE identity. Raw MAC addresses are not retained in these counters.
uint32_t sentryUniqueDeviceHashes[MAX_SENTRY_UNIQUE_DEVICES] = {0};
uint8_t sentryUniqueDeviceKnown[MAX_SENTRY_UNIQUE_DEVICES] = {0};
uint16_t sentryIndividualDeviceCount = 0;
uint16_t sentryKnownDeviceCount = 0;
uint32_t sentryDuplicateDeviceCount = 0;
uint32_t sentrySessionScanCount = 0;

enum SentryState : uint8_t {
    SENTRY_STATE_SLEEP = 0,
    SENTRY_STATE_SAMPLE,
    SENTRY_STATE_ACTIVE
};

bool sentryModeActive = false;
bool sentryEntering = false;
bool sentryDashboardAwake = false;
bool sentryLegalNoticeVisible = false;
uint32_t sentryEnteringStartedMs = 0;
uint32_t sentryWakeStartedMs = 0;
uint32_t lastSentryTouchMs = 0;
uint32_t lastSentryDashRefreshMs = 0;
bool lastSentryTouchPressed = false;
volatile bool sentryDashboardDirty = false;

SentryState sentryState = SENTRY_STATE_SLEEP;
uint32_t sentryStateStartedMs = 0;
uint32_t sentryActiveStartedMs = 0;
uint32_t sentryActiveWindowStartedMs = 0;
uint16_t sentryBaseline = 0;
bool sentryBaselineValid = false;
uint16_t sentryLastSample = 0;
uint16_t sentryOldBaseline = 0;
uint16_t sentryProvisionalMedian = 0;
uint16_t sentryActiveSamples[SENTRY_ACTIVE_SAMPLE_CAPACITY] = {0};
uint16_t sentryActiveSampleCount = 0;

// One fixed hash set is reused for the current Sentry measurement window.
// window. A mutex prevents the BLE callback and loop() from resetting it at
// the same time.
uint32_t sentryWindowDeviceHashes[MAX_SENTRY_WINDOW_UNIQUE_DEVICES] = {0};
uint16_t sentryWindowUniqueCount = 0;
SemaphoreHandle_t sentryActivityMutex = nullptr;

uint32_t lastStatusTime = 0;
uint32_t lastSerialStatsTime = 0;

// Async BLE scan state. This allows the loading circle to animate
// while the radio is actually scanning.
volatile bool scanFinishedFlag = false;
uint32_t lastScanStartMs = 0;
bool firstScanPending = true;

// UI state
volatile bool warningScreenActive = false;
uint32_t lastSpinnerFrameMs = 0;
uint8_t spinnerFrame = 0;
uint32_t lastWarningLabelScrollMs = 0;
SemaphoreHandle_t uiMutex = nullptr;

enum DisplayPage {
    PAGE_SCANNER = 0,
    PAGE_STATUS
};

enum LEDLogicalState {
    LED_STATE_BLUE = 0,
    LED_STATE_GREEN,
    LED_STATE_ORANGE,
    LED_STATE_RED,
    LED_STATE_OFF,
    LED_STATE_SENTRY_DIM_RED,
    LED_STATE_TEST_BLUE,
    LED_STATE_TEST_GREEN,
    LED_STATE_TEST_ORANGE,
    LED_STATE_TEST_RED
};

volatile LEDLogicalState ledLogicalState = LED_STATE_BLUE;
bool bleScanningAtBoot = false;

DisplayPage currentPage = PAGE_SCANNER;
uint32_t lastStatusPageRefreshMs = 0;
bool lastPageButtonPressed = false;
uint32_t lastPageButtonChangeMs = 0;
uint32_t pageButtonPressedAtMs = 0;
bool pageButtonLongHandled = false;
bool pageButtonSafeEjectHandled = false;
bool statusShortClickPending = false;
bool statusSecondClickArmed = false;
uint32_t statusFirstClickReleasedMs = 0;

// Manual-log confirmation/failure overlay.
bool manualLogNoticeActive = false;
uint32_t manualLogNoticeExpiresMs = 0;
String manualLogNoticeTitle = "";
String manualLogNoticeLine1 = "";
String manualLogNoticeLine2 = "";

// Callback-local post-detection snapshot used by logging/statistics paths.
// It is created only after all detection engines have examined the original BLE
// advertisement, so consolidating these fields cannot change rule matching.
struct BLELogSnapshot {
    String address;
    String name;
    String manufacturerHex;
    String serviceUUIDs;
    int rssi;
    uint16_t companyId;
    uint8_t addressType;
};

// Ordered BLE observations used ONLY for deliberate manual context logging.
// These are raw observation-order snapshots, not a strongest/nearest device list.
struct ManualBLEObservation {
    uint32_t observedAtMs;
    char deviceMacHash[DEVICE_MAC_HASH_LEN];

    char name[64];
    uint8_t addressType;
    uint16_t companyId;
    char manufacturerHex[160];
    char serviceUUID[40];
    bool detectedByScanner;
    const char* product;
    uint8_t cameraStatusCode;
    uint8_t tier;
    uint8_t confidence;
};

// Ring containing exactly the most recent observations before a manual event.
ManualBLEObservation manualBeforeRing[MANUAL_CONTEXT_BEFORE_COUNT] = {};
uint8_t manualBeforeRingCount = 0;
uint8_t manualBeforeRingNext = 0;

// The BEFORE ring itself is the frozen pre-event snapshot. AFTER observations
// are filled by the BLE callback after the BOOT hold. Once all 5 arrive, loop()
// writes the complete 5+5 event to manual_smart_glasses_logged.csv.
uint8_t manualBeforeFrozenStart = 0;
ManualBLEObservation manualContextAfter[MANUAL_CONTEXT_AFTER_COUNT] = {};
volatile bool manualContextCaptureActive = false;
volatile bool manualContextReadyToWrite = false;
bool manualContextWriteInProgress = false;
uint8_t manualContextAfterCount = 0;
uint32_t manualContextEventCounter = 0;
char manualContextEventId[48] = "";
SemaphoreHandle_t manualContextMutex = nullptr;

// Exact-advertisement de-duplication cache.
// RSSI is deliberately NOT part of the signature, so the same advertisement
// is not written repeatedly just because signal strength changed.
// A payload/name/company/service change produces a different signature and is logged.
#define MAX_ADV_SIGNATURES 1024
uint32_t seenAdvSignatures[MAX_ADV_SIGNATURES] = {0};
uint16_t seenAdvSignatureCount = 0;
uint16_t seenAdvSignatureReplace = 0;

struct WarningDevice {
    String deviceMacHash;
    String name;
    const char* product;
    const char* cameraStatus;
    int rssi;
    uint8_t confidence;
    uint32_t expiresAt;  // this tile disappears 10 seconds after its own detection
};

WarningDevice warningDevices[MAX_WARNING_DEVICES];
uint8_t warningDeviceCount = 0;

// v6.1 POSSIBLE alert: one orange banner, independent of the 97..100 red tiles.
struct PossibleAlertBanner {
    bool active;
    String deviceMacHash;
    String label;
    int rssi;
    uint8_t confidence;
    uint32_t expiresAt;
};
PossibleAlertBanner possibleBanner = {};

// Separate orange review screen for DIY/development/unusual BLE devices.
// This is independent from the smart-glasses confidence engine.
struct SuspiciousAlertState {
    bool active;
    String deviceMacHash;
    const char* deviceLabel;
    const char* category;
    uint8_t confidence;
    uint32_t expiresAt;
};
SuspiciousAlertState suspiciousAlert = {};
String lastSuspiciousAlertDeviceMacHash = "";
uint32_t lastSuspiciousAlertMs = 0;

// Separate orange CAM AND AUDIO review screen. This is independent from both
// smart-glasses confidence and suspicious_devices.h. It outranks the generic
// suspicious/review screen but never outranks a 97%+ HIGH glasses warning.
struct MicCamAlertState {
    bool active;
    String deviceMacHash;
    const char* deviceLabel;
    const char* category;
    uint8_t confidence;
    uint32_t expiresAt;
};
MicCamAlertState micCamAlert = {};
String lastMicCamAlertDeviceMacHash = "";
uint32_t lastMicCamAlertMs = 0;

// -----------------------------------------------------------------------------
// Self-test support interface
// -----------------------------------------------------------------------------
// self_test.h owns the complete editable diagnostic sequence. It deliberately
// calls the REAL operational render/LED helpers declared below. Keep these
// declarations in sync if an operational helper signature changes.
float approximateDistanceMetres(int rssi);
uint16_t getCompanyId(BLEAdvertisedDevice& device);
String getManufacturerHex(BLEAdvertisedDevice& device);
bool takeUi(uint32_t timeoutMs);
void giveUi();
void clearWarningDevice(WarningDevice& d);
void warningGrid(uint8_t count, uint8_t& cols, uint8_t& rows);
void drawWarningTile(const WarningDevice& d, uint8_t number, int x, int y, int w, int h);
void showStatusPage();
void showSelectedPage();
void clearWarningDevices();
void clearPossibleBanner();
void clearSuspiciousAlert();
void clearMicCamAlert();
void drawSuspiciousAlertScreen(const char* deviceLabel, const char* category);
void drawMicCamAlertScreen(const char* deviceLabel, const char* category);
void rgbBlue();
void rgbGreen();
void rgbOrange();
void rgbRed();
void rgbDimRed();

#include "self_test.h"

int lastRSSI = -127;

struct TrackedDevice {
    uint32_t deviceHash;
    uint32_t lastSeen;
    int      rssi;
    uint8_t  tier;
    bool     hasCamera;
};

TrackedDevice trackedDevices[MAX_TRACKED_DEVICES];
int trackedDeviceCount = 0;

// ---------------- Helpers ----------------

void writeCsvField(Print& out, const char* value) {
    out.write('"');
    if (value != nullptr) {
        for (const char* p = value; *p != '\0'; ++p) {
            if (*p == '"') {
                out.print("\"\"");
            } else if (*p == '\r' || *p == '\n') {
                out.write(' ');
            } else {
                out.write((uint8_t)*p);
            }
        }
    }
    out.write('"');
}

void writeCsvField(Print& out, const String& value) {
    out.write('"');
    for (size_t i = 0; i < value.length(); ++i) {
        char c = value[i];
        if (c == '"') {
            out.print("\"\"");
        } else if (c == '\r' || c == '\n') {
            out.write(' ');
        } else {
            out.write((uint8_t)c);
        }
    }
    out.write('"');
}

// -----------------------------------------------------------------------------
// v6.1 session-scoped device privacy
// -----------------------------------------------------------------------------
// The radio stack supplies a full BLE address to the callback. Matching may use
// it transiently (including exact-MAC rules), but the address is not persisted
// and is not kept in long-lived runtime state. Instead, one pseudonymous device MAC hash is
// derived from a RAM-only random salt for the current session. A new session
// produces different MAC-HASH-XXXXXXXXXXXXXXXX values for the same physical BLE address.
void regenerateSessionDeviceSalt() {
    sessionDeviceSaltA = esp_random();
    sessionDeviceSaltB = esp_random();

    // Avoid the all-zero edge case even on a pathological RNG result.
    if ((sessionDeviceSaltA | sessionDeviceSaltB) == 0) {
        sessionDeviceSaltA = 0x9E3779B9UL ^ millis();
        sessionDeviceSaltB = 0x85EBCA6BUL ^ micros();
    }
}

uint32_t sessionDeviceHashForAddress(const String& address) {
    uint32_t hash = 2166136261UL ^ sessionDeviceSaltA;
    for (size_t i = 0; i < address.length(); i++) {
        hash ^= (uint8_t)address[i];
        hash *= 16777619UL;
    }
    hash ^= sessionDeviceSaltB;
    hash *= 16777619UL;
    if (hash == 0) hash = 1;
    return hash;
}

void formatDeviceMacHashForAddress(const String& address,
                                   char* out,
                                   size_t outLen) {
    // Two independently mixed 32-bit values are concatenated to produce a
    // compact 64-bit session-scoped MAC hash for persistent logs. The random
    // salt is never stored, so the same MAC normally hashes differently in a
    // later session while remaining stable inside the current session.
    uint32_t h1 = sessionDeviceHashForAddress(address);
    uint32_t h2 = 2166136261UL ^ sessionDeviceSaltB ^ 0xA5A5A5A5UL;
    for (size_t i = 0; i < address.length(); i++) {
        h2 ^= (uint8_t)address[i];
        h2 *= 16777619UL;
    }
    h2 ^= sessionDeviceSaltA ^ 0x5A5A5A5AUL;
    h2 *= 16777619UL;
    if (h2 == 0) h2 = 1;

    if (out == nullptr || outLen == 0) return;
    snprintf(out, outLen, "MAC-HASH-%08lX%08lX",
             (unsigned long)h1, (unsigned long)h2);
    out[outLen - 1] = '\0';
}

String deviceMacHashForAddress(const String& address) {
    char out[DEVICE_MAC_HASH_LEN];
    formatDeviceMacHashForAddress(address, out, sizeof(out));
    return String(out);
}

String bytesToHex(const uint8_t* data, size_t len) {
    static const char HEX_DIGITS[] = "0123456789ABCDEF";
    String hex;
    hex.reserve(len * 2);
    for (size_t i = 0; i < len; i++) {
        hex += HEX_DIGITS[(data[i] >> 4) & 0x0F];
        hex += HEX_DIGITS[data[i] & 0x0F];
    }
    return hex;
}

bool containsIgnoreCase(const String& haystack, const char* needle) {
    if (needle == nullptr) return false;
    String a = haystack;
    String b = String(needle);
    a.toLowerCase();
    b.toLowerCase();
    return a.indexOf(b) >= 0;
}

const char* scanRatingForRSSI(int rssi) {
    if (rssi >= -55) return "VERY CLOSE";
    if (rssi >= -60) return "CLOSE";
    if (rssi >= -65) return "MEDIUM";
    if (rssi >= -80) return "WEAK";
    return "VERY WEAK";
}

// RSSI distance is a rough estimate only. Indoor BLE is strongly affected
// by bodies, reflections, walls, antenna orientation and transmit power.
float approximateDistanceMetres(int rssi) {
    const float txPowerAt1m = -59.0f;
    const float pathLossN = 2.2f;
    float exponent = (txPowerAt1m - (float)rssi) / (10.0f * pathLossN);
    float metres = powf(10.0f, exponent);
    if (metres < APPROX_DISTANCE_MIN_M) metres = APPROX_DISTANCE_MIN_M;
    if (metres > APPROX_DISTANCE_MAX_M) metres = APPROX_DISTANCE_MAX_M;
    return metres;
}

const char* ledStatusText() {
    switch (ledLogicalState) {
        case LED_STATE_GREEN:       return "GREEN - CLEAR";
        case LED_STATE_ORANGE:      return "ORANGE - POSSIBLE / REVIEW";
        case LED_STATE_RED:         return "RED - HIGH 97%+";
        case LED_STATE_OFF:         return "OFF";
        case LED_STATE_SENTRY_DIM_RED: return "DIM RED - SENTRY MODE";
        case LED_STATE_TEST_BLUE:   return "TEST BLUE";
        case LED_STATE_TEST_GREEN:  return "TEST GREEN";
        case LED_STATE_TEST_ORANGE: return "TEST ORANGE";
        case LED_STATE_TEST_RED:    return "TEST RED";
        case LED_STATE_BLUE:
        default:                    return "BLUE - BOOT/SETUP";
    }
}

void formatSessionId() {
    snprintf(sessionId, sizeof(sessionId), "%s-S%04lu",
             DEVICE_NAME,
             (unsigned long)sessionNumber);
}

void initSessionId() {
    sessionPrefs.begin("glassscan", false);
    sessionNumber = sessionPrefs.getUInt("session", 0) + 1;
    if (sessionNumber == 0 || sessionNumber > 9999) sessionNumber = 1;
    sessionPrefs.putUInt("session", sessionNumber);
    sessionPrefs.end();

    formatSessionId();
    regenerateSessionDeviceSalt();
    sessionPeriodStartedMs = millis();
}

void loadSentryBaseline() {
    sessionPrefs.begin("glassscan", false);
    sentryBaselineValid = sessionPrefs.getUInt("sbase_set", 0) == 1;
    sentryBaseline = (uint16_t)sessionPrefs.getUInt("sbase", 0);
    sessionPrefs.end();

    if (!sentryBaselineValid) sentryBaseline = 0;
}

void saveSentryBaseline(uint16_t baseline) {
    sentryBaseline = baseline;
    sentryBaselineValid = true;

    sessionPrefs.begin("glassscan", false);
    sessionPrefs.putUInt("sbase", (uint32_t)baseline);
    sessionPrefs.putUInt("sbase_set", 1);
    sessionPrefs.end();
}

uint16_t calculateSentryTriggerThreshold() {
    if (!sentryBaselineValid) return 0;

    uint32_t byPercent = ((uint32_t)sentryBaseline * SENTRY_TRIGGER_PERCENT + 99UL) / 100UL;
    uint32_t byIncrease = (uint32_t)sentryBaseline + SENTRY_TRIGGER_MIN_INCREASE;
    uint32_t threshold = byPercent > byIncrease ? byPercent : byIncrease;
    if (threshold > 65535UL) threshold = 65535UL;
    return (uint16_t)threshold;
}

bool startsWithIgnoreCase(const String& value, const char* prefix) {
    if (prefix == nullptr) return false;
    String a = value;
    String b = String(prefix);
    a.toLowerCase();
    b.toLowerCase();
    return a.startsWith(b);
}

bool suspiciousNameMatches(const String& name,
                           const char* pattern,
                           SuspiciousNameMatchMode mode) {
    if (pattern == nullptr || !name.length()) return false;

    switch (mode) {
        case SUSPICIOUS_PREFIX:
            return startsWithIgnoreCase(name, pattern);
        case SUSPICIOUS_EXACT:
            return name.equalsIgnoreCase(pattern);
        case SUSPICIOUS_CONTAINS:
        default:
            return containsIgnoreCase(name, pattern);
    }
}

bool evaluateSuspiciousDevice(BLEAdvertisedDevice& device,
                              SuspiciousMatchResult& result) {
    result = {};

    String name = device.haveName()
        ? String(device.getName().c_str())
        : String("");
    name.trim();
    if (!name.length()) return false;

    bool companyIdLoaded = false;
    uint16_t companyId = 0xFFFF;
    bool ouiLoaded = false;
    uint32_t oui = 0;

    for (int i = 0; SUSPICIOUS_DEVICE_RULES[i].category != nullptr; i++) {
        const SuspiciousDeviceRule& rule = SUSPICIOUS_DEVICE_RULES[i];

        // Primary model/family name is required for the current onboard rules.
        // Optional company/OUI criteria can be added later in the editable header.
        if (!suspiciousNameMatches(name, rule.primaryNamePattern, rule.nameMode)) {
            continue;
        }
        if (rule.companyId >= 0) {
            if (!companyIdLoaded) {
                companyId = getCompanyId(device);
                companyIdLoaded = true;
            }
            if (companyId == 0xFFFF || companyId != (uint16_t)rule.companyId) {
                continue;
            }
        }
        if (rule.oui24 != 0) {
            if (!ouiLoaded) {
                const uint8_t* mac = device.getAddress().getNative();
                oui = ((uint32_t)mac[0] << 16) |
                      ((uint32_t)mac[1] << 8) |
                      ((uint32_t)mac[2]);
                ouiLoaded = true;
            }
            if (oui != rule.oui24) continue;
        }

        bool secondary = rule.secondaryNamePattern != nullptr &&
                         containsIgnoreCase(name, rule.secondaryNamePattern);
        uint16_t score = rule.confidence;
        if (secondary) score += rule.secondaryBoost;
        if (score > 99) score = 99;

        if (result.matched && score <= result.confidence) continue;

        result.matched = true;
        result.category = rule.category;
        result.deviceLabel = rule.deviceLabel;
        result.confidence = (uint8_t)score;
        snprintf(result.reasonBuf, sizeof(result.reasonBuf), "%s%s",
                 rule.reason ? rule.reason : "Review rule match",
                 secondary ? " + secondary name clue" : "");
        result.reason = result.reasonBuf;
    }

    return result.matched && result.confidence >= SUSPICIOUS_ALERT_MIN_CONFIDENCE;
}

const char* micCamCategoryLabel(MicCamCategory category) {
    switch (category) {
        case MICCAM_CATEGORY_CAMERA:       return "CAMERA";
        case MICCAM_CATEGORY_MICROPHONE:   return "MICROPHONE";
        case MICCAM_CATEGORY_RECORDER:     return "RECORDER";
        case MICCAM_CATEGORY_CAMERA_AUDIO:
        default:                           return "CAMERA / AUDIO";
    }
}

bool micCamNameMatches(const String& name,
                       const char* pattern,
                       MicCamNameMatchMode mode) {
    if (pattern == nullptr || !name.length()) return false;

    switch (mode) {
        case MICCAM_PREFIX:
            return startsWithIgnoreCase(name, pattern);
        case MICCAM_EXACT:
            return name.equalsIgnoreCase(pattern);
        case MICCAM_CONTAINS:
        default:
            return containsIgnoreCase(name, pattern);
    }
}

bool evaluateMicCamDevice(BLEAdvertisedDevice& device,
                          MicCamMatchResult& result) {
    result = {};

    String name = device.haveName()
        ? String(device.getName().c_str())
        : String("");
    name.trim();

    bool companyIdLoaded = false;
    uint16_t companyId = 0xFFFF;
    bool manufacturerHexLoaded = false;
    String manufacturerHex;
    bool ouiLoaded = false;
    uint32_t oui = 0;

    for (int i = 0; MIC_CAM_RULES[i].deviceLabel != nullptr; i++) {
        const MicCamRule& rule = MIC_CAM_RULES[i];
        bool hasCriterion = false;

        if (rule.namePattern != nullptr) {
            hasCriterion = true;
            if (!micCamNameMatches(name, rule.namePattern, rule.nameMode)) continue;
        }

        if (rule.companyId >= 0) {
            hasCriterion = true;
            if (!companyIdLoaded) {
                companyId = getCompanyId(device);
                companyIdLoaded = true;
            }
            if (companyId == 0xFFFF || companyId != (uint16_t)rule.companyId) continue;
        }

        if (rule.manufacturerHex != nullptr) {
            hasCriterion = true;
            if (!manufacturerHexLoaded) {
                manufacturerHex = getManufacturerHex(device);
                manufacturerHexLoaded = true;
            }
            String hay = manufacturerHex;
            String needle = String(rule.manufacturerHex);
            hay.replace("_", "");
            needle.replace("_", "");
            hay.toUpperCase();
            needle.toUpperCase();
            if (hay.indexOf(needle) < 0) continue;
        }

        if (rule.serviceUuid16 != 0) {
            hasCriterion = true;
            BLEUUID target(rule.serviceUuid16);
            if (!device.isAdvertisingService(target)) continue;
        }

        if (rule.oui24 != 0) {
            hasCriterion = true;
            if (!ouiLoaded) {
                const uint8_t* mac = device.getAddress().getNative();
                oui = ((uint32_t)mac[0] << 16) |
                      ((uint32_t)mac[1] << 8) |
                      ((uint32_t)mac[2]);
                ouiLoaded = true;
            }
            if (oui != rule.oui24) continue;
        }

        if (!hasCriterion) continue;
        if (result.matched && rule.confidence <= result.confidence) continue;

        result.matched = true;
        result.deviceLabel = rule.deviceLabel;
        result.category = rule.category;
        result.confidence = rule.confidence;
        result.reason = rule.reason ? rule.reason : "CAM AND AUDIO rule match";
    }

    return result.matched && result.confidence >= MICCAM_ALERT_MIN_CONFIDENCE;
}

bool matchFalsePositive(const String& name,
                        const char*& assumedDevice,
                        const char*& assumedType,
                        const char** reason) {
    assumedDevice = "";
    assumedType = "";
    if (reason != nullptr) *reason = "";
    if (!name.length()) return false;

    for (int i = 0; FALSE_POSITIVE_RULES[i].namePattern != nullptr; i++) {
        if (containsIgnoreCase(name, FALSE_POSITIVE_RULES[i].namePattern)) {
            assumedDevice = FALSE_POSITIVE_RULES[i].assumedDevice
                ? FALSE_POSITIVE_RULES[i].assumedDevice : "";
            assumedType = FALSE_POSITIVE_RULES[i].assumedType
                ? FALSE_POSITIVE_RULES[i].assumedType : "";
            if (reason != nullptr) {
                *reason = FALSE_POSITIVE_RULES[i].reason
                    ? FALSE_POSITIVE_RULES[i].reason : "";
            }
            return true;
        }
    }
    return false;
}

bool matchKnownDevice(const String& name,
                      uint16_t companyId,
                      const String& manufacturerHex,
                      const char*& assumedDevice,
                      const char*& assumedType) {
    assumedDevice = "";
    assumedType = "";

    // More-specific payload identities win over advertised-name annotations.
    // Company ID must match as well, so a bare ASCII fragment is never enough.
    for (int i = 0; KNOWN_MANUFACTURER_PAYLOAD_RULES[i].manufacturer != nullptr; i++) {
        const KnownManufacturerPayloadRule& rule = KNOWN_MANUFACTURER_PAYLOAD_RULES[i];
        if (rule.companyId == 0xFFFF || companyId != rule.companyId) continue;
        if (!rule.manufacturerHexPattern || !manufacturerHex.length()) continue;
        if (!containsIgnoreCase(manufacturerHex, rule.manufacturerHexPattern)) continue;

        assumedDevice = rule.assumedDeviceId ? rule.assumedDeviceId : "";
        assumedType = rule.deviceType ? rule.deviceType : "";
        return true;
    }

    for (int i = 0; KNOWN_DEVICES[i].manufacturer != nullptr; i++) {
        const KnownDeviceRule& rule = KNOWN_DEVICES[i];
        bool matched = false;

        switch (rule.matchMode) {
            case KNOWN_PREFIX:
                matched = name.length() &&
                          startsWithIgnoreCase(name, rule.advertisedPattern);
                break;
            case KNOWN_EXACT:
                matched = name.length() && rule.advertisedPattern &&
                          name.equalsIgnoreCase(rule.advertisedPattern);
                break;
            case KNOWN_COMPANY_ID_ONLY:
                matched = (rule.companyId != 0xFFFF && companyId == rule.companyId);
                break;
            case KNOWN_CONTAINS:
            default:
                matched = name.length() && rule.advertisedPattern &&
                          containsIgnoreCase(name, rule.advertisedPattern);
                break;
        }

        if (!matched) continue;
        if (rule.companyId != 0xFFFF &&
            rule.matchMode != KNOWN_COMPANY_ID_ONLY &&
            companyId != rule.companyId) {
            continue;
        }

        assumedDevice = rule.assumedDeviceId ? rule.assumedDeviceId : "";
        assumedType = rule.deviceType ? rule.deviceType : "";
        return true;
    }
    return false;
}

AssumedDeviceInfo classifyAssumedDevice(const String& name,
                                        uint16_t companyId,
                                        const String& manufacturerHex,
                                        const String& serviceUUIDs) {
    const char* fpDevice = "";
    const char* fpType = "";
    bool falsePositive = matchFalsePositive(name, fpDevice, fpType, nullptr);

    const char* knownDevice = "";
    const char* knownType = "";
    if (matchKnownDevice(name, companyId, manufacturerHex, knownDevice, knownType)) {
        return { knownDevice, knownType, falsePositive };
    }

    if (falsePositive) {
        return { fpDevice, fpType, true };
    }

    // v6.2 Session 06 service UUID references are context only. They are
    // considered only after stronger known-device/false-positive rules and
    // never feed the smart-glasses confidence engine.
    const BleServiceReference* serviceRef = serviceReferenceForUuidString(serviceUUIDs);
    if (serviceRef != nullptr) {
        if (name.length()) {
            return { name.c_str(), serviceRef->contextType, false };
        }
        return { serviceRef->displayName, serviceRef->contextType, false };
    }

    // If no defensible lookup matches, preserve the raw advertised name as the
    // assumed device ID. The pointer remains valid while `name` remains alive.
    return { name.length() ? name.c_str() : "", "", false };
}

const char* tierName(uint8_t tier) {
    if (tier == TIER_HIGH) return "HIGH";
    if (tier == TIER_MEDIUM) return "MEDIUM";
    return "LOW";
}

const char* alertLevelForConfidence(uint8_t confidence) {
    if (confidence >= HIGH_ALERT_CONFIDENCE) return "HIGH";
    if (confidence >= POSSIBLE_CONFIDENCE_MIN) return "POSSIBLE";
    if (confidence > 0) return "LOW";
    return "NONE";
}

// Company-name lookup is annotation only. Session 06 reference additions are
// checked first so ordinary manufacturers can be named without turning them
// into smart-glasses confidence rules. Existing LOW Company-ID clues remain
// available as a fallback.
const char* companyNameForId(uint16_t id) {
    for (int i = 0; BLE_COMPANY_REFERENCES[i].companyName != nullptr; i++) {
        if (BLE_COMPANY_REFERENCES[i].companyId == id) {
            return BLE_COMPANY_REFERENCES[i].companyName;
        }
    }

    for (int i = 0; LOW_CONFIDENCE_RULES[i].company != nullptr; i++) {
        if (LOW_CONFIDENCE_RULES[i].companyId == (int32_t)id) {
            return LOW_CONFIDENCE_RULES[i].company;
        }
    }
    return "UNKNOWN";
}

bool serviceUuidStringMatches16(const String& serviceUUIDs, uint16_t uuid16) {
    if (!serviceUUIDs.length()) return false;

    char shortUuid[5];
    snprintf(shortUuid, sizeof(shortUuid), "%04x", uuid16);

    String value = serviceUUIDs;
    value.trim();
    value.toLowerCase();

    if (value.length() == 4) {
        return value.equals(shortUuid);
    }

    String basePrefix = String("0000") + shortUuid + "-";
    return value.startsWith(basePrefix);
}

const BleServiceReference* serviceReferenceForUuidString(const String& serviceUUIDs) {
    for (int i = 0; BLE_SERVICE_REFERENCES[i].displayName != nullptr; i++) {
        if (serviceUuidStringMatches16(serviceUUIDs, BLE_SERVICE_REFERENCES[i].uuid16)) {
            return &BLE_SERVICE_REFERENCES[i];
        }
    }
    return nullptr;
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

String getServiceUUIDs(BLEAdvertisedDevice& device) {
    String out = "";
    if (device.haveServiceUUID()) {
        out = device.getServiceUUID().toString().c_str();
    }
    return out;
}

const char* addressTypeLabel(uint8_t type) {
    switch (type) {
        case 0: return "PUBLIC";
        case 1: return "RANDOM";
        case 2: return "RPA_PUBLIC";
        case 3: return "RPA_RANDOM";
        default: return "UNKNOWN";
    }
}

const char* cameraStatusLabel(const DetectionResult& result) {
    if (!result.cameraKnown) return "UNKNOWN";
    return result.hasCamera ? "LIKELY" : "NOT LIKELY";
}

void copyToBuf(char* dst, size_t dstSize, const String& value) {
    if (dstSize == 0) return;
    value.toCharArray(dst, dstSize);
    dst[dstSize - 1] = '\0';
}

enum ManualCameraStatusCode : uint8_t {
    MANUAL_CAMERA_NONE = 0,
    MANUAL_CAMERA_UNKNOWN,
    MANUAL_CAMERA_LIKELY,
    MANUAL_CAMERA_NOT_LIKELY
};

uint8_t manualCameraStatusCode(const DetectionResult* result) {
    if (result == nullptr || !result->matched) return MANUAL_CAMERA_NONE;
    if (!result->cameraKnown) return MANUAL_CAMERA_UNKNOWN;
    return result->hasCamera ? MANUAL_CAMERA_LIKELY : MANUAL_CAMERA_NOT_LIKELY;
}

const char* manualCameraStatusLabel(uint8_t code) {
    switch (code) {
        case MANUAL_CAMERA_UNKNOWN:    return "UNKNOWN";
        case MANUAL_CAMERA_LIKELY:     return "LIKELY";
        case MANUAL_CAMERA_NOT_LIKELY: return "NOT LIKELY";
        case MANUAL_CAMERA_NONE:
        default:                       return "";
    }
}

void fillManualBLEObservation(const BLELogSnapshot& snapshot,
                              bool detected,
                              const DetectionResult* result,
                              ManualBLEObservation& d) {
    memset(&d, 0, sizeof(d));
    d.observedAtMs = millis();

    formatDeviceMacHashForAddress(
        snapshot.address, d.deviceMacHash, sizeof(d.deviceMacHash)
    );
    copyToBuf(d.name, sizeof(d.name), snapshot.name);
    d.addressType = snapshot.addressType;
    d.companyId = snapshot.companyId;
    copyToBuf(d.manufacturerHex, sizeof(d.manufacturerHex),
              snapshot.manufacturerHex);
    copyToBuf(d.serviceUUID, sizeof(d.serviceUUID),
              snapshot.serviceUUIDs);

    d.detectedByScanner = detected;
    d.product = (result && result->matched) ? result->product : nullptr;
    d.cameraStatusCode = manualCameraStatusCode(result);
    d.tier = (result && result->matched) ? result->tier : TIER_LOW;
    d.confidence = (result && result->matched) ? result->confidence : 0;
}

void rememberManualObservation(const BLELogSnapshot& snapshot,
                               bool detected,
                               const DetectionResult* result) {
    if (manualContextMutex == nullptr) return;

    ManualBLEObservation observation = {};
    fillManualBLEObservation(snapshot, detected, result, observation);

    if (xSemaphoreTake(manualContextMutex, pdMS_TO_TICKS(20)) != pdTRUE) return;

    // If a manual event is armed, the next 5 observations are copied in exact
    // arrival order. Repeated addresses are intentionally retained.
    if (manualContextCaptureActive &&
        manualContextAfterCount < MANUAL_CONTEXT_AFTER_COUNT) {
        manualContextAfter[manualContextAfterCount] = observation;
        manualContextAfterCount++;

        if (manualContextAfterCount >= MANUAL_CONTEXT_AFTER_COUNT) {
            manualContextCaptureActive = false;
            manualContextReadyToWrite = true;
        }
    }

    // Freeze the BEFORE ring for the whole active/pending manual event so an SD
    // retry cannot overwrite the pre-event context. Otherwise maintain the most
    // recent five observations for the next event.
    if (!manualContextCaptureActive &&
        !manualContextReadyToWrite &&
        !manualContextWriteInProgress) {
        manualBeforeRing[manualBeforeRingNext] = observation;
        manualBeforeRingNext =
            (manualBeforeRingNext + 1) % MANUAL_CONTEXT_BEFORE_COUNT;
        if (manualBeforeRingCount < MANUAL_CONTEXT_BEFORE_COUNT) {
            manualBeforeRingCount++;
        }
    }

    xSemaphoreGive(manualContextMutex);
}

bool armManualContextCapture(String& eventIdOut) {
    if (manualContextMutex == nullptr) return false;
    if (xSemaphoreTake(manualContextMutex, pdMS_TO_TICKS(50)) != pdTRUE) return false;

    if (manualContextCaptureActive ||
        manualContextReadyToWrite ||
        manualContextWriteInProgress ||
        manualBeforeRingCount < MANUAL_CONTEXT_BEFORE_COUNT) {
        xSemaphoreGive(manualContextMutex);
        return false;
    }

    manualContextEventCounter++;
    snprintf(manualContextEventId, sizeof(manualContextEventId),
             "%s-M%04lu", sessionId,
             (unsigned long)manualContextEventCounter);

    // manualBeforeRingNext points to the oldest observation when the ring is full.
    // Save that index and freeze the ring until this event is successfully written.
    manualBeforeFrozenStart = manualBeforeRingNext;

    memset(manualContextAfter, 0, sizeof(manualContextAfter));
    manualContextAfterCount = 0;
    manualContextReadyToWrite = false;
    manualContextCaptureActive = true;
    eventIdOut = String(manualContextEventId);

    xSemaphoreGive(manualContextMutex);
    return true;
}

uint32_t fnv1a32(const String& value, uint32_t hash = 2166136261UL) {
    for (size_t i = 0; i < value.length(); i++) {
        hash ^= (uint8_t)value[i];
        hash *= 16777619UL;
    }
    return hash;
}

bool rememberKnownNonGlassesDevice(const String& address) {
    if (!address.length()) return false;

    uint32_t hash = sessionDeviceHashForAddress(address);

    for (uint16_t i = 0; i < knownNonGlassesDeviceCount; i++) {
        if (knownNonGlassesDeviceHashes[i] == hash) {
            return false;
        }
    }

    if (knownNonGlassesDeviceCount >= MAX_KNOWN_NON_GLASSES_DEVICES) {
        return false;
    }

    knownNonGlassesDeviceHashes[knownNonGlassesDeviceCount++] = hash;
    return true;
}

void clearSentryActivityWindow() {
    if (sentryActivityMutex &&
        xSemaphoreTake(sentryActivityMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return;
    }

    memset(sentryWindowDeviceHashes, 0, sizeof(sentryWindowDeviceHashes));
    sentryWindowUniqueCount = 0;

    if (sentryActivityMutex) xSemaphoreGive(sentryActivityMutex);
}

uint16_t snapshotAndResetSentryActivityWindow() {
    uint16_t count = 0;

    if (sentryActivityMutex &&
        xSemaphoreTake(sentryActivityMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return sentryWindowUniqueCount;
    }

    count = sentryWindowUniqueCount;
    memset(sentryWindowDeviceHashes, 0, sizeof(sentryWindowDeviceHashes));
    sentryWindowUniqueCount = 0;

    if (sentryActivityMutex) xSemaphoreGive(sentryActivityMutex);
    return count;
}

void recordSentryActivityObservation(const String& address) {
    if (!sentryModeActive || !address.length()) return;
    if (sentryState != SENTRY_STATE_SAMPLE && sentryState != SENTRY_STATE_ACTIVE) return;

    uint32_t hash = sessionDeviceHashForAddress(address);

    if (sentryActivityMutex &&
        xSemaphoreTake(sentryActivityMutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return;
    }

    for (uint16_t i = 0; i < sentryWindowUniqueCount; i++) {
        if (sentryWindowDeviceHashes[i] == hash) {
            if (sentryActivityMutex) xSemaphoreGive(sentryActivityMutex);
            return;
        }
    }

    if (sentryWindowUniqueCount < MAX_SENTRY_WINDOW_UNIQUE_DEVICES) {
        sentryWindowDeviceHashes[sentryWindowUniqueCount++] = hash;
    }

    if (sentryActivityMutex) xSemaphoreGive(sentryActivityMutex);
    sentryDashboardDirty = true;
}

void recordSentryDeviceObservation(const String& address, bool knownIdentity) {
    if (!address.length()) return;

    // Activity-window counting is independent of the 24-hour session dedupe.
    recordSentryActivityObservation(address);

    uint32_t hash = sessionDeviceHashForAddress(address);

    for (uint16_t i = 0; i < sentryIndividualDeviceCount; i++) {
        if (sentryUniqueDeviceHashes[i] == hash) {
            sentryDuplicateDeviceCount++;
            if (knownIdentity && !sentryUniqueDeviceKnown[i]) {
                sentryUniqueDeviceKnown[i] = 1;
                sentryKnownDeviceCount++;
            }
            sentryDashboardDirty = true;
            return;
        }
    }

    if (sentryIndividualDeviceCount < MAX_SENTRY_UNIQUE_DEVICES) {
        uint16_t slot = sentryIndividualDeviceCount++;
        sentryUniqueDeviceHashes[slot] = hash;
        sentryUniqueDeviceKnown[slot] = knownIdentity ? 1 : 0;
        if (knownIdentity) sentryKnownDeviceCount++;
        sentryDashboardDirty = true;
    }
}

uint32_t makeAdvertisementSignature(const String& address,
                                    const String& name,
                                    uint16_t cid,
                                    const String& mfgHex,
                                    const String& serviceUUIDs) {
    // Session-scoped seed prevents the in-RAM dedupe token from remaining
    // stable across sessions while preserving advertisement-level dedupe.
    uint32_t hash = sessionDeviceHashForAddress(address);
    hash = fnv1a32("|", hash);
    hash = fnv1a32(name, hash);
    hash = fnv1a32("|", hash);
    hash ^= cid;
    hash *= 16777619UL;
    hash = fnv1a32("|", hash);
    hash = fnv1a32(mfgHex, hash);
    hash = fnv1a32("|", hash);
    hash = fnv1a32(serviceUUIDs, hash);

    // Zero is reserved for an unused cache slot.
    if (hash == 0) hash = 1;
    return hash;
}

bool isAdvertisementAlreadyLogged(uint32_t signature) {
    for (uint16_t i = 0; i < seenAdvSignatureCount; i++) {
        if (seenAdvSignatures[i] == signature) return true;
    }
    return false;
}

void rememberLoggedAdvertisement(uint32_t signature) {
    if (seenAdvSignatureCount < MAX_ADV_SIGNATURES) {
        seenAdvSignatures[seenAdvSignatureCount++] = signature;
    } else {
        // Fixed memory usage: once full, replace the oldest cache slot.
        seenAdvSignatures[seenAdvSignatureReplace] = signature;
        seenAdvSignatureReplace =
            (seenAdvSignatureReplace + 1) % MAX_ADV_SIGNATURES;
    }
}

// ---------------- SD logging ----------------

const char* sdStatusText() {
    switch (sdStatus) {
        case SD_STATUS_DETECTED:     return "SD DETECTED";
        case SD_STATUS_FAIL:         return "SD FAIL";
        case SD_STATUS_EJECTED:      return "SD EJECTED - SAFE TO REMOVE";
        case SD_STATUS_NOT_DETECTED:
        default:                     return "SD NOT DETECTED";
    }
}

uint16_t sdStatusColor() {
    if (sdStatus == SD_STATUS_DETECTED) return TFT_GREEN;
    if (sdStatus == SD_STATUS_EJECTED) return TFT_CYAN;
    return TFT_RED;
}

SDStatus initSD() {
    if (!sdSPIStarted) {
        sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
        sdSPIStarted = true;
    }

    SD.end();

    if (!SD.begin(SD_CS, sdSPI, 20000000)) {
        return SD_STATUS_NOT_DETECTED;
    }

    if (SD.cardType() == CARD_NONE) {
        SD.end();
        return SD_STATUS_NOT_DETECTED;
    }

    const char* advHeader =
        "session_id,uptime_ms,device_mac_hash,address_type,advertised_name,assumed_device_id,assumed_device_type,rssi,scan_rating,approx_distance_m,company_id_hex,company_name,false_positive,manufacturer_data_hex,service_uuid,detected,product,tier,confidence,alert_level";
    const char* detHeader =
        "session_id,uptime_ms,device_mac_hash,advertised_name,assumed_device_id,assumed_device_type,rssi,scan_rating,approx_distance_m,company_id_hex,company_name,product,tier,confidence,alert_level,camera_status,reason,manufacturer_data_hex,service_uuid";
    const char* manualHeader =

        "session_id,manual_event_id,context_position,context_index,uptime_ms,device_mac_hash,address_type,advertised_name,assumed_device_id,assumed_device_type,company_id_hex,company_name,manufacturer_data_hex,service_uuid,detected_by_scanner,product,camera_status,tier,confidence,alert_level,manual_log";
    const char* sessionsHeader =
        "session_id,session_number,start_uptime_ms,reason";

    if (!SD.exists("/advertisements.csv")) {
        File f = SD.open("/advertisements.csv", FILE_WRITE);
        if (!f) return SD_STATUS_FAIL;
        f.println(advHeader);
        f.close();
    }
    if (!SD.exists("/detections.csv")) {
        File f = SD.open("/detections.csv", FILE_WRITE);
        if (!f) return SD_STATUS_FAIL;
        f.println(detHeader);
        f.close();
    }
    if (!SD.exists("/manual_smart_glasses_logged.csv")) {
        File f = SD.open("/manual_smart_glasses_logged.csv", FILE_WRITE);
        if (!f) return SD_STATUS_FAIL;
        f.println(manualHeader);
        f.close();
    }
    if (!SD.exists("/sessions.csv")) {
        File f = SD.open("/sessions.csv", FILE_WRITE);
        if (!f) return SD_STATUS_FAIL;
        f.println(sessionsHeader);
        f.close();
    }

    File a = SD.open("/advertisements.csv", FILE_APPEND);
    if (!a) return SD_STATUS_FAIL;
    a.close();
    File d = SD.open("/detections.csv", FILE_APPEND);
    if (!d) return SD_STATUS_FAIL;
    d.close();
    File m = SD.open("/manual_smart_glasses_logged.csv", FILE_APPEND);
    if (!m) return SD_STATUS_FAIL;
    m.close();
    File ss = SD.open("/sessions.csv", FILE_APPEND);
    if (!ss) return SD_STATUS_FAIL;
    ss.close();

    return SD_STATUS_DETECTED;
}

void markSDWriteFailure() {
    sdSafelyEjected = false;
    sdOK = false;
    sdStatus = SD_STATUS_FAIL;
    sentryDashboardDirty = true;
    SD.end();
}

bool takeSD(uint32_t timeoutMs = 1000) {
    if (sdMutex == nullptr) return true;
    return xSemaphoreTake(sdMutex, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

void giveSD() {
    if (sdMutex != nullptr) xSemaphoreGive(sdMutex);
}

void writeSessionStartRecord(const char* reason = "MOUNT") {
    if (!sdOK || sdSafelyEjected) return;
    if (!takeSD()) return;

    File f = SD.open("/sessions.csv", FILE_APPEND);
    if (!f) {
        giveSD();
        markSDWriteFailure();
        return;
    }

    f.clearWriteError();
    f.print(sessionId); f.print(",");
    f.print(sessionNumber); f.print(",");
    f.print(millis()); f.print(",");
    writeCsvField(f, reason ? reason : "");
    f.println();
    f.flush();
    bool ok = (f.getWriteError() == 0);
    f.close();
    giveSD();

    if (!ok) markSDWriteFailure();
}

void resetSessionRuntimeState() {
    memset(seenAdvSignatures, 0, sizeof(seenAdvSignatures));
    seenAdvSignatureCount = 0;
    seenAdvSignatureReplace = 0;

    memset(trackedDevices, 0, sizeof(trackedDevices));
    trackedDeviceCount = 0;

    memset(knownNonGlassesDeviceHashes, 0, sizeof(knownNonGlassesDeviceHashes));
    knownNonGlassesDeviceCount = 0;

    memset(sentryUniqueDeviceHashes, 0, sizeof(sentryUniqueDeviceHashes));
    memset(sentryUniqueDeviceKnown, 0, sizeof(sentryUniqueDeviceKnown));
    sentryIndividualDeviceCount = 0;
    sentryKnownDeviceCount = 0;
    sentryDuplicateDeviceCount = 0;
    sentrySessionScanCount = 0;
    sentryDashboardDirty = true;

    totalSuspiciousDeviceReviews = 0;
    totalMicCamReviews = 0;
    lastSuspiciousAlertDeviceMacHash = "";
    lastSuspiciousAlertMs = 0;
    lastMicCamAlertDeviceMacHash = "";
    lastMicCamAlertMs = 0;
    clearMicCamAlert();
    clearSuspiciousAlert();

    if (manualContextMutex && xSemaphoreTake(manualContextMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        memset(manualBeforeRing, 0, sizeof(manualBeforeRing));

        memset(manualContextAfter, 0, sizeof(manualContextAfter));
        manualBeforeRingCount = 0;
        manualBeforeRingNext = 0;
        manualBeforeFrozenStart = 0;
        manualContextCaptureActive = false;
        manualContextReadyToWrite = false;
        manualContextWriteInProgress = false;
        manualContextAfterCount = 0;
        manualContextEventCounter = 0;
        manualContextEventId[0] = '\0';
        xSemaphoreGive(manualContextMutex);
    }
}

void checkSession24HourReset() {
    uint32_t now = millis();
    if ((uint32_t)(now - sessionPeriodStartedMs) < SESSION_RESET_MS) return;

    // Avoid changing session identity while an asynchronous BLE callback may be
    // actively writing an advertisement.
    if (pBLEScan != nullptr && pBLEScan->isScanning()) return;

    sessionNumber = 1;
    sessionPrefs.begin("glassscan", false);
    sessionPrefs.putUInt("session", sessionNumber);
    sessionPrefs.end();

    formatSessionId();
    regenerateSessionDeviceSalt();
    sessionPeriodStartedMs = now;
    resetSessionRuntimeState();

    if (sdOK && !sdSafelyEjected) {
        writeSessionStartRecord("24H_RESET");
    }

    Serial.printf("SESSION RESET 24H: %s\n", sessionId);
    if (!warningScreenActive && !micCamAlert.active && !suspiciousAlert.active && !manualLogNoticeActive && !selfTestActive) {
        showSelectedPage();
    }
}

bool safeEjectSD() {
    if (sdSafelyEjected) return true;
    if (!takeSD(1500)) return false;

    // Every logger uses open/write/flush/close. Taking this mutex means
    // there is no active SD write when the filesystem is released.
    SD.end();
    sdOK = false;
    sdSafelyEjected = true;
    sdRemovalSeenAfterSafeEject = false;
    sdStatus = SD_STATUS_EJECTED;
    lastSDCheckMs = millis();

    giveSD();
    Serial.println("SD safely ejected - safe to remove.");
    return true;
}

bool remountEjectedSD() {
    if (!sdSafelyEjected) return false;

    sdSafelyEjected = false;
    sdRemovalSeenAfterSafeEject = false;
    sdStatus = initSD();
    sdOK = (sdStatus == SD_STATUS_DETECTED);
    lastSDCheckMs = millis();

    if (sdOK) {
        writeSessionStartRecord();
        Serial.println("SD remounted.");
        return true;
    }

    return false;
}

// v6.1 SD presence / reinsertion handling.
//
// There is no dedicated microSD card-detect switch on the CYD SD interface, so
// presence is verified over SPI. All checks are done between BLE scans and use
// the SD mutex so they cannot collide with a logger write.
//
// Mounted card: verify it is still present every 10 seconds.
// Missing card: attempt a remount every 10 seconds.
// Safe eject: remain unmounted while the card is still inserted. Once absence
// is observed, wait for a later probe to see the card reinserted, then remount
// automatically.

bool checkMountedSDStillPresent() {
    if (!sdOK || sdSafelyEjected) return false;
    if (!takeSD(1000)) return true;  // Busy is not the same as missing.

    // Force a real, read-only sector transaction every 10 seconds. This is a
    // stronger presence check than SD.cardType(), which can remain cached after
    // a card has been physically removed. Sector 0 is only read, never changed.
    bool present = SD.readRAW(sdPresenceProbeBuffer, 0);
    if (!present) {
        SD.end();
        sdOK = false;
        sdStatus = SD_STATUS_NOT_DETECTED;
    }

    giveSD();
    return present;
}

bool probeUnmountedSDPresent() {
    if (!takeSD(1000)) return false;

    if (!sdSPIStarted) {
        sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
        sdSPIStarted = true;
    }

    // Probe only: mount/read card type, then immediately release it again.
    // No files are opened or written by this probe.
    SD.end();
    bool began = SD.begin(SD_CS, sdSPI, 20000000);
    bool present = began && (SD.cardType() != CARD_NONE);
    SD.end();

    giveSD();
    return present;
}

bool refreshSDStatus(bool force = false) {
    uint32_t now = millis();
    if (!force && (uint32_t)(now - lastSDCheckMs) < SD_CHECK_INTERVAL_MS) {
        return false;
    }
    lastSDCheckMs = now;

    SDStatus oldStatus = sdStatus;
    bool oldOK = sdOK;
    bool oldSafelyEjected = sdSafelyEjected;

    // SAFE EJECT state: do not remount just because the card is still present.
    // First observe physical removal, then auto-remount when a later probe sees
    // the card present again.
    if (sdSafelyEjected) {
        bool present = probeUnmountedSDPresent();

        if (!present) {
            sdRemovalSeenAfterSafeEject = true;
            sdStatus = SD_STATUS_NOT_DETECTED;
            sdOK = false;
        } else if (sdRemovalSeenAfterSafeEject) {
            sdSafelyEjected = false;
            sdRemovalSeenAfterSafeEject = false;
            sdStatus = initSD();
            sdOK = (sdStatus == SD_STATUS_DETECTED);

            if (sdOK) {
                writeSessionStartRecord("REINSERT");
                Serial.println("SD reinserted and remounted automatically.");
            }
        } else {
            // Card has been safely ejected but has not yet been physically
            // removed. Keep it unmounted and safe to remove.
            sdStatus = SD_STATUS_EJECTED;
            sdOK = false;
        }
    }
    else if (sdOK) {
        // Normal mounted state: verify physical presence every 10 seconds.
        checkMountedSDStillPresent();
    }
    else {
        // No usable card: check for insertion/remount every 10 seconds.
        sdStatus = initSD();
        sdOK = (sdStatus == SD_STATUS_DETECTED);
        if (sdOK && !force) {
            writeSessionStartRecord("MOUNT");
            Serial.println("SD detected and mounted automatically.");
        }
    }

    bool changed = (sdStatus != oldStatus) ||
                   (sdOK != oldOK) ||
                   (sdSafelyEjected != oldSafelyEjected);

    if (changed) {
        Serial.printf("SD status changed: %s\n", sdStatusText());
    }

    return changed;
}

void logAdvertisement(const BLELogSnapshot& snapshot, bool detected,
                      const DetectionResult* result) {
    totalAdvertisements++;

    const String& address = snapshot.address;
    const String& name = snapshot.name;
    const String& mfgHex = snapshot.manufacturerHex;
    const String& serviceUUIDs = snapshot.serviceUUIDs;
    int rssi = snapshot.rssi;
    uint16_t cid = snapshot.companyId;

    AssumedDeviceInfo assumed = classifyAssumedDevice(name, cid, mfgHex, serviceUUIDs);
    bool falsePositive = assumed.falsePositive;

    char deviceMacHash[DEVICE_MAC_HASH_LEN];
    formatDeviceMacHashForAddress(address, deviceMacHash, sizeof(deviceMacHash));

    lastRSSI = rssi;

    uint32_t signature =
        makeAdvertisementSignature(address, name, cid, mfgHex, serviceUUIDs);

    if (isAdvertisementAlreadyLogged(signature)) return;

    // No SD / safely ejected SD: observe normally, but never poison dedupe.
    if (!sdOK || sdSafelyEjected) return;
    if (!takeSD()) return;

    File f = SD.open("/advertisements.csv", FILE_APPEND);
    if (!f) {
        giveSD();
        markSDWriteFailure();
        return;
    }

    f.clearWriteError();

    char cidHex[7] = "";
    if (cid != 0xFFFF) snprintf(cidHex, sizeof(cidHex), "0x%04X", cid);

    // v6.1 schema: assumed_device_id sits immediately beside advertised_name.
    f.print(sessionId); f.print(",");
    f.print(millis()); f.print(",");
    writeCsvField(f, deviceMacHash); f.print(",");
    writeCsvField(f, addressTypeLabel(snapshot.addressType)); f.print(",");
    writeCsvField(f, name); f.print(",");
    writeCsvField(f, assumed.deviceId); f.print(",");
    writeCsvField(f, assumed.deviceType); f.print(",");
    f.print(rssi); f.print(",");
    writeCsvField(f, scanRatingForRSSI(rssi)); f.print(",");
    f.print(approximateDistanceMetres(rssi), 2); f.print(",");
    writeCsvField(f, cid == 0xFFFF ? "" : cidHex); f.print(",");
    writeCsvField(f, cid == 0xFFFF ? "" : companyNameForId(cid)); f.print(",");
    f.print(falsePositive ? "1" : "0"); f.print(",");
    writeCsvField(f, mfgHex); f.print(",");
    writeCsvField(f, serviceUUIDs); f.print(",");
    f.print((detected && !falsePositive) ? "1" : "0"); f.print(",");
    writeCsvField(f, (result && result->matched && result->product) ? result->product : ""); f.print(",");
    writeCsvField(f, (result && result->matched) ? tierName(result->tier) : ""); f.print(",");
    f.print((result && result->matched) ? result->confidence : 0); f.print(",");
    writeCsvField(f, (result && result->matched) ? alertLevelForConfidence(result->confidence) : "NONE");
    f.println();

    f.flush();
    bool writeOK = (f.getWriteError() == 0);
    f.close();

    if (!writeOK) {
        giveSD();
        markSDWriteFailure();
        return;
    }

    // Only successful persisted advertisements enter the dedupe cache.
    rememberLoggedAdvertisement(signature);
    totalUniqueAdvertisements++;
    giveSD();
}

void logDetection(const BLELogSnapshot& snapshot, const DetectionResult& result) {
    if (!sdOK || sdSafelyEjected) return;
    if (result.confidence < POSSIBLE_CONFIDENCE_MIN) return;

    uint16_t cid = snapshot.companyId;
    const String& name = snapshot.name;
    const String& mfgHex = snapshot.manufacturerHex;
    const String& serviceUUIDs = snapshot.serviceUUIDs;
    int rssi = snapshot.rssi;

    AssumedDeviceInfo assumed = classifyAssumedDevice(name, cid, mfgHex, serviceUUIDs);
    bool falsePositive = assumed.falsePositive;
    if (falsePositive) return;

    char cidHex[7] = "";
    if (cid != 0xFFFF) snprintf(cidHex, sizeof(cidHex), "0x%04X", cid);

    if (!takeSD()) return;
    File f = SD.open("/detections.csv", FILE_APPEND);
    if (!f) {
        giveSD();
        markSDWriteFailure();
        return;
    }

    f.clearWriteError();

    char deviceMacHash[DEVICE_MAC_HASH_LEN];
    formatDeviceMacHashForAddress(
        snapshot.address, deviceMacHash, sizeof(deviceMacHash)
    );

    f.print(sessionId); f.print(",");
    f.print(millis()); f.print(",");
    writeCsvField(f, deviceMacHash); f.print(",");
    writeCsvField(f, name); f.print(",");
    writeCsvField(f, assumed.deviceId); f.print(",");
    writeCsvField(f, assumed.deviceType); f.print(",");
    f.print(rssi); f.print(",");
    writeCsvField(f, scanRatingForRSSI(rssi)); f.print(",");
    f.print(approximateDistanceMetres(rssi), 2); f.print(",");
    writeCsvField(f, cid == 0xFFFF ? "" : cidHex); f.print(",");
    writeCsvField(f, result.company ? result.company : ""); f.print(",");
    writeCsvField(f, result.product ? result.product : ""); f.print(",");
    writeCsvField(f, tierName(result.tier)); f.print(",");
    f.print(result.confidence); f.print(",");
    writeCsvField(f, alertLevelForConfidence(result.confidence)); f.print(",");
    writeCsvField(f, cameraStatusLabel(result)); f.print(",");
    writeCsvField(f, result.reason ? result.reason : ""); f.print(",");
    writeCsvField(f, mfgHex); f.print(",");
    writeCsvField(f, serviceUUIDs);
    f.println();

    f.flush();
    bool writeOK = (f.getWriteError() == 0);
    f.close();

    if (!writeOK) {
        giveSD();
        markSDWriteFailure();
        return;
    }

    giveSD();
}

void writeManualContextRow(File& f,
                           const ManualBLEObservation& d,
                           const char* eventId,
                           const char* position,
                           uint8_t contextIndex) {
    String manualName(d.name);
    String manualManufacturerHex(d.manufacturerHex);
    String manualServiceUUID(d.serviceUUID);
    AssumedDeviceInfo assumed = classifyAssumedDevice(
        manualName, d.companyId, manualManufacturerHex, manualServiceUUID
    );

    char cidHex[7] = "";
    if (d.companyId != 0xFFFF) {
        snprintf(cidHex, sizeof(cidHex), "0x%04X", d.companyId);
    }

    f.print(sessionId); f.print(",");
    writeCsvField(f, eventId); f.print(",");
    writeCsvField(f, position); f.print(",");
    f.print(contextIndex); f.print(",");
    f.print(d.observedAtMs); f.print(",");

    writeCsvField(f, d.deviceMacHash); f.print(",");
    writeCsvField(f, addressTypeLabel(d.addressType)); f.print(",");
    writeCsvField(f, d.name); f.print(",");

    writeCsvField(f, assumed.deviceId); f.print(",");
    writeCsvField(f, assumed.deviceType); f.print(",");
    writeCsvField(f, d.companyId == 0xFFFF ? "" : cidHex); f.print(",");
    writeCsvField(f, d.companyId == 0xFFFF ? "" : companyNameForId(d.companyId)); f.print(",");
    writeCsvField(f, d.manufacturerHex); f.print(",");
    writeCsvField(f, d.serviceUUID); f.print(",");
    f.print(d.detectedByScanner ? "1" : "0"); f.print(",");
    writeCsvField(f, d.product ? d.product : ""); f.print(",");
    writeCsvField(f, manualCameraStatusLabel(d.cameraStatusCode)); f.print(",");
    writeCsvField(f, d.confidence > 0 ? tierName(d.tier) : ""); f.print(",");
    f.print(d.confidence); f.print(",");
    writeCsvField(f, alertLevelForConfidence(d.confidence)); f.print(",");
    f.println("1");
}

bool writeManualContextEvent() {
    if (!sdOK || sdSafelyEjected) return false;
    if (!takeSD()) return false;

    File f = SD.open("/manual_smart_glasses_logged.csv", FILE_APPEND);
    if (!f) {
        giveSD();
        markSDWriteFailure();
        return false;
    }

    f.clearWriteError();

    // Write the full event as one grouped append: BEFORE 1..5 then AFTER 1..5.
    for (uint8_t i = 0; i < MANUAL_CONTEXT_BEFORE_COUNT; i++) {
        uint8_t slot = (manualBeforeFrozenStart + i) % MANUAL_CONTEXT_BEFORE_COUNT;
        writeManualContextRow(
            f, manualBeforeRing[slot], manualContextEventId, "BEFORE", i + 1
        );
    }
    for (uint8_t i = 0; i < MANUAL_CONTEXT_AFTER_COUNT; i++) {
        writeManualContextRow(
            f, manualContextAfter[i], manualContextEventId, "AFTER", i + 1
        );
    }

    f.flush();
    bool writeOK = (f.getWriteError() == 0);
    f.close();

    if (!writeOK) {
        giveSD();
        markSDWriteFailure();
        return false;
    }

    giveSD();

    Serial.printf(
        "{\"type\":\"manual_context_log\","
        "\"session\":\"%s\",\"event\":\"%s\","
        "\"before\":%u,\"after\":%u,\"selection\":\"chronological\",\"ts\":%lu}\n",
        sessionId,
        manualContextEventId,
        (unsigned)MANUAL_CONTEXT_BEFORE_COUNT,
        (unsigned)MANUAL_CONTEXT_AFTER_COUNT,
        (unsigned long)millis()
    );

    return true;
}

// ---------------- CYD display ----------------

void screenBacklight(bool on) {
    analogWrite(TFT_BL, on ? 255 : 0);
    screenAwake = on;
    if (!on) {
        tft.fillScreen(TFT_BLACK);
    }
}

bool loggingHealthy() {
    return sdOK && !sdSafelyEjected && sdStatus == SD_STATUS_DETECTED;
}

uint16_t sentryUnknownDeviceCount() {
    return (sentryIndividualDeviceCount >= sentryKnownDeviceCount)
        ? (sentryIndividualDeviceCount - sentryKnownDeviceCount)
        : 0;
}

uint16_t sentryMedian(const uint16_t* values, uint16_t count) {
    if (count == 0) return 0;
    if (count > SENTRY_ACTIVE_SAMPLE_CAPACITY) count = SENTRY_ACTIVE_SAMPLE_CAPACITY;

    uint16_t sorted[SENTRY_ACTIVE_SAMPLE_CAPACITY];
    memcpy(sorted, values, (size_t)count * sizeof(uint16_t));

    for (uint16_t i = 1; i < count; i++) {
        uint16_t key = sorted[i];
        int j = (int)i - 1;
        while (j >= 0 && sorted[j] > key) {
            sorted[j + 1] = sorted[j];
            j--;
        }
        sorted[j + 1] = key;
    }

    if (count & 1U) return sorted[count / 2];
    return (uint16_t)(((uint32_t)sorted[(count / 2) - 1] + sorted[count / 2]) / 2UL);
}

const char* sentryStateText() {
    switch (sentryState) {
        case SENTRY_STATE_SAMPLE: return "SAMPLING";
        case SENTRY_STATE_ACTIVE: return "ACTIVE";
        case SENTRY_STATE_SLEEP:
        default:                  return "SLEEPING";
    }
}

void formatCountdown(uint32_t remainingMs, char* out, size_t outLen) {
    uint32_t totalSeconds = (remainingMs + 999UL) / 1000UL;
    uint32_t hours = totalSeconds / 3600UL;
    uint32_t minutes = (totalSeconds % 3600UL) / 60UL;
    uint32_t seconds = totalSeconds % 60UL;

    if (hours > 0) {
        snprintf(out, outLen, "%02lu:%02lu:%02lu",
                 (unsigned long)hours,
                 (unsigned long)minutes,
                 (unsigned long)seconds);
    } else {
        snprintf(out, outLen, "%02lu:%02lu",
                 (unsigned long)minutes,
                 (unsigned long)seconds);
    }
}

void setSentryIdleBacklight() {
    analogWrite(TFT_BL, 0);
    screenAwake = false;
}

void drawLegalPrivacyNotice(bool includeCredits,
                            const char* actionLine1,
                            const char* actionLine2) {
    if (!takeUi(100)) return;

    analogWrite(TFT_BL, 255);
    screenAwake = true;

    tft.fillScreen(TFT_BLACK);
    tft.setTextWrap(false);
    tft.setTextColor(TFT_RED, TFT_BLACK);

    tft.setTextSize(2);
    tft.setCursor(26, 5);
    tft.print("LEGAL / PRIVACY NOTICE");

    tft.setTextSize(1);
    int y = 34;
    const int step = 11;
    const char* lines[] = {
        "THIS DEVICE SCANS",
        "PUBLIC BLE ADVERTISEMENTS.",
        "",
        "FOR LAWFUL AND AUTHORISED",
        "USE ONLY.",
        "",
        "BLE MATCHES IDENTIFY DEVICES,",
        "NOT PEOPLE, INTENT OR ACTIVITY.",
        "",
        "A MATCH DOES NOT PROVE",
        "RECORDING OR SURVEILLANCE.",
        "",
        "RAW OBSERVED MAC ADDRESSES",
        "ARE NOT PERSISTENTLY LOGGED."
    };

    for (const char* line : lines) {
        tft.setCursor(8, y);
        tft.print(line);
        y += step;
    }

    if (includeCredits) {
        tft.setCursor(8, 191);
        tft.print("GITHUB INSPIRATION USED:");
        tft.setCursor(8, 202);
        tft.print("ESP-GlassHole / glass-detect");
    }

    tft.setTextSize(1);
    tft.setCursor(8, includeCredits ? 216 : 202);
    tft.print(actionLine1);
    tft.setCursor(8, includeCredits ? 227 : 216);
    tft.print(actionLine2);

    tft.setCursor(284, 227);
    tft.print("DEV-L");

    giveUi();
}

void drawBootLegalNotice() {
    drawLegalPrivacyNotice(true, "TO BEGIN SCANNING:", "TOUCH THE SCREEN");
}

void drawSentryLegalNotice() {
    sentryDashboardAwake = true;
    sentryLegalNoticeVisible = true;
    sentryWakeStartedMs = millis();
    sentryDashboardDirty = false;
    drawLegalPrivacyNotice(false, "TO VIEW SENTRY STATUS:", "TOUCH THE SCREEN AGAIN");
}

void waitForBootTouchToStart() {
    // Ignore a touch already held during power-up; require a fresh press.
    while (digitalRead(TOUCH_IRQ_PIN) == LOW) delay(10);
    delay(80);

    for (;;) {
        if (digitalRead(TOUCH_IRQ_PIN) == LOW) {
            delay(40);
            if (digitalRead(TOUCH_IRQ_PIN) == LOW) break;
        }
        delay(10);
    }

    // Wait for release so the same touch cannot immediately trigger another UI action.
    while (digitalRead(TOUCH_IRQ_PIN) == LOW) delay(10);
    delay(80);
}

void drawSentryEntering() {
    if (!takeUi(100)) return;

    analogWrite(TFT_BL, SENTRY_ENTER_BACKLIGHT_PWM);
    screenAwake = true;

    tft.fillScreen(TFT_BLACK);
    tft.setTextWrap(false);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(61, 96);
    tft.print("ENTERING SENTRY MODE");

    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(92, 128);
    tft.print("30 MINUTE SLEEP STARTING");

    giveUi();
}

void drawSentryIdle() {
    if (!takeUi(100)) return;
    tft.fillScreen(TFT_BLACK);
    giveUi();

    sentryDashboardAwake = false;
    sentryLegalNoticeVisible = false;
    setSentryIdleBacklight();
}

void drawSentryDashboard() {
    if (!takeUi(100)) return;

    analogWrite(TFT_BL, 255);
    screenAwake = true;

    const bool logging = loggingHealthy();
    const uint16_t unknown = sentryUnknownDeviceCount();
    const uint16_t triggerThreshold = calculateSentryTriggerThreshold();
    const uint32_t now = millis();

    char timeBuf[16] = "--:--";
    if (sentryState == SENTRY_STATE_SLEEP) {
        uint32_t elapsed = now - sentryStateStartedMs;
        uint32_t remaining = elapsed >= SENTRY_SLEEP_MS ? 0 : SENTRY_SLEEP_MS - elapsed;
        formatCountdown(remaining, timeBuf, sizeof(timeBuf));
    } else if (sentryState == SENTRY_STATE_SAMPLE) {
        uint32_t elapsed = now - sentryStateStartedMs;
        uint32_t remaining = elapsed >= SENTRY_SAMPLE_MS ? 0 : SENTRY_SAMPLE_MS - elapsed;
        formatCountdown(remaining, timeBuf, sizeof(timeBuf));
    } else {
        uint32_t elapsed = now - sentryActiveStartedMs;
        uint32_t remaining = elapsed >= SENTRY_ACTIVE_MS ? 0 : SENTRY_ACTIVE_MS - elapsed;
        formatCountdown(remaining, timeBuf, sizeof(timeBuf));
    }

    tft.fillScreen(TFT_BLACK);
    tft.setTextWrap(false);

    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(8, 5);
    tft.print("SENTRY MODE");

    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(12, 31);
    tft.print("SESSION:");

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(71, 25);
    tft.print(sessionId);

    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(12, 51);
    tft.print("STATE: ");
    tft.print(sentryStateText());
    tft.setCursor(205, 51);
    tft.print(sentryState == SENTRY_STATE_ACTIVE ? "LEFT: " : "NEXT: ");
    tft.print(timeBuf);

    tft.setCursor(12, 67);
    if (sentryState == SENTRY_STATE_ACTIVE) {
        tft.printf("OLD:%u  MED:%u  CUR:%u",
                   (unsigned)sentryOldBaseline,
                   (unsigned)sentryProvisionalMedian,
                   (unsigned)sentryWindowUniqueCount);
    } else if (sentryBaselineValid) {
        tft.printf("BASE:%u  LAST:%u  TRIGGER:>%u",
                   (unsigned)sentryBaseline,
                   (unsigned)sentryLastSample,
                   (unsigned)triggerThreshold);
    } else {
        tft.print("BASE:UNSET  FIRST SAMPLE WILL SET NORMAL");
    }

    const int leftLabel = 12;
    const int leftValue = 121;
    const int rightLabel = 166;
    const int rightValue = 280;

    tft.setTextSize(1);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setCursor(leftLabel, 94);  tft.print("INDIVIDUAL");
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(leftValue, 94);  tft.print(sentryIndividualDeviceCount);

    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setCursor(rightLabel, 94); tft.print("KNOWN");
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(rightValue, 94); tft.print(sentryKnownDeviceCount);

    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setCursor(leftLabel, 113); tft.print("UNKNOWN");
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(leftValue, 113); tft.print(unknown);

    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setCursor(rightLabel, 113); tft.print("DUPLICATES");
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(rightValue, 113); tft.print(sentryDuplicateDeviceCount);

    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setCursor(leftLabel, 132); tft.print("SCANS");
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(leftValue, 132); tft.print(sentrySessionScanCount);

    if (sentryState == SENTRY_STATE_ACTIVE) {
        tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        tft.setCursor(rightLabel, 132); tft.print("WINDOWS");
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setCursor(rightValue, 132); tft.print(sentryActiveSampleCount);
    }

    const char* bleText = "BLE:SLEEP";
    uint16_t bleColor = TFT_DARKGREY;
    if (sentryState == SENTRY_STATE_SAMPLE) {
        bleText = "BLE:SAMPLE";
        bleColor = TFT_ORANGE;
    } else if (sentryState == SENTRY_STATE_ACTIVE) {
        bleText = "BLE:ACTIVE";
        bleColor = TFT_GREEN;
    }

    tft.setTextColor(bleColor, TFT_BLACK);
    tft.setCursor(12, 166);
    tft.print(bleText);

    tft.setTextColor(logging ? TFT_GREEN : TFT_RED, TFT_BLACK);
    tft.setCursor(112, 166);
    tft.print(logging ? "LOGGING:YES" : "LOGGING:NO");

    tft.setTextColor(sdStatus == SD_STATUS_DETECTED ? TFT_GREEN : TFT_RED, TFT_BLACK);
    tft.setCursor(219, 166);
    tft.print(sdStatus == SD_STATUS_DETECTED ? "SD:PRESENT" : "SD:NOT READY");

    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setCursor(68, 207);
    tft.printf("DISPLAY SLEEPS AFTER %lu SEC",
               (unsigned long)SENTRY_SCREEN_WAKE_SECONDS);

    giveUi();
    sentryDashboardDirty = false;
    lastSentryDashRefreshMs = millis();
}

void wakeSentryDashboard() {
    if (!sentryModeActive || sentryEntering) return;
    sentryDashboardAwake = true;
    sentryLegalNoticeVisible = false;
    sentryWakeStartedMs = millis();
    sentryDashboardDirty = true;
    drawSentryDashboard();
}

void handleSentryTouchWake() {
    if (!sentryModeActive || sentryEntering) return;

    if (!sentryDashboardAwake) {
        // First touch from the fully dark Sentry state shows the legal notice.
        drawSentryLegalNotice();
        return;
    }

    if (sentryLegalNoticeVisible) {
        // Second touch advances from the notice to the normal Sentry dashboard.
        wakeSentryDashboard();
        return;
    }

    // A touch while the dashboard is already visible simply refreshes its timeout.
    wakeSentryDashboard();
}

void beginSentrySleep(uint32_t now) {
    sentryState = SENTRY_STATE_SLEEP;
    sentryStateStartedMs = now;
    clearSentryActivityWindow();
    sentryDashboardDirty = true;
}

void beginSentrySample(uint32_t now) {
    sentryState = SENTRY_STATE_SAMPLE;
    sentryStateStartedMs = now;
    clearSentryActivityWindow();
    sentryDashboardDirty = true;
    Serial.printf("SENTRY: %lu-second BLE activity sample started\n",
                  (unsigned long)SENTRY_MEASUREMENT_WINDOW_SECONDS);
}

void beginSentryActive(uint32_t now) {
    sentryState = SENTRY_STATE_ACTIVE;
    sentryStateStartedMs = now;
    sentryActiveStartedMs = now;
    sentryActiveWindowStartedMs = now;
    sentryOldBaseline = sentryBaseline;
    sentryActiveSampleCount = 0;
    sentryProvisionalMedian = 0;
    memset(sentryActiveSamples, 0, sizeof(sentryActiveSamples));
    clearSentryActivityWindow();
    sentryDashboardDirty = true;

    Serial.printf("SENTRY: ACTIVE %lu-minute scan started | old baseline=%u | trigger=%u\n",
                  (unsigned long)SENTRY_ACTIVE_MINUTES,
                  (unsigned)sentryOldBaseline,
                  (unsigned)calculateSentryTriggerThreshold());
}

void finishSentryWakeSample(uint32_t now) {
    uint16_t sample = snapshotAndResetSentryActivityWindow();
    sentryLastSample = sample;

    if (!sentryBaselineValid) {
        saveSentryBaseline(sample);
        Serial.printf("SENTRY: initial baseline established at %u unique devices / %lu sec\n",
                      (unsigned)sample,
                      (unsigned long)SENTRY_MEASUREMENT_WINDOW_SECONDS);
        beginSentrySleep(now);
        if (pBLEScan != nullptr && pBLEScan->isScanning()) pBLEScan->stop();
        return;
    }

    uint16_t threshold = calculateSentryTriggerThreshold();
    Serial.printf("SENTRY: sample=%u | baseline=%u | trigger if >%u\n",
                  (unsigned)sample,
                  (unsigned)sentryBaseline,
                  (unsigned)threshold);

    // Hard rule: ordinary configured wake samples are comparison-only.
    // They never alter the baseline.
    if (sample > threshold) {
        beginSentryActive(now);
    } else {
        beginSentrySleep(now);
        if (pBLEScan != nullptr && pBLEScan->isScanning()) pBLEScan->stop();
    }
}

void finishSentryActive(uint32_t now) {
    // The active timer is fixed and is never allowed to extend. Compute the
    // learned baseline at the fixed endpoint, switch to SLEEP, then stop any
    // in-progress 2-second scan so post-stop callbacks cannot affect learning.
    if (sentryActiveSampleCount > 0) {
        uint16_t learned = sentryMedian(sentryActiveSamples, sentryActiveSampleCount);
        saveSentryBaseline(learned);
        sentryProvisionalMedian = learned;
        Serial.printf("SENTRY: %lu-minute learning complete | windows=%u | new baseline=%u\n",
                      (unsigned long)SENTRY_ACTIVE_MINUTES,
                      (unsigned)sentryActiveSampleCount,
                      (unsigned)learned);
    } else {
        Serial.printf("SENTRY: %lu-minute learning ended with no complete windows; old baseline retained\n",
                      (unsigned long)SENTRY_ACTIVE_MINUTES);
    }

    beginSentrySleep(now);
    if (pBLEScan != nullptr && pBLEScan->isScanning()) pBLEScan->stop();
}

void updateSentryEngine() {
    if (!sentryModeActive) return;

    uint32_t now = millis();

    if (sentryState == SENTRY_STATE_SLEEP) {
        if ((uint32_t)(now - sentryStateStartedMs) >= SENTRY_SLEEP_MS) {
            beginSentrySample(now);
        }
        return;
    }

    if (sentryState == SENTRY_STATE_SAMPLE) {
        if ((uint32_t)(now - sentryStateStartedMs) >= SENTRY_SAMPLE_MS) {
            // Do not force-stop the 2-second scan here. If one is crossing the
            // configured measurement boundary, the state change below makes later callbacks
            // belong to ACTIVE or be ignored in SLEEP. This avoids an async
            // stop/restart race while keeping the measurement boundary fixed.
            finishSentryWakeSample(now);
        }
        return;
    }

    // ACTIVE: capture one directly-comparable unique-device count per configured
    // measurement window while BLE scans are chained back-to-back.
    while ((uint32_t)(now - sentryActiveWindowStartedMs) >= SENTRY_ACTIVITY_WINDOW_MS &&
           sentryActiveSampleCount < SENTRY_ACTIVE_SAMPLE_CAPACITY) {
        uint16_t count = snapshotAndResetSentryActivityWindow();
        sentryActiveSamples[sentryActiveSampleCount++] = count;
        sentryProvisionalMedian = sentryMedian(sentryActiveSamples, sentryActiveSampleCount);
        sentryActiveWindowStartedMs += SENTRY_ACTIVITY_WINDOW_MS;
        sentryDashboardDirty = true;
    }

    if ((uint32_t)(now - sentryActiveStartedMs) >= SENTRY_ACTIVE_MS) {
        finishSentryActive(now);
    }
}

void enterSentryMode() {
    if (sentryModeActive || currentPage != PAGE_STATUS) return;
    if (selfTestActive || manualLogNoticeActive || warningScreenActive || micCamAlert.active || suspiciousAlert.active || possibleBanner.active) return;

    sentryModeActive = true;
    sentryEntering = true;
    sentryEnteringStartedMs = millis();
    sentryDashboardAwake = false;
    sentryLegalNoticeVisible = false;
    statusShortClickPending = false;
    statusSecondClickArmed = false;
    lastSentryTouchPressed = (digitalRead(TOUCH_IRQ_PIN) == LOW);
    sentryDashboardDirty = true;

    clearPossibleBanner();
    clearSuspiciousAlert();
    clearWarningDevices();
    warningScreenActive = false;

    if (pBLEScan != nullptr && pBLEScan->isScanning()) pBLEScan->stop();
    firstScanPending = false;
    beginSentrySleep(millis());
    rgbDimRed();
    drawSentryEntering();
}

void exitSentryMode() {
    if (!sentryModeActive) return;

    if (pBLEScan != nullptr && pBLEScan->isScanning()) pBLEScan->stop();

    sentryModeActive = false;
    sentryEntering = false;
    sentryDashboardAwake = false;
    sentryLegalNoticeVisible = false;
    statusShortClickPending = false;
    statusSecondClickArmed = false;
    clearSentryActivityWindow();

    analogWrite(TFT_BL, 255);
    screenAwake = true;
    currentPage = PAGE_STATUS;
    // Resume the normal 5-second scheduler cleanly after the stopped Sentry
    // scan has delivered its completion callback.
    firstScanPending = false;
    lastScanStartMs = millis();
    rgbGreen();
    showStatusPage();
}

void updateSentryMode() {
    if (!sentryModeActive) return;

    uint32_t now = millis();

    if (sentryEntering) {
        if ((uint32_t)(now - sentryEnteringStartedMs) >= SENTRY_ENTER_NOTICE_MS) {
            sentryEntering = false;
            lastSentryTouchPressed = (digitalRead(TOUCH_IRQ_PIN) == LOW);
            drawSentryIdle();
        }
        return;
    }

    bool touched = (digitalRead(TOUCH_IRQ_PIN) == LOW);

    if (touched && !lastSentryTouchPressed &&
        (uint32_t)(now - lastSentryTouchMs) >= SENTRY_TOUCH_DEBOUNCE_MS) {
        lastSentryTouchMs = now;
        handleSentryTouchWake();
    }
    lastSentryTouchPressed = touched;

    if (sentryDashboardAwake) {
        if ((uint32_t)(now - sentryWakeStartedMs) >= SENTRY_WAKE_MS) {
            drawSentryIdle();
            return;
        }

        if (!sentryLegalNoticeVisible &&
            sentryDashboardDirty &&
            (uint32_t)(now - lastSentryDashRefreshMs) >= SENTRY_DASH_REFRESH_MS) {
            drawSentryDashboard();
        }
    }
}

bool takeUi(uint32_t timeoutMs = 100) {
    if (uiMutex == nullptr) return true;
    return xSemaphoreTake(uiMutex, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

void giveUi() {
    if (uiMutex != nullptr) xSemaphoreGive(uiMutex);
}

int warningDeviceIndex(const String& deviceMacHash) {
    for (uint8_t i = 0; i < warningDeviceCount; i++) {
        if (warningDevices[i].deviceMacHash.equalsIgnoreCase(deviceMacHash)) return i;
    }
    return -1;
}

void clearWarningDevice(WarningDevice& d) {
    d.deviceMacHash = "";
    d.name = "";
    d.product = nullptr;
    d.cameraStatus = nullptr;
    d.rssi = -127;
    d.confidence = 0;
    d.expiresAt = 0;
}

void clearWarningDevices() {
    for (uint8_t i = 0; i < MAX_WARNING_DEVICES; i++) {
        clearWarningDevice(warningDevices[i]);
    }
    warningDeviceCount = 0;
}

const char* warningLabelText(const WarningDevice& d) {
    // Prefer the matched/confirmed product name on an alert.
    // Fall back to the raw BLE name, then the session-scoped device MAC hash.
    if (d.product && d.product[0]) return d.product;
    if (d.name.length()) return d.name.c_str();
    return d.deviceMacHash.c_str();
}

void printScrollingText(const char* source, uint8_t maxChars) {
    if (source == nullptr || maxChars < 1) return;

    size_t sourceLen = strlen(source);
    if (sourceLen <= maxChars) {
        tft.print(source);
        return;
    }

    const uint8_t gap = 4;
    const uint32_t stepMs = 250UL;
    size_t cycleLen = sourceLen + gap;
    size_t offset = (millis() / stepMs) % cycleLen;

    for (uint8_t i = 0; i < maxChars; i++) {
        size_t pos = (offset + i) % cycleLen;
        tft.write((uint8_t)(pos < sourceLen ? source[pos] : ' '));
    }
}

void warningGrid(uint8_t count, uint8_t& cols, uint8_t& rows) {
    if (count <= 1) {
        cols = 1; rows = 1;
    } else if (count == 2) {
        cols = 2; rows = 1;
    } else if (count <= 4) {
        cols = 2; rows = 2;
    } else if (count <= 9) {
        cols = 3; rows = 3;
    } else if (count <= 16) {
        cols = 4; rows = 4;
    } else {
        // 17-20 active warnings.
        cols = 5; rows = 4;
    }
}

void drawWarningTile(const WarningDevice& d,
                     uint8_t number,
                     int x, int y, int w, int h) {
    tft.fillRect(x, y, w, h, TFT_RED);
    tft.drawRect(x, y, w, h, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, TFT_RED);

    const char* label = warningLabelText(d);

    if (w >= 145 && h >= 150) {
        // Large single/two-device HIGH card.
        tft.setTextSize(2);
        tft.setCursor(x + 6, y + 7);
        tft.printf("#%u  %u%%", (unsigned)number, (unsigned)d.confidence);

        uint8_t maxChars = (uint8_t)((w - 12) / 12);
        if (maxChars < 1) maxChars = 1;
        tft.setCursor(x + 6, y + 33);
        printScrollingText(label, maxChars);

        tft.setTextSize(1);
        tft.setCursor(x + 6, y + 70);
        tft.printf("RSSI: %d dBm", d.rssi);
        tft.setCursor(x + 6, y + 84);
        tft.printf("Approx: %.1f m", approximateDistanceMetres(d.rssi));

        tft.setTextSize(2);
        tft.setCursor(x + 6, y + 104);
        tft.print("CAMERA");
        tft.setCursor(x + 6, y + 129);
        tft.print(d.cameraStatus ? d.cameraStatus : "UNKNOWN");

        tft.setTextSize(1);
        tft.setCursor(x + 6, y + h - 14);
        tft.print("10 SEC");
        return;
    }

    if (w >= 95 && h >= 75) {
        tft.setTextSize(1);
        tft.setCursor(x + 4, y + 4);
        tft.printf("#%u %u%% ", (unsigned)number, (unsigned)d.confidence);

        int prefixChars = (number >= 10) ? 9 : 8;
        int maxChars = ((w - 8) / 6) - prefixChars;
        if (maxChars < 1) maxChars = 1;
        printScrollingText(label, (uint8_t)maxChars);

        tft.setCursor(x + 4, y + 22);
        tft.printf("RSSI %d ~%.1fm", d.rssi, approximateDistanceMetres(d.rssi));

        tft.setCursor(x + 4, y + 40);
        tft.print("CAM ");
        tft.print(d.cameraStatus ? d.cameraStatus : "UNKNOWN");

        tft.setCursor(x + 4, y + h - 13);
        tft.print("10 SEC");
        return;
    }

    if (w >= 60 && h >= 42) {
        tft.setTextSize(1);
        tft.setCursor(x + 2, y + 3);
        tft.printf("#%u %u%%", (unsigned)number, (unsigned)d.confidence);

        tft.setCursor(x + 2, y + 18);
        tft.printf("%d ~%.1fm", d.rssi, approximateDistanceMetres(d.rssi));

        tft.setCursor(x + 2, y + 32);
        if (d.cameraStatus && strcmp(d.cameraStatus, "LIKELY") == 0) tft.print("LIKELY");
        else if (d.cameraStatus && strcmp(d.cameraStatus, "NOT LIKELY") == 0) tft.print("NOT");
        else tft.print("UNKNOWN");
        return;
    }

    tft.setTextSize(1);
    tft.setCursor(x + 2, y + 3);
    tft.printf("#%u", (unsigned)number);
    tft.setCursor(x + 2, y + 15);
    tft.printf("%u%%", (unsigned)d.confidence);
}

void updateWarningLabelScroll() {
    if (selfTestActive || !warningScreenActive || warningDeviceCount == 0) return;

    uint32_t now = millis();
    if ((uint32_t)(now - lastWarningLabelScrollMs) < 250UL) return;
    lastWarningLabelScrollMs = now;

    if (!takeUi(20)) return;

    const int left = 4;
    const int right = 316;
    const int top = 59;
    const int bottom = 236;
    const int gap = 3;

    uint8_t cols, rows;
    warningGrid(warningDeviceCount, cols, rows);

    int areaW = right - left;
    int areaH = bottom - top;
    int tileW = (areaW - gap * (cols - 1)) / cols;
    int tileH = (areaH - gap * (rows - 1)) / rows;

    for (uint8_t i = 0; i < warningDeviceCount; i++) {
        int col = i % cols;
        int row = i / cols;
        if (row >= rows) break;

        int x = left + col * (tileW + gap);
        int y = top + row * (tileH + gap);
        const char* label = warningLabelText(warningDevices[i]);

        tft.setTextColor(TFT_WHITE, TFT_RED);

        if (tileW >= 145 && tileH >= 150) {
            tft.fillRect(x + 5, y + 31, tileW - 10, 24, TFT_RED);
            tft.setTextSize(2);
            tft.setCursor(x + 6, y + 33);
            uint8_t maxChars = (uint8_t)((tileW - 12) / 12);
            if (maxChars < 1) maxChars = 1;
            printScrollingText(label, maxChars);
        } else if (tileW >= 95 && tileH >= 75) {
            // Keep # and confidence fixed; scroll only the remaining label area.
            tft.fillRect(x + 3, y + 3, tileW - 6, 11, TFT_RED);
            tft.setTextSize(1);
            tft.setCursor(x + 4, y + 4);
            tft.printf("#%u %u%% ",
                       (unsigned)(i + 1),
                       (unsigned)warningDevices[i].confidence);
            int prefixChars = ((i + 1) >= 10) ? 9 : 8;
            int maxChars = ((tileW - 8) / 6) - prefixChars;
            if (maxChars < 1) maxChars = 1;
            printScrollingText(label, (uint8_t)maxChars);
        }
        // Compact 4x4/5-column tiles keep their confidence line static.
    }

    giveUi();
}

void redrawWarningScreen() {
    if (!warningScreenActive || warningDeviceCount == 0) return;
    if (!takeUi()) return;

    tft.fillScreen(TFT_BLACK);
    tft.setTextWrap(false);

    // Bright-red header.
    tft.fillRect(0, 0, 320, 55, TFT_RED);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.setTextSize(2);
    tft.setCursor(7, 5);
    tft.print("HIGH GLASSES ALERT");

    tft.setTextSize(1);
    tft.setCursor(8, 27);
    tft.print("SESSION ");
    tft.print(sessionId);

    tft.setCursor(8, 39);
    tft.print("100% - 10 SECONDS");

    if (warningDeviceCount > 1) {
        tft.setCursor(205, 39);
        tft.printf("%u ACTIVE", (unsigned)warningDeviceCount);
    }

    const int left = 4;
    const int right = 316;
    const int top = 59;
    const int bottom = 236;
    const int gap = 3;

    uint8_t cols, rows;
    warningGrid(warningDeviceCount, cols, rows);

    int areaW = right - left;
    int areaH = bottom - top;
    int tileW = (areaW - gap * (cols - 1)) / cols;
    int tileH = (areaH - gap * (rows - 1)) / rows;

    for (uint8_t i = 0; i < warningDeviceCount; i++) {
        int col = i % cols;
        int row = i / cols;
        if (row >= rows) break;

        int x = left + col * (tileW + gap);
        int y = top + row * (tileH + gap);

        drawWarningTile(warningDevices[i], i + 1, x, y, tileW, tileH);
    }

    giveUi();
}

bool expireWarningDevices() {
    if (warningDeviceCount == 0) return false;

    uint32_t now = millis();
    uint8_t writeIndex = 0;
    bool changed = false;

    for (uint8_t readIndex = 0; readIndex < warningDeviceCount; readIndex++) {
        bool expired =
            ((int32_t)(now - warningDevices[readIndex].expiresAt) >= 0);

        if (expired) {
            clearWarningDevice(warningDevices[readIndex]);
            changed = true;
            continue;
        }

        if (writeIndex != readIndex) {
            warningDevices[writeIndex] = warningDevices[readIndex];
            clearWarningDevice(warningDevices[readIndex]);
            changed = true;
        }
        writeIndex++;
    }

    warningDeviceCount = writeIndex;
    return changed;
}

void drawDetectionAlert(const DetectionResult& result, BLEAdvertisedDevice& device) {
    // v6.1 hard rule: red tiled HIGH alerts require confidence >= 97.
    if (result.confidence < HIGH_ALERT_CONFIDENCE) return;

    if (selfTestActive) {
        abortSelfTestForRealWarning();
    }

    uint32_t now = millis();
    expireWarningDevices();

    String address = device.getAddress().toString().c_str();
    String deviceMacHash = deviceMacHashForAddress(address);
    int existing = warningDeviceIndex(deviceMacHash);

    if (existing >= 0) {
        WarningDevice& d = warningDevices[existing];
        d.rssi = device.getRSSI();
        d.confidence = result.confidence;
        d.expiresAt = now + WARNING_BASE_MS;
    } else if (warningDeviceCount < MAX_WARNING_DEVICES) {
        WarningDevice& d = warningDevices[warningDeviceCount++];
        d.deviceMacHash = deviceMacHash;
        d.name = device.haveName() ? String(device.getName().c_str()) : "";
        d.product = result.product;
        d.cameraStatus = cameraStatusLabel(result);
        d.rssi = device.getRSSI();
        d.confidence = result.confidence;
        d.expiresAt = now + WARNING_BASE_MS;
    }

    warningScreenActive = (warningDeviceCount > 0);
    clearMicCamAlert();
    clearSuspiciousAlert();
    clearPossibleBanner();
    screenWakeStarted = now;
    screenBacklight(true);
    redrawWarningScreen();
}

void clearPossibleBanner() {
    possibleBanner.active = false;
    possibleBanner.deviceMacHash = "";
    possibleBanner.label = "";
    possibleBanner.rssi = -127;
    possibleBanner.confidence = 0;
    possibleBanner.expiresAt = 0;
}

void clearSuspiciousAlert() {
    suspiciousAlert.active = false;
    suspiciousAlert.deviceMacHash = "";
    suspiciousAlert.deviceLabel = nullptr;
    suspiciousAlert.category = nullptr;
    suspiciousAlert.confidence = 0;
    suspiciousAlert.expiresAt = 0;
}

void clearMicCamAlert() {
    micCamAlert.active = false;
    micCamAlert.deviceMacHash = "";
    micCamAlert.deviceLabel = nullptr;
    micCamAlert.category = nullptr;
    micCamAlert.confidence = 0;
    micCamAlert.expiresAt = 0;
}

void printTftTruncated(const char* text, size_t maxChars) {
    if (text == nullptr) return;
    for (size_t i = 0; text[i] != '\0' && i < maxChars; ++i) {
        tft.write((uint8_t)text[i]);
    }
}

void drawMicCamAlertScreen(const char* deviceLabel, const char* category) {
    if (!takeUi(50)) return;

    tft.fillScreen(TFT_ORANGE);
    tft.setTextWrap(false);
    tft.setTextColor(TFT_BLACK, TFT_ORANGE);

    tft.setTextSize(2);
    tft.setCursor(6, 5);
    tft.print(MICCAM_TITLE_LINE_1);

    tft.setTextSize(1);
    tft.setCursor(7, 34);
    tft.print(MICCAM_TITLE_LINE_2);

    tft.setCursor(7, 58);
    tft.print("DEVICE: ");
    const char* shown = (deviceLabel && deviceLabel[0])
        ? deviceLabel : "CAMERA / AUDIO DEVICE";
    printTftTruncated(shown, 42);

    if (category && category[0]) {
        tft.setCursor(7, 72);
        tft.print("TYPE: ");
        tft.print(category);
    }

    tft.setTextSize(2);
    tft.setCursor(7, 92);
    tft.print("CAUTION");

    tft.setTextSize(1);
    tft.setCursor(7, 120);
    tft.print(MICCAM_CAUTION_LINE_1);
    tft.setCursor(7, 134);
    tft.print(MICCAM_CAUTION_LINE_2);
    tft.setCursor(7, 158);
    tft.print(MICCAM_CAUTION_LINE_3);
    tft.setCursor(7, 172);
    tft.print(MICCAM_CAUTION_LINE_4);

    giveUi();
}

void showMicCamAlert(const MicCamMatchResult& result,
                     BLEAdvertisedDevice& device) {
    if (!result.matched || result.confidence < MICCAM_ALERT_MIN_CONFIDENCE) return;
    if (sentryModeActive) return;
    if (selfTestActive) abortSelfTestForRealWarning();
    if (warningScreenActive) return;

    manualLogNoticeActive = false;

    String address = device.getAddress().toString().c_str();
    String deviceMacHash = deviceMacHashForAddress(address);
    uint32_t now = millis();

    if (deviceMacHash.equalsIgnoreCase(lastMicCamAlertDeviceMacHash) &&
        (uint32_t)(now - lastMicCamAlertMs) < MICCAM_DEVICE_COOLDOWN_MS) {
        return;
    }

    lastMicCamAlertDeviceMacHash = deviceMacHash;
    lastMicCamAlertMs = now;
    totalMicCamReviews++;

    // CAM AND AUDIO is a more specific review than the generic suspicious layer.
    clearSuspiciousAlert();
    clearPossibleBanner();

    micCamAlert.active = true;
    micCamAlert.deviceMacHash = deviceMacHash;
    micCamAlert.deviceLabel = result.deviceLabel
        ? result.deviceLabel : "CAMERA / AUDIO DEVICE";
    micCamAlert.category = micCamCategoryLabel(result.category);
    micCamAlert.confidence = result.confidence;
    micCamAlert.expiresAt = now + MICCAM_ALERT_MS;

    screenBacklight(true);
    drawMicCamAlertScreen(micCamAlert.deviceLabel, micCamAlert.category);

    Serial.printf(
        "CAM AND AUDIO REVIEW | device_mac_hash=%s | name=%s | label=%s | category=%s | confidence=%u | reason=%s\n",
        deviceMacHash.c_str(),
        device.haveName() ? device.getName().c_str() : "",
        micCamAlert.deviceLabel ? micCamAlert.deviceLabel : "",
        micCamAlert.category ? micCamAlert.category : "",
        (unsigned)result.confidence,
        result.reason ? result.reason : "CAM AND AUDIO rule match"
    );
}

void updateMicCamAlert() {
    if (!micCamAlert.active) return;
    if ((int32_t)(millis() - micCamAlert.expiresAt) < 0) return;

    clearMicCamAlert();
    if (!warningScreenActive && !micCamAlert.active && !suspiciousAlert.active &&
        !manualLogNoticeActive && !selfTestActive) {
        showSelectedPage();
    }
}

void drawSuspiciousAlertScreen(const char* deviceLabel, const char* category) {
    if (!takeUi(50)) return;

    tft.fillScreen(TFT_ORANGE);
    tft.setTextWrap(false);
    tft.setTextColor(TFT_BLACK, TFT_ORANGE);

    tft.setTextSize(2);
    tft.setCursor(6, 5);
    tft.print(SUSPICIOUS_TITLE_LINE_1);
    tft.setCursor(6, 28);
    tft.print(SUSPICIOUS_TITLE_LINE_2);

    tft.setTextSize(1);
    tft.setCursor(7, 58);
    tft.print("DEVICE: ");
    const char* shown = (deviceLabel && deviceLabel[0])
        ? deviceLabel : "UNUSUAL BLE DEVICE";
    printTftTruncated(shown, 42);

    if (category && category[0]) {
        tft.setCursor(7, 72);
        tft.print("TYPE: ");
        tft.print(category);
    }

    tft.setTextSize(2);
    tft.setCursor(7, 92);
    tft.print("CAUTION");

    tft.setTextSize(1);
    tft.setCursor(7, 120);
    tft.print(SUSPICIOUS_CAUTION_LINE_1);
    tft.setCursor(7, 134);
    tft.print(SUSPICIOUS_CAUTION_LINE_2);
    tft.setCursor(7, 158);
    tft.print(SUSPICIOUS_CAUTION_LINE_3);
    tft.setCursor(7, 184);
    tft.print(SUSPICIOUS_CAUTION_LINE_4);
    tft.setCursor(7, 198);
    tft.print(SUSPICIOUS_CAUTION_LINE_5);

    giveUi();
}

void showSuspiciousAlert(const SuspiciousMatchResult& result,
                         BLEAdvertisedDevice& device) {
    if (!result.matched || result.confidence < SUSPICIOUS_ALERT_MIN_CONFIDENCE) return;
    if (sentryModeActive) return;
    if (selfTestActive) abortSelfTestForRealWarning();
    if (warningScreenActive || micCamAlert.active) return;

    // A real review alert owns the screen over transient manual notices.
    manualLogNoticeActive = false;

    String address = device.getAddress().toString().c_str();
    String deviceMacHash = deviceMacHashForAddress(address);
    uint32_t now = millis();

    // Do not let one chatty advertiser permanently pin the review screen.
    if (deviceMacHash.equalsIgnoreCase(lastSuspiciousAlertDeviceMacHash) &&
        (uint32_t)(now - lastSuspiciousAlertMs) < SUSPICIOUS_DEVICE_COOLDOWN_MS) {
        return;
    }

    lastSuspiciousAlertDeviceMacHash = deviceMacHash;
    lastSuspiciousAlertMs = now;
    totalSuspiciousDeviceReviews++;

    suspiciousAlert.active = true;
    suspiciousAlert.deviceMacHash = deviceMacHash;
    suspiciousAlert.deviceLabel = result.deviceLabel
        ? result.deviceLabel : "UNUSUAL BLE DEVICE";
    suspiciousAlert.category = result.category
        ? result.category : "REVIEW";
    suspiciousAlert.confidence = result.confidence;
    suspiciousAlert.expiresAt = now + SUSPICIOUS_ALERT_MS;

    // Review warnings outrank POSSIBLE-glasses banners, but a 97%+ HIGH
    // glasses alert can still replace this screen.
    clearPossibleBanner();
    screenBacklight(true);
    drawSuspiciousAlertScreen(suspiciousAlert.deviceLabel, suspiciousAlert.category);

    Serial.printf(
        "SUSPICIOUS DEVICE REVIEW | device_mac_hash=%s | name=%s | label=%s | category=%s | confidence=%u | reason=%s\n",
        deviceMacHash.c_str(),
        device.haveName() ? device.getName().c_str() : "",
        suspiciousAlert.deviceLabel ? suspiciousAlert.deviceLabel : "",
        suspiciousAlert.category ? suspiciousAlert.category : "",
        (unsigned)result.confidence,
        result.reason ? result.reason : "Review rule match"
    );
}

void updateSuspiciousAlert() {
    if (!suspiciousAlert.active) return;
    if ((int32_t)(millis() - suspiciousAlert.expiresAt) < 0) return;

    clearSuspiciousAlert();
    if (!warningScreenActive && !micCamAlert.active && !manualLogNoticeActive && !selfTestActive) {
        showSelectedPage();
    }
}

void drawPossibleBanner() {
    if (!possibleBanner.active || warningScreenActive || micCamAlert.active || suspiciousAlert.active || selfTestActive || manualLogNoticeActive) return;
    if (!takeUi(40)) return;

    // Orange banner only; it does not replace the underlying scanner/status page.
    tft.fillRect(0, 0, 320, 54, TFT_ORANGE);
    tft.setTextWrap(false);
    tft.setTextColor(TFT_BLACK, TFT_ORANGE);

    tft.setTextSize(2);
    tft.setCursor(6, 4);
    tft.printf("POSSIBLE GLASSES %u%%", (unsigned)possibleBanner.confidence);

    tft.setTextSize(1);
    String label = possibleBanner.label.length() ? possibleBanner.label : possibleBanner.deviceMacHash;
    if (label.length() > 31) label = label.substring(0, 31);
    tft.setCursor(6, 27);
    tft.print(label);

    tft.setCursor(6, 41);
    tft.printf("RSSI %d dBm   ~%.1f m",
               possibleBanner.rssi,
               approximateDistanceMetres(possibleBanner.rssi));

    giveUi();
}

void showPossibleAlert(const DetectionResult& result, BLEAdvertisedDevice& device) {
    if (result.confidence < POSSIBLE_CONFIDENCE_MIN ||
        result.confidence >= HIGH_ALERT_CONFIDENCE) {
        return;
    }

    if (selfTestActive) {
        abortSelfTestForRealWarning();
    }

    uint32_t now = millis();
    String address = device.getAddress().toString().c_str();
    String deviceMacHash = deviceMacHashForAddress(address);

    bool expired = !possibleBanner.active ||
                   ((int32_t)(now - possibleBanner.expiresAt) >= 0);
    bool sameDevice = possibleBanner.active &&
                      possibleBanner.deviceMacHash.equalsIgnoreCase(deviceMacHash);

    // Keep the strongest active possible match. The same device may refresh
    // its RSSI/range and 10-second lifetime.
    if (expired || sameDevice || result.confidence >= possibleBanner.confidence) {
        possibleBanner.active = true;
        possibleBanner.deviceMacHash = deviceMacHash;
        possibleBanner.label = result.product ? String(result.product) :
                              (device.haveName() ? String(device.getName().c_str()) : deviceMacHash);
        possibleBanner.rssi = device.getRSSI();
        possibleBanner.confidence = result.confidence;
        possibleBanner.expiresAt = now + WARNING_BASE_MS;
    }

    screenBacklight(true);
    drawPossibleBanner();
}

void updatePossibleBanner() {
    if (!possibleBanner.active) return;
    uint32_t now = millis();
    if ((int32_t)(now - possibleBanner.expiresAt) < 0) return;

    clearPossibleBanner();
    if (!warningScreenActive && !micCamAlert.active && !suspiciousAlert.active && !manualLogNoticeActive && !selfTestActive) {
        showSelectedPage();
    }
}

void drawSpinnerFrame(bool active) {
    if (warningScreenActive || micCamAlert.active || suspiciousAlert.active || possibleBanner.active) return;
    if (!takeUi(20)) return;
    if (warningScreenActive || micCamAlert.active || suspiciousAlert.active) {
        giveUi();
        return;
    }

    // 8-dot loading circle on the right side of the landscape screen.
    const int cx = 267;
    const int cy = 72;
    const int radius = 18;

    tft.fillRect(cx - 27, cy - 27, 54, 54, TFT_BLACK);

    for (uint8_t i = 0; i < 8; i++) {
        float angle = (PI / 4.0f) * i - (PI / 2.0f);
        int x = cx + (int)(cosf(angle) * radius);
        int y = cy + (int)(sinf(angle) * radius);

        if (active && i == spinnerFrame) {
            tft.fillCircle(x, y, 5, TFT_WHITE);
        } else {
            tft.fillCircle(x, y, 2, TFT_DARKGREY);
        }
    }

    giveUi();
}

void showScanningNow() {
    if (warningScreenActive || micCamAlert.active || suspiciousAlert.active || manualLogNoticeActive) return;

    screenBacklight(true);

    if (!takeUi()) return;
    if (warningScreenActive || micCamAlert.active || suspiciousAlert.active) {
        giveUi();
        return;
    }

    tft.fillScreen(TFT_BLACK);
    tft.setTextWrap(false);

    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(8, 20);
    tft.print("CYD DEV LITE");

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(8, 60);
    tft.print("SCANNING NOW");

    // Explicit SD state: both colour and text.
    uint16_t sdColor = sdStatusColor();
    tft.fillCircle(13, 109, 5, sdColor);
    tft.setTextColor(sdColor, TFT_BLACK);
    if (sdStatus == SD_STATUS_DETECTED) {
        // PRESENT and NOT PRESENT deliberately use identical size/position.
        tft.setTextSize(2);
        tft.setCursor(25, 100);
        tft.print("SD PRESENT");
    } else if (sdStatus == SD_STATUS_NOT_DETECTED) {
        tft.setTextSize(2);
        tft.setCursor(25, 100);
        tft.print("SD NOT PRESENT");
    } else {
        // Retain the detailed wording for FAIL / SAFE EJECT states.
        tft.setTextSize(1);
        tft.setCursor(25, 107);
        tft.print(sdStatusText());
    }

    // v6.1 home status layout.
    // Keep each dynamic line on its own row. The previous v6.1 layout placed
    // SESSION at Y=134 and RSSI at Y=130, which caused visible text overlap.
    tft.fillRect(0, 118, 320, 48, TFT_BLACK);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);

    tft.setCursor(6, 120);
    tft.print("SESSION: ");
    tft.print(sessionId);

    tft.setCursor(6, 136);
    tft.print("Last BLE advertisement received:");

    tft.setCursor(6, 150);
    tft.print("RSSI: ");
    if (lastRSSI > -127) {
        tft.printf("%d dBm  ", lastRSSI);

        float approxM = approximateDistanceMetres(lastRSSI);
        if (approxM > 30.0f) {
            tft.setTextColor(TFT_GREEN, TFT_BLACK);
        } else {
            tft.setTextColor(TFT_RED, TFT_BLACK);
        }
        tft.printf("approx %.1f m", approxM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
    } else {
        tft.print("NO BLE YET");
    }

    // Compact BOOT-button controls. GitHub inspiration credits now live on
    // the boot legal page so the normal scanner page stays operationally clean.
    tft.setTextColor(TFT_BLUE, TFT_BLACK);
    tft.setCursor(6, 210);
    tft.print("BOOT: SYS STAT");
    tft.setCursor(6, 224);
    tft.print("HOLD: MANUAL LOG");

    giveUi();

    // White loading wheel while actual BLE scan is active.
    drawSpinnerFrame(pBLEScan != nullptr && pBLEScan->isScanning());
    if (possibleBanner.active) drawPossibleBanner();
}

uint32_t choosePrivateAddressRotationIntervalMs() {
    const uint32_t span = BLE_PRIVATE_ADDR_ROTATE_MAX_MS - BLE_PRIVATE_ADDR_ROTATE_MIN_MS;
    return BLE_PRIVATE_ADDR_ROTATE_MIN_MS + (esp_random() % (span + 1UL));
}

void formatScannerPrivateAddress(const esp_bd_addr_t address) {
    snprintf(scannerPrivateAddressText, sizeof(scannerPrivateAddressText),
             "%02X:%02X:%02X:%02X:%02X:%02X",
             address[0], address[1], address[2],
             address[3], address[4], address[5]);
}

bool scannerPrivateAddressConfirmed() {
    if (!scannerPrivateAddressReady) return false;

    esp_bd_addr_t usedAddress = {0};
    uint8_t usedType = 0xFF;
    if (esp_ble_gap_get_local_used_addr(usedAddress, &usedType) != ESP_OK) return false;
    if (usedType != BLE_ADDR_TYPE_RANDOM) return false;
    return memcmp(usedAddress, scannerPrivateAddress, sizeof(scannerPrivateAddress)) == 0;
}

bool applyFreshScannerPrivateNrpa(bool initialAddress) {
    if (pBLEScan == nullptr || pBLEScan->isScanning()) return false;

    esp_bd_addr_t candidate = {0};
    esp_err_t rc = esp_ble_gap_addr_create_nrpa(candidate);
    if (rc != ESP_OK) {
        Serial.printf("{\"type\":\"ble_privacy_error\",\"step\":\"create_nrpa\",\"code\":%d}\n", (int)rc);
        return false;
    }

    if ((candidate[0] & 0xC0U) != 0x00U) {
        Serial.println("{\"type\":\"ble_privacy_error\",\"step\":\"nrpa_type_check\"}");
        return false;
    }

    rc = esp_ble_gap_set_rand_addr(candidate);
    if (rc != ESP_OK) {
        Serial.printf("{\"type\":\"ble_privacy_error\",\"step\":\"set_rand_addr\",\"code\":%d}\n", (int)rc);
        return false;
    }

    // Bluedroid does not switch its current own-address type to RANDOM merely
    // because esp_ble_gap_set_rand_addr() succeeded. Apply the BLEScan params
    // with own_addr_type=RANDOM now, then verify the stack state before allowing
    // any scan. BLEScan::start() will re-apply this same persistent structure.
    rc = applyBleScannerOwnAddressRandom(pBLEScan);
    if (rc != ESP_OK) {
        scannerPrivateAddressReady = false;
        bleScanningAtBoot = false;
        scannerPrivateAddressNextRotateMs = millis() + BLE_PRIVATE_ADDR_RETRY_MS;
        Serial.printf("{\"type\":\"ble_privacy_error\",\"step\":\"set_scan_params_random\",\"code\":%d}\n", (int)rc);
        return false;
    }

    esp_bd_addr_t confirmedAddress = {0};
    uint8_t confirmedType = 0xFF;
    bool confirmed = false;
    const uint32_t confirmStartedMs = millis();

    while ((uint32_t)(millis() - confirmStartedMs) < BLE_PRIVATE_ADDR_CONFIRM_MS) {
        memset(confirmedAddress, 0, sizeof(confirmedAddress));
        confirmedType = 0xFF;
        if (esp_ble_gap_get_local_used_addr(confirmedAddress, &confirmedType) == ESP_OK &&
            confirmedType == BLE_ADDR_TYPE_RANDOM &&
            memcmp(confirmedAddress, candidate, sizeof(candidate)) == 0) {
            confirmed = true;
            break;
        }
        delay(2);
    }

    if (!confirmed) {
        scannerPrivateAddressReady = false;
        bleScanningAtBoot = false;
        scannerPrivateAddressNextRotateMs = millis() + BLE_PRIVATE_ADDR_RETRY_MS;
        Serial.println("{\"type\":\"ble_privacy_error\",\"step\":\"confirm_nrpa\",\"message\":\"Controller did not confirm PRIVATE NRPA; scanning disabled\"}");
        return false;
    }

    memcpy(scannerPrivateAddress, confirmedAddress, sizeof(scannerPrivateAddress));
    formatScannerPrivateAddress(scannerPrivateAddress);
    scannerPrivateAddressReady = true;
    bleScanningAtBoot = true;
    scannerPrivateAddressRotationCount++;
    scannerPrivateAddressIntervalMs = choosePrivateAddressRotationIntervalMs();
    scannerPrivateAddressNextRotateMs = millis() + scannerPrivateAddressIntervalMs;

    Serial.printf("{\"type\":\"ble_private_address\",\"event\":\"%s\",\"address\":\"%s\",\"address_type\":\"PRIVATE NRPA\",\"rotate_in_ms\":%lu}\n",
                  initialAddress ? "initial" : "rotated",
                  scannerPrivateAddressText,
                  (unsigned long)scannerPrivateAddressIntervalMs);
    return true;
}

void updateScannerPrivateAddressRotation(uint32_t nowMs) {
    if (pBLEScan == nullptr || pBLEScan->isScanning()) return;
    if (scannerPrivateAddressNextRotateMs == 0) return;
    if ((int32_t)(nowMs - scannerPrivateAddressNextRotateMs) < 0) return;

    if (!applyFreshScannerPrivateNrpa(false)) {
        scannerPrivateAddressNextRotateMs = nowMs + BLE_PRIVATE_ADDR_RETRY_MS;
    }
}

String uptimeText() {
    uint32_t totalSeconds = millis() / 1000UL;
    uint32_t days = totalSeconds / 86400UL;
    uint8_t hours = (totalSeconds % 86400UL) / 3600UL;
    uint8_t minutes = (totalSeconds % 3600UL) / 60UL;
    uint8_t seconds = totalSeconds % 60UL;

    char buf[24];
    if (days > 0) {
        snprintf(buf, sizeof(buf), "%lud %02u:%02u:%02u",
                 (unsigned long)days, hours, minutes, seconds);
    } else {
        snprintf(buf, sizeof(buf), "%02u:%02u:%02u",
                 hours, minutes, seconds);
    }
    return String(buf);
}

void showStatusPage() {
    if (warningScreenActive || micCamAlert.active || suspiciousAlert.active) return;

    screenBacklight(true);

    if (!takeUi()) return;
    if (warningScreenActive || micCamAlert.active || suspiciousAlert.active) {
        giveUi();
        return;
    }

    tft.fillScreen(TFT_BLACK);
    tft.setTextWrap(false);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(3);
    tft.setCursor(8, 5);
    tft.print("SYS STAT");

    // SD and BLE status stay grouped below the heading on the right.
    const char* sdShort = "MISSING";
    uint16_t sdColor = TFT_RED;
    if (sdStatus == SD_STATUS_DETECTED) {
        sdShort = "READY";
        sdColor = TFT_GREEN;
    } else if (sdStatus == SD_STATUS_FAIL) {
        sdShort = "FAIL";
    } else if (sdStatus == SD_STATUS_EJECTED) {
        sdShort = "EJECTED";
    }

    tft.setTextSize(1);
    tft.setTextColor(sdColor, TFT_BLACK);
    tft.setCursor(242, 35);
    tft.print("SD: ");
    tft.print(sdShort);

    tft.setTextColor(scannerPrivateAddressReady ? TFT_GREEN : TFT_RED, TFT_BLACK);
    tft.setCursor(242, 47);
    tft.print("BLE: ");
    tft.print(scannerPrivateAddressReady ? "READY" : "OFF");

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);

    const int x = 8;
    tft.setCursor(x, 39);  tft.print("SESSION: "); tft.print(sessionId);
    tft.setCursor(x, 51);  tft.print("UPTIME:  "); tft.print(uptimeText());

    const char* localAddress = scannerPrivateAddressReady
                               ? scannerPrivateAddressText
                               : "--:--:--:--:--:--";
    const char* localAddressType = scannerPrivateAddressReady
                                   ? "PRIVATE NRPA"
                                   : "PRIVACY ERROR";

    tft.setCursor(x, 65);  tft.print("BLE ADDR: "); tft.print(localAddress);
    tft.setCursor(x, 77);  tft.print("ADDR TYPE: "); tft.print(localAddressType);
    tft.setCursor(x, 89);  tft.print("ROTATE: RANDOM 12-18 MIN");

    tft.setCursor(x, 105); tft.print("SCANS:        "); tft.print(totalScans);
    tft.setCursor(x, 117); tft.print("KNOWN DEV:    "); tft.print(knownNonGlassesDeviceCount);
    tft.setCursor(x, 129); tft.print("DETECTIONS:   "); tft.print(totalDetections);
    tft.setCursor(x, 141); tft.print("IDS CAPTURED: "); tft.print(totalAdvertisements);
    tft.setCursor(x, 153); tft.print("IDS WRITTEN:  "); tft.print(totalUniqueAdvertisements);
    tft.setCursor(x, 165); tft.print("WATCHED DEV:  "); tft.print(trackedDeviceCount);

    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t heapSize = ESP.getHeapSize();
    uint32_t usedHeap = heapSize >= freeHeap ? heapSize - freeHeap : 0;
    tft.setCursor(x, 181); tft.printf("FREE HEAP (RAM): %.1f KB", freeHeap / 1024.0f);
    tft.setCursor(x, 193); tft.printf("USED HEAP (RAM): %.1f KB", usedHeap / 1024.0f);

    tft.setTextColor(TFT_BLUE, TFT_BLACK);
    tft.setCursor(8, 204); tft.print("BOOT: BACK   2X BOOT: SENTRY");
    tft.setCursor(8, 216); tft.print("HOLD 4 SEC: SELF TEST");
    tft.setCursor(8, 228); tft.print("HOLD 6 SEC: SD EJECT");

    lastStatusPageRefreshMs = millis();
    giveUi();
    if (possibleBanner.active) drawPossibleBanner();
}

void showSelectedPage() {
    if (sentryModeActive) {
        if (sentryEntering) drawSentryEntering();
        else if (sentryLegalNoticeVisible) drawSentryLegalNotice();
        else if (sentryDashboardAwake) drawSentryDashboard();
        else drawSentryIdle();
        return;
    }

    if (warningScreenActive) {
        redrawWarningScreen();
        return;
    }

    if (micCamAlert.active) {
        drawMicCamAlertScreen(micCamAlert.deviceLabel, micCamAlert.category);
        return;
    }

    if (suspiciousAlert.active) {
        drawSuspiciousAlertScreen(suspiciousAlert.deviceLabel, suspiciousAlert.category);
        return;
    }

    if (currentPage == PAGE_STATUS) showStatusPage();
    else showScanningNow();
}

void showManualLogNotice(const String& title,
                         const String& line1,
                         const String& line2) {
    manualLogNoticeActive = true;
    manualLogNoticeExpiresMs = millis() + MANUAL_LOG_NOTICE_MS;
    manualLogNoticeTitle = title;
    manualLogNoticeLine1 = line1;
    manualLogNoticeLine2 = line2;

    if (!takeUi()) return;

    tft.fillScreen(TFT_BLACK);
    tft.setTextWrap(false);

    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(8, 48);
    tft.print(title);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(8, 95);
    tft.print(line1);
    tft.setCursor(8, 118);
    tft.print(line2);

    giveUi();
}

void performManualSmartGlassesLog() {
    if (currentPage != PAGE_SCANNER ||
        selfTestActive ||
        warningScreenActive ||
        micCamAlert.active ||
        suspiciousAlert.active ||
        manualLogNoticeActive) {
        return;
    }

    if (!sdOK) {
        showManualLogNotice(
            "MANUAL LOG FAILED",
            "SD NOT DETECTED",
            "Insert SD card and try again"
        );
        return;
    }

    if (manualContextCaptureActive ||
        manualContextReadyToWrite ||
        manualContextWriteInProgress) {
        showManualLogNotice(
            "MANUAL CAPTURE BUSY",
            manualContextEventId[0] ? String(manualContextEventId) : String("Event active"),

            "Waiting for current 5 AFTER"
        );
        return;
    }

    if (manualBeforeRingCount < MANUAL_CONTEXT_BEFORE_COUNT) {
        showManualLogNotice(
            "MANUAL LOG FAILED",

            "NEED 5 PRIOR BLE OBS",
            "Chronological capture only"
        );
        return;
    }

    String eventId;
    if (!armManualContextCapture(eventId)) {
        showManualLogNotice(
            "MANUAL LOG FAILED",
            "CONTEXT ARM FAILED",
            "Try again"
        );
        return;
    }

    showManualLogNotice(
        "MANUAL CONTEXT ARMED",
        eventId,

        "5 BEFORE + NEXT 5 AFTER"
    );
}

void processManualContextWrite() {
    if (!manualContextReadyToWrite || manualContextWriteInProgress) return;

    // Keep the completed 10-observation event in RAM until the SD is writable.
    if (!sdOK || sdSafelyEjected) return;

    manualContextWriteInProgress = true;

    bool ok = writeManualContextEvent();

    if (ok) {
        manualContextReadyToWrite = false;

        // Release the frozen BEFORE ring only after persistence succeeds. Requiring
        // five fresh observations prevents a later event from reusing stale context.
        if (manualContextMutex &&
            xSemaphoreTake(manualContextMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            memset(manualBeforeRing, 0, sizeof(manualBeforeRing));
            memset(manualContextAfter, 0, sizeof(manualContextAfter));
            manualBeforeRingCount = 0;
            manualBeforeRingNext = 0;
            manualBeforeFrozenStart = 0;
            manualContextAfterCount = 0;
            xSemaphoreGive(manualContextMutex);
        }
        if (!warningScreenActive && !micCamAlert.active && !suspiciousAlert.active && !selfTestActive && !sentryModeActive) {
            showManualLogNotice(
                "MANUAL CONTEXT SAVED",
                String(manualContextEventId),

                "5 BEFORE + 5 AFTER = 10"
            );
        }
    } else {
        if (!warningScreenActive && !micCamAlert.active && !suspiciousAlert.active && !selfTestActive && !sentryModeActive) {
            showManualLogNotice(
                "MANUAL LOG FAILED",
                "SD WRITE FAILED",
                sdStatusText()
            );
        }
    }

    manualContextWriteInProgress = false;
}

void updateManualLogNotice() {
    if (!manualLogNoticeActive) return;

    if (warningScreenActive || micCamAlert.active || suspiciousAlert.active) {
        // A real warning owns the screen. Do not redraw the manual notice.
        manualLogNoticeActive = false;
        return;
    }

    if ((int32_t)(millis() - manualLogNoticeExpiresMs) >= 0) {
        manualLogNoticeActive = false;
        manualLogNoticeTitle = "";
        manualLogNoticeLine1 = "";
        manualLogNoticeLine2 = "";
        showSelectedPage();
    }
}

void updateStatusPage() {
    if (sentryModeActive || selfTestActive || manualLogNoticeActive || warningScreenActive || micCamAlert.active || suspiciousAlert.active || possibleBanner.active || currentPage != PAGE_STATUS) return;
    if ((uint32_t)(millis() - lastStatusPageRefreshMs) >= 1000UL) {
        showStatusPage();
    }
}

void updatePageButton() {
    bool pressed = (digitalRead(PAGE_BUTTON_PIN) == LOW);
    uint32_t now = millis();

    // While self-test is active, any new BOOT press exits immediately.
    if (selfTestActive && pressed && !lastPageButtonPressed) {
        lastPageButtonPressed = true;
        lastPageButtonChangeMs = now;
        finishSelfTest();
        return;
    }

    if (pressed != lastPageButtonPressed &&
        (uint32_t)(now - lastPageButtonChangeMs) >= PAGE_BUTTON_DEBOUNCE_MS) {

        lastPageButtonChangeMs = now;
        lastPageButtonPressed = pressed;

        if (pressed) {
            pageButtonPressedAtMs = now;
            pageButtonLongHandled = false;
            pageButtonSafeEjectHandled = false;

            // Arm the double press on the SECOND press-down edge. This makes
            // the 500 ms window independent of how long the second tap is held.
            if (currentPage == PAGE_STATUS && statusShortClickPending &&
                (uint32_t)(now - statusFirstClickReleasedMs) <= PAGE_BUTTON_DOUBLE_PRESS_MS) {
                statusSecondClickArmed = true;
                statusShortClickPending = false;
            }
        } else {
            uint32_t heldMs = now - pageButtonPressedAtMs;

            if (currentPage == PAGE_STATUS) {
                if (statusSecondClickArmed && heldMs < PAGE_BUTTON_SHORT_MAX_MS) {
                    statusSecondClickArmed = false;
                    if (sentryModeActive) exitSentryMode();
                    else enterSentryMode();
                    return;
                }

                // Sentry Mode uses BOOT only as a double-press exit control.
                // A single short or long press intentionally does nothing.
                if (sentryModeActive) {
                    if (heldMs < PAGE_BUTTON_SHORT_MAX_MS) {
                        statusShortClickPending = true;
                        statusFirstClickReleasedMs = now;
                    } else {
                        // A long second press is not a valid double-click.
                        // Clear the armed state so a later press cannot be
                        // misinterpreted as the completion of this gesture.
                        statusShortClickPending = false;
                        statusSecondClickArmed = false;
                    }
                    return;
                }

                // Hold 4 to under 6 sec, then RELEASE to start the self test.
                if (!pageButtonSafeEjectHandled &&
                    heldMs >= SYS_STAT_SELF_TEST_HOLD_MS &&
                    heldMs < SYS_STAT_SD_EJECT_HOLD_MS &&
                    !warningScreenActive &&
                    !micCamAlert.active &&
                    !suspiciousAlert.active) {
                    statusShortClickPending = false;
                    statusSecondClickArmed = false;
                    pageButtonLongHandled = true;
                    startSelfTest();
                    return;
                }

                // First short SYS STAT press: wait briefly for a second press.
                if (!pageButtonLongHandled &&
                    !pageButtonSafeEjectHandled &&
                    heldMs < PAGE_BUTTON_SHORT_MAX_MS &&
                    !warningScreenActive &&
                    !micCamAlert.active &&
                    !suspiciousAlert.active) {
                    statusShortClickPending = true;
                    statusFirstClickReleasedMs = now;
                    return;
                }

                // A 2-to-<4 second SYS STAT hold is intentionally a no-op.
                if (heldMs >= PAGE_BUTTON_SHORT_MAX_MS &&
                    heldMs < SYS_STAT_SELF_TEST_HOLD_MS) {
                    statusShortClickPending = false;
                    statusSecondClickArmed = false;
                    return;
                }
            } else {
                // Scanner short press -> System Status.
                if (!pageButtonLongHandled &&
                    heldMs < PAGE_BUTTON_SHORT_MAX_MS &&
                    !manualLogNoticeActive &&
                    !warningScreenActive &&
                    !micCamAlert.active &&
                    !suspiciousAlert.active) {
                    statusShortClickPending = false;
                    statusSecondClickArmed = false;
                    currentPage = PAGE_STATUS;
                    showSelectedPage();
                    return;
                }
            }
        }
    }

    // If SYS STAT received only one short press, honour the original action
    // after the double-press window expires.
    if (statusShortClickPending && !pressed &&
        (uint32_t)(now - statusFirstClickReleasedMs) > PAGE_BUTTON_DOUBLE_PRESS_MS) {
        statusShortClickPending = false;
        if (!sentryModeActive && currentPage == PAGE_STATUS) {
            currentPage = PAGE_SCANNER;
            showSelectedPage();
            return;
        }
        // In Sentry Mode a single BOOT press intentionally has no action.
    }

    // Scanner/home long press still manually logs glasses at ~2 sec.
    if (pressed &&
        !sentryModeActive &&
        currentPage == PAGE_SCANNER &&
        !pageButtonLongHandled &&
        !manualLogNoticeActive &&
        !warningScreenActive &&
        !micCamAlert.active &&
        !suspiciousAlert.active &&
        (uint32_t)(now - pageButtonPressedAtMs) >= SCANNER_MANUAL_LOG_HOLD_MS) {

        pageButtonLongHandled = true;
        performManualSmartGlassesLog();
        return;
    }

    // System Status: 6-second hold safely unmounts/remounts the SD.
    // Sentry Mode must be exited first so an accidental concealed hold cannot
    // eject the logger card.
    if (pressed &&
        !sentryModeActive &&
        currentPage == PAGE_STATUS &&
        !pageButtonSafeEjectHandled &&
        !warningScreenActive &&
        !micCamAlert.active &&
        !suspiciousAlert.active &&
        (uint32_t)(now - pageButtonPressedAtMs) >= SYS_STAT_SD_EJECT_HOLD_MS) {

        statusShortClickPending = false;
        statusSecondClickArmed = false;
        pageButtonSafeEjectHandled = true;
        pageButtonLongHandled = true;

        if (sdSafelyEjected) {
            if (remountEjectedSD()) {
                showManualLogNotice("SD REMOUNTED", sessionId, "Logging enabled");
            } else {
                showManualLogNotice("SD REMOUNT FAILED", "Check card", "Hold 6 sec to retry");
            }
        } else {
            if (safeEjectSD()) {
                showManualLogNotice("SD SAFELY EJECTED", "SAFE TO REMOVE CARD", "Reinsert: auto check 10 sec");
            } else {
                showManualLogNotice("SD EJECT FAILED", "SD BUS BUSY", "Try again");
            }
        }
    }
}

void updateSpinner() {
    if (sentryModeActive || selfTestActive || manualLogNoticeActive || warningScreenActive || micCamAlert.active || suspiciousAlert.active || possibleBanner.active || currentPage != PAGE_SCANNER) return;

    bool scanning = (pBLEScan != nullptr && pBLEScan->isScanning());

    if (scanning) {
        if (millis() - lastSpinnerFrameMs >= 100) {
            lastSpinnerFrameMs = millis();
            spinnerFrame = (spinnerFrame + 1) & 0x07;
            drawSpinnerFrame(true);
        }
    } else {
        // During the ~3 second gap, leave a faint static ring.
        if (millis() - lastSpinnerFrameMs >= 500) {
            lastSpinnerFrameMs = millis();
            drawSpinnerFrame(false);
        }
    }
}

void updateScreenPower() {
    if (selfTestActive) return;
    if (!warningScreenActive) return;

    bool changed = expireWarningDevices();

    if (warningDeviceCount == 0) {
        warningScreenActive = false;
        screenWakeStarted = 0;
        showSelectedPage();
        return;
    }

    // Re-tile the remaining live warnings only when the active set changes.
    if (changed) {
        redrawWarningScreen();
    }
}

// ---------------- RGB LED ----------------

void rgbWriteBrightness(uint8_t redBrightness,
                        uint8_t greenBrightness,
                        uint8_t blueBrightness) {
    // CYD RGB channels are active-low. analogWrite keeps full-bright and dim
    // states on the same PWM path, avoiding a PWM/digital ownership conflict.
    analogWrite(CYD_LED_RED_PIN,   255 - redBrightness);
    analogWrite(CYD_LED_GREEN_PIN, 255 - greenBrightness);
    analogWrite(CYD_LED_BLUE_PIN,  255 - blueBrightness);
}

void rgbBlue() {
    rgbWriteBrightness(0, 0, 255);
    if (!selfTestActive) ledLogicalState = LED_STATE_BLUE;
}

void rgbGreen() {
    rgbWriteBrightness(0, 255, 0);
    if (!selfTestActive) ledLogicalState = LED_STATE_GREEN;
}

void rgbDimRed() {
    rgbWriteBrightness(SENTRY_RED_BRIGHTNESS, 0, 0);
    if (!selfTestActive) ledLogicalState = LED_STATE_SENTRY_DIM_RED;
}

void rgbOrange() {
    rgbWriteBrightness(255, 180, 0);
    if (!selfTestActive) ledLogicalState = LED_STATE_ORANGE;
}

void rgbRed() {
    rgbWriteBrightness(255, 0, 0);
}

void updateLED() {
    if (sentryModeActive) {
        rgbDimRed();
        return;
    }

    // Self-test owns the LED elsewhere. Operational LED state follows the
    // v6.1 alert presentation exactly: 97..100=red, 60..96=orange, otherwise green.
    if (warningScreenActive && warningDeviceCount > 0) {
        ledLogicalState = LED_STATE_RED;
        rgbRed();
        return;
    }

    if (micCamAlert.active) {
        ledLogicalState = LED_STATE_ORANGE;
        rgbOrange();
        return;
    }

    if (suspiciousAlert.active) {
        ledLogicalState = LED_STATE_ORANGE;
        rgbOrange();
        return;
    }

    if (possibleBanner.active) {
        ledLogicalState = LED_STATE_ORANGE;
        rgbOrange();
        return;
    }

    ledLogicalState = LED_STATE_GREEN;
    rgbGreen();
}

// ---------------- Detection engine ----------------

bool isDeviceCoolingDown(uint32_t deviceHash) {
    uint32_t now = millis();
    for (int i = 0; i < trackedDeviceCount; i++) {
        if (trackedDevices[i].deviceHash == deviceHash) {
            if (now - trackedDevices[i].lastSeen < DETECTION_COOLDOWN_MS) return true;
            trackedDevices[i].lastSeen = now;
            return false;
        }
    }
    return false;
}

void trackDevice(uint32_t deviceHash, int rssi, uint8_t tier, bool hasCamera) {
    uint32_t now = millis();
    for (int i = 0; i < trackedDeviceCount; i++) {
        if (trackedDevices[i].deviceHash == deviceHash) {
            trackedDevices[i].lastSeen = now;
            trackedDevices[i].rssi = rssi;
            trackedDevices[i].tier = tier;
            trackedDevices[i].hasCamera = hasCamera;
            return;
        }
    }

    int slot = trackedDeviceCount;
    if (slot >= MAX_TRACKED_DEVICES) {
        slot = 0;
        for (int i = 1; i < trackedDeviceCount; i++)
            if (trackedDevices[i].lastSeen < trackedDevices[slot].lastSeen) slot = i;
    } else {
        trackedDeviceCount++;
    }

    memset(&trackedDevices[slot], 0, sizeof(TrackedDevice));
    trackedDevices[slot].deviceHash = deviceHash;
    trackedDevices[slot].lastSeen = now;
    trackedDevices[slot].rssi = rssi;
    trackedDevices[slot].tier = tier;
    trackedDevices[slot].hasCamera = hasCamera;
}

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
        const uint8_t* mac = device.getAddress().getNative();
        uint32_t oui = ((uint32_t)mac[0] << 16) |
                       ((uint32_t)mac[1] << 8) |
                       ((uint32_t)mac[2]);
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
    for (int i = 0; rules[i].company != nullptr; i++) {
        const GlassesConfidenceRule& rule = rules[i];
        if (!confidenceRuleMatches(rule, device, address, name, companyId, manufacturerHex)) {
            continue;
        }

        // One advertisement resolves to ONE best result. Equal scores keep the
        // earlier rule, preserving exact-MAC -> fingerprint -> multi-signal priority.
        if (best.matched && rule.confidence <= best.confidence) continue;

        best.matched = true;
        best.detected = (rule.confidence >= POSSIBLE_CONFIDENCE_MIN);
        best.company = rule.company;
        best.product = rule.product;
        best.hasCamera = rule.hasCamera;
        best.cameraKnown = rule.cameraKnown;
        best.tier = tier;
        best.confidence = rule.confidence;
        best.reason = rule.reason ? rule.reason : "Confidence rule match";
    }
}

bool evaluateConfidenceRules(BLEAdvertisedDevice& device, DetectionResult& result) {
    result = {};

    String address = device.getAddress().toString().c_str();
    String name = device.haveName() ? String(device.getName().c_str()) : String("");
    name.trim();
    uint16_t companyId = getCompanyId(device);
    String manufacturerHex = getManufacturerHex(device);

    // Evaluate every tier, but keep only the highest-scoring valid rule.
    // This removes the old v3.4 first-match conflicts/double classifications.
    considerRuleArray(HIGH_CONFIDENCE_RULES, TIER_HIGH,
                      device, address, name, companyId, manufacturerHex, result);
    considerRuleArray(MEDIUM_CONFIDENCE_RULES, TIER_MEDIUM,
                      device, address, name, companyId, manufacturerHex, result);
    considerRuleArray(LOW_CONFIDENCE_RULES, TIER_LOW,
                      device, address, name, companyId, manufacturerHex, result);

    return result.matched;
}

void writeJsonEscaped(Print& out, const char* value) {
    if (value == nullptr) return;
    for (const char* p = value; *p != '\0'; ++p) {
        if (*p == '\\') out.print("\\\\");
        else if (*p == '"') out.print("\\\"");
        else if (*p == '\r' || *p == '\n') out.write(' ');
        else out.write((uint8_t)*p);
    }
}

void writeJsonEscaped(Print& out, const String& value) {
    for (size_t i = 0; i < value.length(); ++i) {
        char c = value[i];
        if (c == '\\') out.print("\\\\");
        else if (c == '"') out.print("\\\"");
        else if (c == '\r' || c == '\n') out.write(' ');
        else out.write((uint8_t)c);
    }
}

void sendDetectionSerial(const BLELogSnapshot& snapshot, const DetectionResult& result) {
    char deviceMacHash[DEVICE_MAC_HASH_LEN];
    formatDeviceMacHashForAddress(
        snapshot.address, deviceMacHash, sizeof(deviceMacHash)
    );
    uint16_t cid = snapshot.companyId;

    Serial.print("{\"type\":\"detection\"");
    Serial.print(",\"deviceMacHash\":\""); writeJsonEscaped(Serial, deviceMacHash); Serial.print("\"");
    Serial.print(",\"name\":\""); writeJsonEscaped(Serial, snapshot.name); Serial.print("\"");
    Serial.print(",\"company\":\""); writeJsonEscaped(Serial, result.company ? result.company : ""); Serial.print("\"");
    Serial.print(",\"product\":\""); writeJsonEscaped(Serial, result.product ? result.product : ""); Serial.print("\"");
    Serial.print(",\"reason\":\""); writeJsonEscaped(Serial, result.reason ? result.reason : ""); Serial.print("\"");
    Serial.print(",\"rssi\":"); Serial.print(snapshot.rssi);
    Serial.print(",\"tier\":\""); Serial.print(tierName(result.tier)); Serial.print("\"");
    Serial.print(",\"confidence\":"); Serial.print(result.confidence);
    Serial.print(",\"alertLevel\":\""); Serial.print(alertLevelForConfidence(result.confidence)); Serial.print("\"");
    Serial.print(",\"cameraStatus\":\"");
    Serial.print(cameraStatusLabel(result));
    Serial.print("\"");

    if (cid != 0xFFFF) {
        char cidHex[7];
        snprintf(cidHex, sizeof(cidHex), "0x%04X", cid);
        Serial.print(",\"companyId\":\""); Serial.print(cidHex); Serial.print("\"");
    }

    Serial.print(",\"ts\":"); Serial.print(millis());
    Serial.println("}");
}

// ---------------- BLE callback ----------------

class GlassholeScanCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        DetectionResult result = {};
        bool matched = evaluateConfidenceRules(advertisedDevice, result);
        bool detected = matched && result.detected;  // confidence >= 60 only

        MicCamMatchResult micCamResult = {};
        bool micCamMatched = evaluateMicCamDevice(advertisedDevice, micCamResult);

        SuspiciousMatchResult suspiciousResult = {};
        bool suspiciousMatched = evaluateSuspiciousDevice(
            advertisedDevice, suspiciousResult
        );

        int rssi = advertisedDevice.getRSSI();

        // Materialize post-detection logging/statistics fields once. These remain
        // callback-local and are discarded when this advertisement returns.
        BLELogSnapshot logSnapshot = {
            String(advertisedDevice.getAddress().toString().c_str()),
            advertisedDevice.haveName()
                ? String(advertisedDevice.getName().c_str())
                : String(""),
            getManufacturerHex(advertisedDevice),
            getServiceUUIDs(advertisedDevice),
            rssi,
            getCompanyId(advertisedDevice),
            (uint8_t)advertisedDevice.getAddress().getType()
        };

        // SYS STAT: count unique known non-glasses BLE devices seen this session.
        // Sentry Mode separately treats a 60%+ glasses candidate as KNOWN.
        bool sentryKnownIdentity = detected;
        {
            const char* knownDeviceId = "";
            const char* knownDeviceType = "";
            const char* fpDeviceId = "";
            const char* fpDeviceType = "";

            bool knownNonGlasses = matchKnownDevice(
                logSnapshot.name, logSnapshot.companyId, logSnapshot.manufacturerHex,
                knownDeviceId, knownDeviceType
            );

            if (!knownNonGlasses) {
                knownNonGlasses = matchFalsePositive(
                    logSnapshot.name, fpDeviceId, fpDeviceType, nullptr
                );
            }

            if (knownNonGlasses) {
                sentryKnownIdentity = true;
                if (rememberKnownNonGlassesDevice(logSnapshot.address)) {
                    sentryDashboardDirty = true;
                }
            }
        }

        if (micCamMatched || suspiciousMatched) {
            // A CAM AND AUDIO or generic review-platform match is still an
            // identified device for the Sentry KNOWN/UNKNOWN counters.
            sentryKnownIdentity = true;
        }

        // v6.1 Sentry Mode counters are maintained continuously, even while
        // the display is asleep or Sentry Mode is not currently selected.
        recordSentryDeviceObservation(logSnapshot.address, sentryKnownIdentity);

        // Maintain chronological manual-context history; every BLE observation
        // eligible for the ring/event is retained in arrival order, including repeats.
        rememberManualObservation(
            logSnapshot, detected, matched ? &result : nullptr
        );

        // Save EVERY advertisement. LOW Company-ID matches are annotation only;
        // the detected column stays 0 until confidence reaches 60.
        logAdvertisement(logSnapshot, detected, matched ? &result : nullptr);

        // Separate CAM AND AUDIO review layer. It never changes glasses
        // confidence. HIGH 97%+ glasses remains the top visual priority.
        if (micCamMatched &&
            !(detected && result.confidence >= HIGH_ALERT_CONFIDENCE)) {
            showMicCamAlert(micCamResult, advertisedDevice);
        }

        // Generic DIY/development/unusual-device review layer. CAM AND AUDIO
        // outranks this more general review screen.
        if (suspiciousMatched && !micCamMatched &&
            !(detected && result.confidence >= HIGH_ALERT_CONFIDENCE)) {
            showSuspiciousAlert(suspiciousResult, advertisedDevice);
        }

        // LOW 1..59: manufacturer intelligence only. No glasses alert event.
        if (!detected) return;

        const char* fpAssumedDevice = "";
        const char* fpAssumedType = "";
        const char* fpReason = "";
        bool falsePositive = matchFalsePositive(
            logSnapshot.name,
            fpAssumedDevice,
            fpAssumedType,
            &fpReason
        );

        // Preserve v3.4's explicit name-based false-positive suppression.
        // Company IDs themselves are never globally blacklisted.
        if (falsePositive) {
            totalFalsePositivesSuppressed++;
            Serial.printf(
                "FALSE POSITIVE SUPPRESSED | name=%s | assumed=%s | type=%s | reason=%s\n",
                logSnapshot.name.c_str(),
                fpAssumedDevice,
                fpAssumedType,
                fpReason
            );
            return;
        }

        uint32_t detectionDeviceHash = sessionDeviceHashForAddress(logSnapshot.address);

        // Preserve the baseline 20-second per-device alert/detection cooldown
        // without retaining the raw BLE MAC beyond this callback.
        if (isDeviceCoolingDown(detectionDeviceHash)) return;

        trackDevice(detectionDeviceHash, rssi, result.tier, result.hasCamera);
        totalDetections++;

        logDetection(logSnapshot, result);
        sendDetectionSerial(logSnapshot, result);

        // v6.1 alert presentation:
        //   60..96  -> orange POSSIBLE banner
        //   97..100 -> red tiled HIGH alert
        // Sentry Mode deliberately suppresses visual alerts only; detection,
        // CSV logging and Serial reporting above continue unchanged.
        if (!sentryModeActive) {
            if (result.confidence >= HIGH_ALERT_CONFIDENCE) {
                drawDetectionAlert(result, advertisedDevice);
            } else if (!micCamMatched && !suspiciousMatched) {
                // CAM AND AUDIO and suspicious-device review screens outrank
                // the normal POSSIBLE glasses banner. Detection is still logged.
                showPossibleAlert(result, advertisedDevice);
            }
        }
    }
};

void onBLEScanComplete(BLEScanResults results) {
    (void)results;
    scanFinishedFlag = true;
}

// ---------------- Setup / loop ----------------

void printSerialSystemStats(uint32_t nowMs) {
    bool sdPresent = (sdStatus != SD_STATUS_NOT_DETECTED);

    Serial.println();
    Serial.println("============================================================");
    Serial.println("CYD DEV LITE - SYS STAT");
    Serial.println("============================================================");

    Serial.print("Session: ");
    Serial.println(sessionId);

    Serial.print("Board: ");
    Serial.println(BOARD_TYPE);

    Serial.print("Uptime: ");
    Serial.println(uptimeText());

    Serial.print("Free Heap (RAM): ");
    Serial.println(ESP.getFreeHeap());

    Serial.print("Scans: ");
    Serial.println(totalScans);

    Serial.print("Sentry Mode: ");
    Serial.println(sentryModeActive ? "YES" : "NO");

    Serial.print("Sentry state: ");
    Serial.println(sentryModeActive ? sentryStateText() : "OFF");

    Serial.print("Sentry baseline (unique/10s): ");
    if (sentryBaselineValid) Serial.println(sentryBaseline);
    else Serial.println("UNSET");

    Serial.print("Sentry last sample: ");
    Serial.println(sentryLastSample);

    Serial.print("Sentry trigger threshold: >");
    if (sentryBaselineValid) Serial.println(calculateSentryTriggerThreshold());
    else Serial.println("UNSET");

    Serial.print("Individual devices: ");
    Serial.println(sentryIndividualDeviceCount);

    Serial.print("Known devices: ");
    Serial.println(sentryKnownDeviceCount);

    Serial.print("Unknown devices: ");
    Serial.println(sentryUnknownDeviceCount());

    Serial.print("Duplicate observations: ");
    Serial.println(sentryDuplicateDeviceCount);

    Serial.print("Sentry session scans: ");
    Serial.println(sentrySessionScanCount);

    Serial.print("Logging: ");
    Serial.println(loggingHealthy() ? "YES" : "NO");

    Serial.print("Advertised IDs captured: ");
    Serial.println(totalAdvertisements);

    Serial.print("Unique IDs written: ");
    Serial.println(totalUniqueAdvertisements);

    Serial.print("Detections: ");
    Serial.println(totalDetections);

    Serial.print("False Positive Suppressed: ");
    Serial.println(totalFalsePositivesSuppressed);

    Serial.print("CAM AND AUDIO alerts: ");
    Serial.println(totalMicCamReviews);

    Serial.print("Suspicious / review alerts: ");
    Serial.println(totalSuspiciousDeviceReviews);

    Serial.print("Known non-glasses BLE devices: ");
    Serial.println(knownNonGlassesDeviceCount);

    Serial.print("Watched Devices: ");
    Serial.println(trackedDeviceCount);

    Serial.print("BLE: ");
    Serial.println(bleScanningAtBoot ? "YES" : "NO");

    Serial.print("BLE scanner address: ");
    Serial.println(scannerPrivateAddressReady ? scannerPrivateAddressText : "PRIVACY ERROR");

    Serial.print("BLE scanner address type: ");
    Serial.println(scannerPrivateAddressReady ? "PRIVATE NRPA" : "NOT CONFIGURED");

    Serial.print("BLE private rotations: ");
    Serial.println(scannerPrivateAddressRotationCount);

    Serial.print("SD: ");
    Serial.println(sdPresent ? "PRESENT" : "NOT PRESENT");

    Serial.print("SD state: ");
    Serial.println(sdStatusText());

    Serial.print("Operational LED: ");
    Serial.println(ledStatusText());

    Serial.print("Last BLE advertisement RSSI: ");
    if (lastRSSI > -127) {
        Serial.print(lastRSSI);
        Serial.print(" dBm");

        float approxM = approximateDistanceMetres(lastRSSI);
        Serial.print("  |  approx ");
        Serial.print(approxM, 1);
        Serial.println(" m");
    } else {
        Serial.println("NO BLE YET");
    }

    Serial.println("============================================================");
    Serial.println();
}

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(300);

    pinMode(CYD_LED_RED_PIN, OUTPUT);
    pinMode(CYD_LED_GREEN_PIN, OUTPUT);
    pinMode(CYD_LED_BLUE_PIN, OUTPUT);

    // v6.1: BLUE is the dedicated boot/setup light.
    rgbBlue();

    pinMode(PAGE_BUTTON_PIN, INPUT_PULLUP);
    pinMode(TOUCH_IRQ_PIN, INPUT);
    lastSentryTouchPressed = (digitalRead(TOUCH_IRQ_PIN) == LOW);

    pinMode(TFT_BL, OUTPUT);

    uiMutex = xSemaphoreCreateMutex();
    manualContextMutex = xSemaphoreCreateMutex();
    sdMutex = xSemaphoreCreateMutex();
    sentryActivityMutex = xSemaphoreCreateMutex();

    initSessionId();
    loadSentryBaseline();

    // Keep the backlight OFF only while the TFT controller initializes.
    digitalWrite(TFT_BL, LOW);

    tft.init();
    tft.setRotation(3);

    // Legal/privacy gate: BLE scanning is not initialized or started until a
    // fresh touch is received on this boot page.
    drawBootLegalNotice();
    waitForBootTouchToStart();

    refreshSDStatus(true);
    if (sdOK) writeSessionStartRecord("BOOT");

    Serial.println();
    Serial.println("========================================");
    Serial.println(" ESP-GlassHole CYD + FULL BLE SD LOGGER");
    Serial.println("========================================");
    Serial.println("RSSI match processing floor: NONE");
    Serial.println("TFT warning RSSI threshold: NONE");
    Serial.printf("Low Company-ID clue rules: %u\n",
                  (unsigned)((sizeof(LOW_CONFIDENCE_RULES) / sizeof(LOW_CONFIDENCE_RULES[0])) - 1));
    Serial.println("Alert policy: 1-59 LOG ONLY | 60-96 ORANGE POSSIBLE | 97-100 RED HIGH");
    Serial.printf("SD: %s\n", sdStatusText());
    Serial.println("ALL received advertisements are logged before detection filtering.");
    Serial.println();

    // Scanner-only radio setup.
    // No Wi-Fi library is included or initialized anywhere in this sketch.
    // BLE advertising is never created or started; only scanning is used.
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new GlassholeScanCallbacks(), true);
    pBLEScan->setActiveScan(true);

    // Arduino-ESP32 BLEScan::setInterval/setWindow take milliseconds.
    pBLEScan->setInterval(BLE_SCAN_INTERVAL_MS);
    // 95% BLE scan duty: 100 ms interval, 95 ms window.
    pBLEScan->setWindow(BLE_SCAN_WINDOW_MS);

    // CYD Dev Lite RF privacy: create and confirm a fresh PRIVATE NRPA before the
    // first scan can start. Scan Requests are forced to RANDOM own-address type.
    scannerPrivateAddressReady = applyFreshScannerPrivateNrpa(true);
    bleScanningAtBoot = (pBLEScan != nullptr && scannerPrivateAddressReady);
    if (!scannerPrivateAddressReady) {
        scannerPrivateAddressNextRotateMs = millis() + BLE_PRIVATE_ADDR_RETRY_MS;
        Serial.println("{\"type\":\"ble_privacy_error\",\"message\":\"Scanner disabled until PRIVATE NRPA is confirmed\"}");
    }

    currentPage = PAGE_SCANNER;
    rgbGreen();
    showScanningNow();

    lastStatusTime = millis();
}

void loop() {
    uint32_t now = millis();

    // CYD Dev Lite: rotate/recover the CYD's own PRIVATE NRPA only while BLE is idle.
    updateScannerPrivateAddressRotation(now);

    // v6.1: reset the session counter/identifier to DEVICE_NAME-S0001 after
    // each 24 hours of powered runtime. The reset waits until BLE is idle.
    checkSession24HourReset();

    // v6.1: verify the SD state every 10 seconds between BLE scans.
    // This covers mounted-card removal, normal insertion/remount, and automatic
    // reinsertion detection after SAFE EJECT.
    if (pBLEScan != nullptr && !pBLEScan->isScanning()) {
        if (refreshSDStatus(false) && !warningScreenActive && !micCamAlert.active && !suspiciousAlert.active && !manualLogNoticeActive) {
            showSelectedPage();
        }
    }

    // Sentry state transitions are evaluated BEFORE starting the next BLE scan.
    // In Sentry sleep, the radio stays idle for the configured interval. SAMPLE and ACTIVE
    // chain the existing 2-second asynchronous scans back-to-back.
    updateSentryEngine();

    if (scannerPrivateAddressReady && !pBLEScan->isScanning()) {
        bool scanDue = false;

        if (sentryModeActive) {
            scanDue = (sentryState == SENTRY_STATE_SAMPLE ||
                       sentryState == SENTRY_STATE_ACTIVE);
        } else {
            scanDue = firstScanPending ||
                      ((uint32_t)(now - lastScanStartMs) >= BLE_SCAN_CYCLE_MS);
        }

        if (scanDue) {
            if (!scannerPrivateAddressConfirmed()) {
                scannerPrivateAddressReady = false;
                bleScanningAtBoot = false;
                scannerPrivateAddressNextRotateMs = now + BLE_PRIVATE_ADDR_RETRY_MS;
                Serial.println("{\"type\":\"ble_privacy_error\",\"step\":\"pre_scan_verify\",\"message\":\"PRIVATE NRPA no longer confirmed; scan blocked\"}");
                return;
            }

            firstScanPending = false;
            lastScanStartMs = now;
            scanFinishedFlag = false;

            bool started = pBLEScan->start(
                BLE_SCAN_TIME,
                onBLEScanComplete,
                false
            );

            if (started) {
                totalScans++;
                sentrySessionScanCount++;
                sentryDashboardDirty = true;

                // Do not replace a warning or the user's selected status page.
                if (!warningScreenActive &&
                    !micCamAlert.active &&
                    !suspiciousAlert.active &&
                    !manualLogNoticeActive &&
                    !sentryModeActive &&
                    currentPage == PAGE_SCANNER) {
                    showScanningNow();
                }
            } else {
                Serial.println("{\"type\":\"scan_error\",\"message\":\"BLE scan failed to start\"}");
            }
        }
    }

    // Clear stored scan results after the asynchronous scan completes.
    if (scanFinishedFlag) {
        scanFinishedFlag = false;
        pBLEScan->clearResults();
    }

    // Keep the loading circle moving during the actual active scan.
    updateSpinner();

    // Every positive device warning tile has its own independent 10-second
    // lifetime; expired tiles are removed and the remaining grid reflows.
    updateScreenPower();

    // Orange possible-match banner has its own 10-second lifetime.
    updatePossibleBanner();

    // Separate orange CAM AND AUDIO review screen lifetime.
    updateMicCamAlert();

    // Separate orange suspicious-device review screen lifetime.
    updateSuspiciousAlert();

    // Scroll long matched/confirmed product names inside active alert tiles.
    updateWarningLabelScroll();

    // Sentry Mode touch handling is presentation-only. The Sentry activity state
    // machine runs above before scan scheduling.
    updateSentryMode();

    // Self-test owns the RGB LED while active.
    if (!selfTestActive) {
        updateLED();
    }

    updatePageButton();
    updateSelfTest();
    processManualContextWrite();
    updateManualLogNotice();
    updateStatusPage();

    now = millis();

    // Human-readable serial mirror of the SYS STAT page.
    // Useful when the CYD is connected to a computer with Serial Monitor open.
    if (now - lastSerialStatsTime >= SERIAL_STATS_INTERVAL_MS) {
        printSerialSystemStats(now);
        lastSerialStatsTime = now;
    }

    // Existing machine-readable JSON status output remains every 60 seconds.
    if (now - lastStatusTime >= STATUS_INTERVAL_MS) {
        Serial.printf("SESSION: %s | LED: %s | FP suppressed: %lu\n",
                      sessionId,
                      ledStatusText(),
                      (unsigned long)totalFalsePositivesSuppressed);
        Serial.printf(
            "{\"type\":\"status\",\"board\":\"%s\",\"uptime\":%lu,"
            "\"freeHeap\":%u,\"totalScans\":%lu,\"totalAdvertisements\":%lu,"
            "\"uniqueAdvertisementsWritten\":%lu,\"totalDetections\":%lu,"
            "\"knownNonGlassesDevices\":%u,\"trackedDevices\":%d,"
            "\"sd\":%s,\"scanning\":%s}\n",
            BOARD_TYPE,
            (unsigned long)(now / 1000),
            ESP.getFreeHeap(),
            (unsigned long)totalScans,
            (unsigned long)totalAdvertisements,
            (unsigned long)totalUniqueAdvertisements,
            (unsigned long)totalDetections,
            (unsigned int)knownNonGlassesDeviceCount,
            trackedDeviceCount,
            sdOK ? "true" : "false",
            pBLEScan->isScanning() ? "true" : "false"
        );
        lastStatusTime = now;
    }

    delay(20);
}
