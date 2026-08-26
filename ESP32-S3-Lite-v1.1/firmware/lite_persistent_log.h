#pragma once

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <esp_system.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/*
 * ============================================================================
 * ESP32 S3 Glasses Scanner Lite v1.1 — persistent field-test log
 * ============================================================================
 *
 * PURPOSE
 *   Persist only BLE observations classified at >= 40% plus session/reset
 *   metadata. Ordinary BLE advertisements below 40% are never written.
 *
 * PRIVACY BOUNDARY
 *   - raw observed BLE MAC addresses are NEVER written to LittleFS/NVS
 *   - each boot/session gets a fresh 64-bit RAM-only salt
 *   - observed addresses are converted to a session-scoped 64-bit pseudonym
 *   - the salt is not written to LittleFS, Preferences/NVS, or RTC memory
 *   - the same stable address normally maps to the same pseudonym in one
 *     session and a different pseudonym after the next boot/session
 *
 * STORAGE MODEL
 *   - Preferences/NVS: lifetime boot/session/privacy counters + expected-restart
 *     marker only
 *   - LittleFS: one fixed-record file per session + one reset-history file
 *   - current-session candidate records are deduplicated in RAM by pseudonym
 *   - first records and alert/significant updates are marked urgent
 *   - ordinary repeated count/RSSI changes checkpoint at a slower interval
 *   - writes are performed from loop() while the BLE scan is idle
 *
 * No automatic retention expiry is performed. If the filesystem becomes full,
 * existing records are preserved and new writes report an error rather than
 * silently deleting older field-test evidence.
 * ============================================================================
 */

#define LITE11_FIRMWARE_NAME              "ESP32 S3 Glasses Scanner Lite v1.1"
#define LITE11_LOG_FORMAT_VERSION         1U
#define LITE11_MAX_SESSION_RECORDS        256U
#define LITE11_RECORD_CHECKPOINT_MS       (5UL * 60UL * 1000UL)
#define LITE11_MIN_TARGET_FS_BYTES        (1536UL * 1024UL)

#define LITE11_SESSION_MAGIC              0x4C533131UL  /* LS11 */
#define LITE11_RECORD_MAGIC               0x52433131UL  /* RC11 */
#define LITE11_RESET_MAGIC                0x52533131UL  /* RS11 */

#define LITE11_COMPANY_ID_NONE            0xFFFFU

#if LITE11_MAX_SESSION_RECORDS == 0
#error "LITE11_MAX_SESSION_RECORDS must be greater than zero"
#endif

enum Lite11PrivacyMode : uint8_t {
    LITE11_PRIVACY_STARTING = 0,
    LITE11_PRIVACY_PRIVATE_NRPA = 1,
    LITE11_PRIVACY_PUBLIC_FALLBACK = 2
};

enum Lite11RecordSource : uint8_t {
    LITE11_SOURCE_GLASSES = 1,
    LITE11_SOURCE_CAMERA_AUDIO = 2
};

enum Lite11ClassFlags : uint8_t {
    LITE11_CLASS_GLASSES = 0x01,
    LITE11_CLASS_CAMERA_AUDIO = 0x02,
    LITE11_CLASS_FALSE_POSITIVE_SUPPRESSED = 0x04,
    LITE11_CLASS_KNOWN_DEVICE = 0x08
};

enum Lite11AlertFlags : uint8_t {
    LITE11_ALERT_NONE = 0x00,
    LITE11_ALERT_POSSIBLE_GLASSES = 0x01,
    LITE11_ALERT_HIGH_GLASSES = 0x02,
    LITE11_ALERT_CAMERA_AUDIO = 0x04
};

enum Lite11PreviousEndClass : uint8_t {
    LITE11_END_UNCONFIRMED = 0,
    LITE11_END_EXPECTED_RESTART = 1,
    LITE11_END_FAULT_RESET = 2
};

struct Lite11SessionHeader {
    uint32_t magic;
    uint16_t formatVersion;
    uint16_t headerSize;
    uint32_t sessionId;
    uint32_t bootCount;
    uint32_t resetReason;
    uint32_t recordCount;
    uint32_t alertEpisodes;
    uint32_t droppedCandidates;
    uint32_t storageWriteErrors;
    uint32_t privateFailuresThisSession;
    uint8_t privacyMode;
    uint8_t previousEndClass;
    uint8_t reserved[14];
    uint32_t crc32;
};

struct Lite11CandidateRecord {
    uint32_t magic;
    uint16_t formatVersion;
    uint16_t recordSize;
    uint32_t sessionId;
    uint64_t deviceHash;
    uint32_t firstSeenUptimeMs;
    uint32_t lastSeenUptimeMs;
    uint32_t observationCount;
    int8_t strongestRssi;
    int8_t weakestRssi;
    uint16_t companyId;
    uint16_t relevantUuid16;
    uint8_t currentConfidence;
    uint8_t highestConfidence;
    uint8_t tier;
    uint8_t source;
    uint8_t classFlags;
    uint8_t alertFlags;
    uint8_t falsePositiveSuppressed;
    uint8_t reserved0;
    char classification[32];
    char signature[24];
    uint32_t crc32;
};

struct Lite11ResetRecord {
    uint32_t magic;
    uint16_t formatVersion;
    uint16_t recordSize;
    uint32_t bootCount;
    uint32_t newSessionId;
    uint32_t previousSessionId;
    uint32_t resetReason;
    uint8_t previousEndClass;
    uint8_t faultLike;
    uint8_t reserved[14];
    uint32_t crc32;
};

struct Lite11ObservationInput {
    uint64_t deviceHash;
    uint32_t uptimeMs;
    int8_t rssi;
    uint16_t companyId;
    uint16_t relevantUuid16;
    uint8_t confidence;
    uint8_t tier;
    uint8_t source;
    uint8_t classFlags;
    uint8_t alertFlags;
    bool falsePositiveSuppressed;
    const char* classification;
    const char* signature;
};

static Preferences lite11Prefs;
static bool lite11PrefsReady = false;
static bool lite11FsReady = false;
static bool lite11SessionHeaderDirty = false;
static bool lite11SessionHeaderUrgent = false;
static uint32_t lite11SessionIdValue = 0;
static uint32_t lite11BootCountValue = 0;
static uint64_t lite11SessionSalt = 0;
static Lite11SessionHeader lite11SessionHeader = {};
static Lite11CandidateRecord lite11Records[LITE11_MAX_SESSION_RECORDS];
static bool lite11RecordDirty[LITE11_MAX_SESSION_RECORDS] = {};
static bool lite11RecordUrgent[LITE11_MAX_SESSION_RECORDS] = {};
static uint32_t lite11RecordLastFlushMs[LITE11_MAX_SESSION_RECORDS] = {};
static uint16_t lite11RecordCountRam = 0;
static portMUX_TYPE lite11LogMux = portMUX_INITIALIZER_UNLOCKED;

static inline uint32_t lite11Crc32(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFFUL;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            uint32_t mask = (uint32_t)-(int32_t)(crc & 1U);
            crc = (crc >> 1) ^ (0xEDB88320UL & mask);
        }
    }
    return ~crc;
}

static inline uint32_t lite11SessionHeaderCrc(const Lite11SessionHeader& h) {
    return lite11Crc32((const uint8_t*)&h, offsetof(Lite11SessionHeader, crc32));
}

static inline uint32_t lite11CandidateCrc(const Lite11CandidateRecord& r) {
    return lite11Crc32((const uint8_t*)&r, offsetof(Lite11CandidateRecord, crc32));
}

static inline uint32_t lite11ResetCrc(const Lite11ResetRecord& r) {
    return lite11Crc32((const uint8_t*)&r, offsetof(Lite11ResetRecord, crc32));
}

static inline void lite11CopyText(char* dst, size_t dstSize, const char* src) {
    if (dst == nullptr || dstSize == 0) return;
    if (src == nullptr) src = "";
    strncpy(dst, src, dstSize - 1);
    dst[dstSize - 1] = '\0';
}

static inline void lite11SessionPath(uint32_t sessionId, char* out, size_t outSize) {
    if (out == nullptr || outSize == 0) return;
    snprintf(out, outSize, "/S%08lu.bin", (unsigned long)sessionId);
}

static inline const char* lite11PrivacyModeText(uint8_t mode) {
    switch (mode) {
        case LITE11_PRIVACY_PRIVATE_NRPA: return "PRIVATE_NRPA";
        case LITE11_PRIVACY_PUBLIC_FALLBACK: return "PUBLIC_FALLBACK";
        default: return "STARTING";
    }
}

static inline const char* lite11PreviousEndText(uint8_t endClass) {
    switch (endClass) {
        case LITE11_END_EXPECTED_RESTART: return "EXPECTED_RESTART";
        case LITE11_END_FAULT_RESET: return "FAULT_RESET";
        default: return "UNCONFIRMED";
    }
}

static inline const char* lite11ResetReasonText(uint32_t reason) {
    switch ((esp_reset_reason_t)reason) {
        case ESP_RST_UNKNOWN: return "UNKNOWN";
        case ESP_RST_POWERON: return "POWER_ON";
        case ESP_RST_EXT: return "EXTERNAL_PIN";
        case ESP_RST_SW: return "SOFTWARE_RESTART";
        case ESP_RST_PANIC: return "PANIC_EXCEPTION";
        case ESP_RST_INT_WDT: return "INTERRUPT_WATCHDOG";
        case ESP_RST_TASK_WDT: return "TASK_WATCHDOG";
        case ESP_RST_WDT: return "OTHER_WATCHDOG";
        case ESP_RST_DEEPSLEEP: return "DEEP_SLEEP_WAKE";
        case ESP_RST_BROWNOUT: return "BROWNOUT";
        case ESP_RST_SDIO: return "SDIO_RESET";
        case ESP_RST_USB: return "USB_RESET";
        case ESP_RST_JTAG: return "JTAG_RESET";
        case ESP_RST_EFUSE: return "EFUSE_ERROR";
        case ESP_RST_PWR_GLITCH: return "POWER_GLITCH";
        case ESP_RST_CPU_LOCKUP: return "CPU_LOCKUP";
        default: return "OTHER_RESET";
    }
}

static inline bool lite11ResetReasonFaultLike(esp_reset_reason_t reason) {
    switch (reason) {
        case ESP_RST_PANIC:
        case ESP_RST_INT_WDT:
        case ESP_RST_TASK_WDT:
        case ESP_RST_WDT:
        case ESP_RST_BROWNOUT:
        case ESP_RST_EFUSE:
        case ESP_RST_PWR_GLITCH:
        case ESP_RST_CPU_LOCKUP:
            return true;
        default:
            return false;
    }
}

static inline uint8_t lite11DeterminePreviousEnd(esp_reset_reason_t reason,
                                                  bool expectedRestart,
                                                  uint32_t expectedSession,
                                                  uint32_t previousSession) {
    if (reason == ESP_RST_SW && expectedRestart && expectedSession == previousSession) {
        return LITE11_END_EXPECTED_RESTART;
    }
    if (lite11ResetReasonFaultLike(reason)) return LITE11_END_FAULT_RESET;
    return LITE11_END_UNCONFIRMED;
}

static inline uint64_t lite11NewSessionSalt() {
    uint64_t a = (uint64_t)esp_random();
    uint64_t b = (uint64_t)esp_random();
    uint64_t salt = (a << 32) ^ b ^ ((uint64_t)micros() << 17);
    if (salt == 0) salt = 0x9E3779B97F4A7C15ULL ^ (uint64_t)esp_random();
    return salt;
}

static inline uint64_t lite11HashObservedAddress(const uint8_t nativeAddress[6]) {
    if (nativeAddress == nullptr) return 0;

    // Keyed FNV-1a style mixer. The 64-bit salt changes every session and is
    // RAM-only; this value is a session pseudonym, not a permanent identity.
    uint64_t h = 1469598103934665603ULL ^ lite11SessionSalt;
    for (uint8_t i = 0; i < 6; ++i) {
        h ^= (uint64_t)nativeAddress[i];
        h *= 1099511628211ULL;
        h ^= (lite11SessionSalt >> ((i * 9U) & 63U));
    }
    h ^= (h >> 33);
    h *= 0xff51afd7ed558ccdULL;
    h ^= (h >> 33);
    if (h == 0) h = 1;
    return h;
}

static inline bool lite11WriteSessionHeaderNow() {
    if (!lite11FsReady || lite11SessionIdValue == 0) return false;

    Lite11SessionHeader snap;
    portENTER_CRITICAL(&lite11LogMux);
    snap = lite11SessionHeader;
    snap.recordCount = lite11RecordCountRam;
    snap.crc32 = lite11SessionHeaderCrc(snap);
    lite11SessionHeaderDirty = false;
    lite11SessionHeaderUrgent = false;
    portEXIT_CRITICAL(&lite11LogMux);

    char path[24];
    lite11SessionPath(lite11SessionIdValue, path, sizeof(path));
    File f = LittleFS.open(path, "r+");
    if (!f) {
        portENTER_CRITICAL(&lite11LogMux);
        ++lite11SessionHeader.storageWriteErrors;
        lite11SessionHeaderDirty = true;
        portEXIT_CRITICAL(&lite11LogMux);
        return false;
    }

    bool ok = f.seek(0, SeekSet) && (f.write((const uint8_t*)&snap, sizeof(snap)) == sizeof(snap));
    f.flush();
    f.close();

    if (!ok) {
        portENTER_CRITICAL(&lite11LogMux);
        ++lite11SessionHeader.storageWriteErrors;
        lite11SessionHeaderDirty = true;
        portEXIT_CRITICAL(&lite11LogMux);
    }
    return ok;
}

static inline bool lite11WriteCandidateSlotNow(uint16_t index, const Lite11CandidateRecord& input) {
    if (!lite11FsReady || index >= LITE11_MAX_SESSION_RECORDS) return false;

    Lite11CandidateRecord record = input;
    record.crc32 = lite11CandidateCrc(record);

    char path[24];
    lite11SessionPath(lite11SessionIdValue, path, sizeof(path));
    File f = LittleFS.open(path, "r+");
    if (!f) return false;

    const size_t offset = sizeof(Lite11SessionHeader) + ((size_t)index * sizeof(Lite11CandidateRecord));
    bool ok = f.seek(offset, SeekSet) &&
              (f.write((const uint8_t*)&record, sizeof(record)) == sizeof(record));
    f.flush();
    f.close();
    return ok;
}

static inline bool lite11AppendResetRecord(const Lite11ResetRecord& input) {
    if (!lite11FsReady) return false;
    Lite11ResetRecord record = input;
    record.crc32 = lite11ResetCrc(record);
    File f = LittleFS.open("/resets.bin", FILE_APPEND);
    if (!f) return false;
    bool ok = (f.write((const uint8_t*)&record, sizeof(record)) == sizeof(record));
    f.flush();
    f.close();
    return ok;
}

static inline bool lite11CreateCurrentSessionFile() {
    if (!lite11FsReady) return false;
    char path[24];
    lite11SessionPath(lite11SessionIdValue, path, sizeof(path));

    File f = LittleFS.open(path, FILE_WRITE);
    if (!f) return false;

    Lite11SessionHeader snap = lite11SessionHeader;
    snap.recordCount = 0;
    snap.crc32 = lite11SessionHeaderCrc(snap);
    bool ok = (f.write((const uint8_t*)&snap, sizeof(snap)) == sizeof(snap));
    f.flush();
    f.close();
    return ok;
}

static inline bool lite11MountFsSafely() {
    bool provisioned = lite11PrefsReady ? lite11Prefs.getBool("fsprov", false) : false;

    if (LittleFS.begin(false)) {
        lite11FsReady = true;
        if (lite11PrefsReady && !provisioned) lite11Prefs.putBool("fsprov", true);
        return true;
    }

    // Format only on an unprovisioned first installation. Once the firmware has
    // successfully provisioned LittleFS, a later mount failure does NOT format
    // automatically because preserving existing evidence takes priority.
    if (!provisioned) {
        if (LittleFS.format() && LittleFS.begin(false)) {
            lite11FsReady = true;
            if (lite11PrefsReady) lite11Prefs.putBool("fsprov", true);
            return true;
        }
    }

    lite11FsReady = false;
    return false;
}

static inline bool lite11PersistentBegin() {
    memset(lite11Records, 0, sizeof(lite11Records));
    memset(lite11RecordDirty, 0, sizeof(lite11RecordDirty));
    memset(lite11RecordUrgent, 0, sizeof(lite11RecordUrgent));
    memset(lite11RecordLastFlushMs, 0, sizeof(lite11RecordLastFlushMs));
    lite11RecordCountRam = 0;

    lite11PrefsReady = lite11Prefs.begin("s3lite11", false);

    uint32_t previousSessionId = 0;
    bool expectedRestart = false;
    uint32_t expectedSession = 0;

    if (lite11PrefsReady) {
        previousSessionId = lite11Prefs.getUInt("session", 0);
        expectedRestart = lite11Prefs.getBool("expect", false);
        expectedSession = lite11Prefs.getUInt("expSess", 0);

        lite11BootCountValue = lite11Prefs.getUInt("boots", 0) + 1U;
        lite11SessionIdValue = previousSessionId + 1U;

        lite11Prefs.putUInt("boots", lite11BootCountValue);
        lite11Prefs.putUInt("session", lite11SessionIdValue);
        lite11Prefs.putBool("expect", false);
        lite11Prefs.putUInt("expSess", 0);
    } else {
        // Scanner still runs if NVS is unavailable, but lifetime counters cannot
        // be guaranteed. Use session/boot 1 for the current runtime only.
        lite11BootCountValue = 1;
        lite11SessionIdValue = 1;
    }

    lite11SessionSalt = lite11NewSessionSalt();
    esp_reset_reason_t reason = esp_reset_reason();
    uint8_t previousEnd = lite11DeterminePreviousEnd(reason,
                                                     expectedRestart,
                                                     expectedSession,
                                                     previousSessionId);

    lite11MountFsSafely();

    memset(&lite11SessionHeader, 0, sizeof(lite11SessionHeader));
    lite11SessionHeader.magic = LITE11_SESSION_MAGIC;
    lite11SessionHeader.formatVersion = LITE11_LOG_FORMAT_VERSION;
    lite11SessionHeader.headerSize = sizeof(Lite11SessionHeader);
    lite11SessionHeader.sessionId = lite11SessionIdValue;
    lite11SessionHeader.bootCount = lite11BootCountValue;
    lite11SessionHeader.resetReason = (uint32_t)reason;
    lite11SessionHeader.privacyMode = LITE11_PRIVACY_STARTING;
    lite11SessionHeader.previousEndClass = previousEnd;
    lite11SessionHeader.crc32 = lite11SessionHeaderCrc(lite11SessionHeader);

    if (lite11FsReady) {
        if (!lite11CreateCurrentSessionFile()) {
            lite11FsReady = false;
        } else {
            Lite11ResetRecord resetRecord = {};
            resetRecord.magic = LITE11_RESET_MAGIC;
            resetRecord.formatVersion = LITE11_LOG_FORMAT_VERSION;
            resetRecord.recordSize = sizeof(Lite11ResetRecord);
            resetRecord.bootCount = lite11BootCountValue;
            resetRecord.newSessionId = lite11SessionIdValue;
            resetRecord.previousSessionId = previousSessionId;
            resetRecord.resetReason = (uint32_t)reason;
            resetRecord.previousEndClass = previousEnd;
            resetRecord.faultLike = lite11ResetReasonFaultLike(reason) ? 1 : 0;
            lite11AppendResetRecord(resetRecord);
        }
    }

    return lite11FsReady;
}

static inline uint32_t lite11CurrentSessionId() { return lite11SessionIdValue; }
static inline uint32_t lite11LifetimeBootCount() { return lite11BootCountValue; }
static inline bool lite11StorageReady() { return lite11FsReady; }

static inline void lite11NotePrivateStartSuccess() {
    if (lite11PrefsReady) {
        uint32_t value = lite11Prefs.getUInt("privOK", 0);
        if (value != UINT32_MAX) ++value;
        lite11Prefs.putUInt("privOK", value);
    }
    portENTER_CRITICAL(&lite11LogMux);
    lite11SessionHeader.privacyMode = LITE11_PRIVACY_PRIVATE_NRPA;
    lite11SessionHeaderDirty = true;
    lite11SessionHeaderUrgent = true;
    portEXIT_CRITICAL(&lite11LogMux);
}

static inline void lite11NotePrivateFailure() {
    if (lite11PrefsReady) {
        uint32_t value = lite11Prefs.getUInt("privFail", 0);
        if (value != UINT32_MAX) ++value;
        lite11Prefs.putUInt("privFail", value);
    }
    portENTER_CRITICAL(&lite11LogMux);
    if (lite11SessionHeader.privateFailuresThisSession != UINT32_MAX) {
        ++lite11SessionHeader.privateFailuresThisSession;
    }
    lite11SessionHeaderDirty = true;
    lite11SessionHeaderUrgent = true;
    portEXIT_CRITICAL(&lite11LogMux);
}

static inline void lite11NotePublicFallback() {
    if (lite11PrefsReady) {
        uint32_t value = lite11Prefs.getUInt("public", 0);
        if (value != UINT32_MAX) ++value;
        lite11Prefs.putUInt("public", value);
    }
    portENTER_CRITICAL(&lite11LogMux);
    lite11SessionHeader.privacyMode = LITE11_PRIVACY_PUBLIC_FALLBACK;
    lite11SessionHeaderDirty = true;
    lite11SessionHeaderUrgent = true;
    portEXIT_CRITICAL(&lite11LogMux);
}

static inline void lite11NoteAlertEpisode() {
    portENTER_CRITICAL(&lite11LogMux);
    if (lite11SessionHeader.alertEpisodes != UINT32_MAX) ++lite11SessionHeader.alertEpisodes;
    lite11SessionHeaderDirty = true;
    lite11SessionHeaderUrgent = true;
    portEXIT_CRITICAL(&lite11LogMux);
}

static inline int16_t lite11FindRecordIndex(uint64_t deviceHash) {
    for (uint16_t i = 0; i < lite11RecordCountRam; ++i) {
        if (lite11Records[i].deviceHash == deviceHash) return (int16_t)i;
    }
    return -1;
}

static inline bool lite11ObserveCandidate(const Lite11ObservationInput& input) {
    if (input.deviceHash == 0 || input.confidence < 40) return false;

    bool accepted = true;
    uint32_t now = input.uptimeMs;

    portENTER_CRITICAL(&lite11LogMux);

    int16_t found = lite11FindRecordIndex(input.deviceHash);
    uint16_t index;
    bool isNew = false;

    if (found < 0) {
        if (lite11RecordCountRam >= LITE11_MAX_SESSION_RECORDS) {
            if (lite11SessionHeader.droppedCandidates != UINT32_MAX) {
                ++lite11SessionHeader.droppedCandidates;
            }
            lite11SessionHeaderDirty = true;
            accepted = false;
            portEXIT_CRITICAL(&lite11LogMux);
            return false;
        }

        index = lite11RecordCountRam++;
        isNew = true;
        memset(&lite11Records[index], 0, sizeof(Lite11CandidateRecord));
        Lite11CandidateRecord& r = lite11Records[index];
        r.magic = LITE11_RECORD_MAGIC;
        r.formatVersion = LITE11_LOG_FORMAT_VERSION;
        r.recordSize = sizeof(Lite11CandidateRecord);
        r.sessionId = lite11SessionIdValue;
        r.deviceHash = input.deviceHash;
        r.firstSeenUptimeMs = now;
        r.lastSeenUptimeMs = now;
        r.observationCount = 1;
        r.strongestRssi = input.rssi;
        r.weakestRssi = input.rssi;
        r.companyId = input.companyId;
        r.relevantUuid16 = input.relevantUuid16;
        r.currentConfidence = input.confidence;
        r.highestConfidence = input.confidence;
        r.tier = input.tier;
        r.source = input.source;
        r.classFlags = input.classFlags;
        r.alertFlags = input.alertFlags;
        r.falsePositiveSuppressed = input.falsePositiveSuppressed ? 1 : 0;
        lite11CopyText(r.classification, sizeof(r.classification), input.classification);
        lite11CopyText(r.signature, sizeof(r.signature), input.signature);
        lite11RecordDirty[index] = true;
        lite11RecordUrgent[index] = true;
        lite11SessionHeader.recordCount = lite11RecordCountRam;
        lite11SessionHeaderDirty = true;
        lite11SessionHeaderUrgent = true;
        portEXIT_CRITICAL(&lite11LogMux);
        return true;
    }

    index = (uint16_t)found;
    Lite11CandidateRecord& r = lite11Records[index];

    if (r.observationCount != UINT32_MAX) ++r.observationCount;
    r.lastSeenUptimeMs = now;
    if (input.rssi > r.strongestRssi) r.strongestRssi = input.rssi;
    if (input.rssi < r.weakestRssi) r.weakestRssi = input.rssi;
    r.currentConfidence = input.confidence;
    r.classFlags |= input.classFlags;
    uint8_t previousAlertFlags = r.alertFlags;
    r.alertFlags |= input.alertFlags;
    if (input.falsePositiveSuppressed) r.falsePositiveSuppressed = 1;
    if (r.companyId == LITE11_COMPANY_ID_NONE && input.companyId != LITE11_COMPANY_ID_NONE) {
        r.companyId = input.companyId;
    }
    if (r.relevantUuid16 == 0 && input.relevantUuid16 != 0) {
        r.relevantUuid16 = input.relevantUuid16;
    }

    bool higherConfidence = input.confidence > r.highestConfidence;
    if (higherConfidence) {
        r.highestConfidence = input.confidence;
        r.tier = input.tier;
        r.source = input.source;
        lite11CopyText(r.classification, sizeof(r.classification), input.classification);
        lite11CopyText(r.signature, sizeof(r.signature), input.signature);
    }

    lite11RecordDirty[index] = true;
    if (higherConfidence || input.alertFlags != LITE11_ALERT_NONE || r.alertFlags != previousAlertFlags) {
        lite11RecordUrgent[index] = true;
    }

    portEXIT_CRITICAL(&lite11LogMux);
    (void)isNew;
    return accepted;
}

static inline bool lite11FlushRecordIndex(uint16_t index) {
    if (!lite11FsReady || index >= lite11RecordCountRam) return false;

    Lite11CandidateRecord snap;
    portENTER_CRITICAL(&lite11LogMux);
    snap = lite11Records[index];
    lite11RecordDirty[index] = false;
    lite11RecordUrgent[index] = false;
    portEXIT_CRITICAL(&lite11LogMux);

    bool ok = lite11WriteCandidateSlotNow(index, snap);
    if (!ok) {
        portENTER_CRITICAL(&lite11LogMux);
        lite11RecordDirty[index] = true;
        lite11RecordUrgent[index] = true;
        if (lite11SessionHeader.storageWriteErrors != UINT32_MAX) {
            ++lite11SessionHeader.storageWriteErrors;
        }
        lite11SessionHeaderDirty = true;
        portEXIT_CRITICAL(&lite11LogMux);
        return false;
    }

    lite11RecordLastFlushMs[index] = millis();
    return true;
}

static inline void lite11FlushPending(bool forceAll) {
    if (!lite11FsReady) return;
    uint32_t now = millis();

    for (uint16_t i = 0; i < lite11RecordCountRam; ++i) {
        bool dirty;
        bool urgent;
        uint32_t lastFlush;
        portENTER_CRITICAL(&lite11LogMux);
        dirty = lite11RecordDirty[i];
        urgent = lite11RecordUrgent[i];
        lastFlush = lite11RecordLastFlushMs[i];
        portEXIT_CRITICAL(&lite11LogMux);

        if (!dirty) continue;
        if (!forceAll && !urgent && (uint32_t)(now - lastFlush) < LITE11_RECORD_CHECKPOINT_MS) continue;
        lite11FlushRecordIndex(i);
    }

    bool headerDirty;
    bool headerUrgent;
    portENTER_CRITICAL(&lite11LogMux);
    headerDirty = lite11SessionHeaderDirty;
    headerUrgent = lite11SessionHeaderUrgent;
    portEXIT_CRITICAL(&lite11LogMux);

    if (headerDirty && (forceAll || headerUrgent)) {
        lite11WriteSessionHeaderNow();
    }
}

static inline void lite11PrepareExpectedRestart() {
    // Persist current candidate/header state before the firmware's intentional
    // privacy-recovery restart. This is not called for abrupt power loss/faults.
    lite11FlushPending(true);
    if (lite11PrefsReady) {
        lite11Prefs.putUInt("expSess", lite11SessionIdValue);
        lite11Prefs.putBool("expect", true);
    }
}

static inline bool lite11ReadValidSessionHeader(uint32_t sessionId, Lite11SessionHeader& out) {
    if (!lite11FsReady) return false;
    char path[24];
    lite11SessionPath(sessionId, path, sizeof(path));
    File f = LittleFS.open(path, FILE_READ);
    if (!f || f.size() < sizeof(Lite11SessionHeader)) {
        if (f) f.close();
        return false;
    }
    bool ok = (f.read((uint8_t*)&out, sizeof(out)) == sizeof(out));
    f.close();
    if (!ok) return false;
    if (out.magic != LITE11_SESSION_MAGIC || out.formatVersion != LITE11_LOG_FORMAT_VERSION) return false;
    return out.crc32 == lite11SessionHeaderCrc(out);
}

static inline uint32_t lite11ValidRecordCountForSession(uint32_t sessionId) {
    if (!lite11FsReady) return 0;
    char path[24];
    lite11SessionPath(sessionId, path, sizeof(path));
    File f = LittleFS.open(path, FILE_READ);
    if (!f || f.size() < sizeof(Lite11SessionHeader)) {
        if (f) f.close();
        return 0;
    }
    size_t payload = f.size() - sizeof(Lite11SessionHeader);
    uint32_t slots = (uint32_t)(payload / sizeof(Lite11CandidateRecord));
    uint32_t valid = 0;
    if (!f.seek(sizeof(Lite11SessionHeader), SeekSet)) {
        f.close();
        return 0;
    }
    for (uint32_t i = 0; i < slots; ++i) {
        Lite11CandidateRecord r = {};
        if (f.read((uint8_t*)&r, sizeof(r)) != sizeof(r)) break;
        if (r.magic != LITE11_RECORD_MAGIC || r.formatVersion != LITE11_LOG_FORMAT_VERSION) continue;
        if (r.sessionId != sessionId) continue;
        if (r.crc32 != lite11CandidateCrc(r)) continue;
        ++valid;
    }
    f.close();
    return valid;
}

static inline uint32_t lite11ResetRecordCount() {
    if (!lite11FsReady) return 0;
    File f = LittleFS.open("/resets.bin", FILE_READ);
    if (!f) return 0;
    uint32_t count = (uint32_t)(f.size() / sizeof(Lite11ResetRecord));
    f.close();
    return count;
}

static inline void lite11PrintAlertFlags(uint8_t flags) {
    if (flags == LITE11_ALERT_NONE) {
        Serial.print("NO");
        return;
    }
    bool first = true;
    if (flags & LITE11_ALERT_HIGH_GLASSES) {
        Serial.print("HIGH_GLASSES"); first = false;
    }
    if (flags & LITE11_ALERT_CAMERA_AUDIO) {
        if (!first) Serial.print("+");
        Serial.print("CAMERA_AUDIO"); first = false;
    }
    if (flags & LITE11_ALERT_POSSIBLE_GLASSES) {
        if (!first) Serial.print("+");
        Serial.print("POSSIBLE_GLASSES");
    }
}

static inline void lite11DumpResetHistory() {
    Serial.println("RESET / CRASH HISTORY");
    Serial.println("---------------------");
    if (!lite11FsReady) {
        Serial.println("LittleFS unavailable; reset file cannot be read.");
        return;
    }

    File f = LittleFS.open("/resets.bin", FILE_READ);
    if (!f) {
        Serial.println("No reset history file.");
        return;
    }

    uint32_t index = 0;
    while (f.available() >= (int)sizeof(Lite11ResetRecord)) {
        Lite11ResetRecord r = {};
        if (f.read((uint8_t*)&r, sizeof(r)) != sizeof(r)) break;
        if (r.magic != LITE11_RESET_MAGIC || r.formatVersion != LITE11_LOG_FORMAT_VERSION ||
            r.crc32 != lite11ResetCrc(r)) {
            continue;
        }
        ++index;
        Serial.printf("RESET #%lu | boot=%lu | new_session=%lu | previous_session=%lu | reason=%s | previous_end=%s | fault=%s\n",
                      (unsigned long)index,
                      (unsigned long)r.bootCount,
                      (unsigned long)r.newSessionId,
                      (unsigned long)r.previousSessionId,
                      lite11ResetReasonText(r.resetReason),
                      lite11PreviousEndText(r.previousEndClass),
                      r.faultLike ? "YES" : "NO");
    }
    f.close();
}

static inline void lite11DumpSession(uint32_t sessionId) {
    char path[24];
    lite11SessionPath(sessionId, path, sizeof(path));
    File f = LittleFS.open(path, FILE_READ);
    if (!f) return;

    Lite11SessionHeader h = {};
    if (f.read((uint8_t*)&h, sizeof(h)) != sizeof(h) ||
        h.magic != LITE11_SESSION_MAGIC ||
        h.formatVersion != LITE11_LOG_FORMAT_VERSION ||
        h.crc32 != lite11SessionHeaderCrc(h)) {
        f.close();
        return;
    }

    Serial.println();
    Serial.printf("SESSION %lu | boot=%lu | reset=%s | previous_end=%s | privacy=%s | alerts=%lu | dropped=%lu | write_errors=%lu\n",
                  (unsigned long)h.sessionId,
                  (unsigned long)h.bootCount,
                  lite11ResetReasonText(h.resetReason),
                  lite11PreviousEndText(h.previousEndClass),
                  lite11PrivacyModeText(h.privacyMode),
                  (unsigned long)h.alertEpisodes,
                  (unsigned long)h.droppedCandidates,
                  (unsigned long)h.storageWriteErrors);

    uint32_t slot = 0;
    while (f.available() >= (int)sizeof(Lite11CandidateRecord)) {
        Lite11CandidateRecord r = {};
        if (f.read((uint8_t*)&r, sizeof(r)) != sizeof(r)) break;
        ++slot;
        if (r.magic != LITE11_RECORD_MAGIC || r.formatVersion != LITE11_LOG_FORMAT_VERSION ||
            r.sessionId != h.sessionId || r.crc32 != lite11CandidateCrc(r)) {
            Serial.printf("  RECORD %lu | INVALID/INCOMPLETE\n", (unsigned long)slot);
            continue;
        }

        char hashText[32];
        snprintf(hashText, sizeof(hashText), "MAC-HASH-%016llX", (unsigned long long)r.deviceHash);
        Serial.printf("  RECORD %lu | %s | confidence=%u | highest=%u | observations=%lu | first_ms=%lu | last_ms=%lu | RSSI=%d..%d | ",
                      (unsigned long)slot,
                      hashText,
                      (unsigned)r.currentConfidence,
                      (unsigned)r.highestConfidence,
                      (unsigned long)r.observationCount,
                      (unsigned long)r.firstSeenUptimeMs,
                      (unsigned long)r.lastSeenUptimeMs,
                      (int)r.weakestRssi,
                      (int)r.strongestRssi);

        if (r.companyId == LITE11_COMPANY_ID_NONE) Serial.print("company=- | ");
        else Serial.printf("company=0x%04X | ", (unsigned)r.companyId);

        if (r.relevantUuid16 == 0) Serial.print("uuid=- | ");
        else Serial.printf("uuid=0x%04X | ", (unsigned)r.relevantUuid16);

        Serial.printf("classification=%s | signature=%s | alert=", r.classification, r.signature);
        lite11PrintAlertFlags(r.alertFlags);
        Serial.printf(" | known_device=%s | false_positive_suppressed=%s\n",
                      (r.classFlags & LITE11_CLASS_KNOWN_DEVICE) ? "YES" : "NO",
                      r.falsePositiveSuppressed ? "YES" : "NO");
    }
    f.close();
}

static inline void lite11DumpAll(const char* currentModeText) {
    // Make the current RAM snapshot as durable as practical before retrieval.
    lite11FlushPending(true);

    uint32_t privateOk = lite11PrefsReady ? lite11Prefs.getUInt("privOK", 0) : 0;
    uint32_t privateFail = lite11PrefsReady ? lite11Prefs.getUInt("privFail", 0) : 0;
    uint32_t publicFallback = lite11PrefsReady ? lite11Prefs.getUInt("public", 0) : 0;

    uint32_t retainedSessions = 0;
    uint32_t totalEntries = 0;
    uint32_t totalAlerts = 0;

    if (lite11FsReady) {
        for (uint32_t sid = 1; sid <= lite11SessionIdValue; ++sid) {
            Lite11SessionHeader h = {};
            if (!lite11ReadValidSessionHeader(sid, h)) continue;
            ++retainedSessions;
            totalEntries += lite11ValidRecordCountForSession(sid);
            totalAlerts += h.alertEpisodes;
        }
    }

    Serial.println();
    Serial.println("============================================================");
    Serial.println("LOG DUMP");
    Serial.println("============================================================");
    Serial.printf("FIRMWARE: %s\n", LITE11_FIRMWARE_NAME);
    Serial.printf("CURRENT_SESSION_ID: %lu\n", (unsigned long)lite11SessionIdValue);
    Serial.printf("TOTAL_PERSISTENT_SESSIONS: %lu\n", (unsigned long)lite11SessionIdValue);
    Serial.printf("RETAINED_SESSION_FILES: %lu\n", (unsigned long)retainedSessions);
    Serial.printf("LIFETIME_BOOT_COUNT: %lu\n", (unsigned long)lite11BootCountValue);
    Serial.printf("CURRENT_PRIVACY_MODE: %s\n", currentModeText ? currentModeText : "UNKNOWN");
    Serial.printf("PRIVATE_START_SUCCESSES: %lu\n", (unsigned long)privateOk);
    Serial.printf("PRIVATE_START_FAILURES: %lu\n", (unsigned long)privateFail);
    Serial.printf("PUBLIC_FALLBACK_STARTS: %lu\n", (unsigned long)publicFallback);
    Serial.printf("TOTAL_STORED_GE40_ENTRIES: %lu\n", (unsigned long)totalEntries);
    Serial.printf("TOTAL_ALERT_EPISODES: %lu\n", (unsigned long)totalAlerts);
    Serial.printf("TOTAL_RESET_CRASH_RECORDS: %lu\n", (unsigned long)lite11ResetRecordCount());
    Serial.printf("LITTLEFS_STATUS: %s\n", lite11FsReady ? "READY" : "UNAVAILABLE");
    if (lite11FsReady) {
        Serial.printf("LITTLEFS_USED_BYTES: %lu\n", (unsigned long)LittleFS.usedBytes());
        Serial.printf("LITTLEFS_TOTAL_BYTES: %lu\n", (unsigned long)LittleFS.totalBytes());
        Serial.printf("EIGHT_DAY_CAPACITY_TARGET: %s\n",
                      LittleFS.totalBytes() >= LITE11_MIN_TARGET_FS_BYTES ? "PARTITION_SIZE_OK" : "PARTITION_BELOW_RECOMMENDED_MINIMUM");
    }
    Serial.println();

    lite11DumpResetHistory();
    Serial.println();
    Serial.println("SESSION-BY-SESSION >=40% DEDUPLICATED HISTORY");
    Serial.println("---------------------------------------------");

    if (lite11FsReady) {
        for (uint32_t sid = 1; sid <= lite11SessionIdValue; ++sid) {
            lite11DumpSession(sid);
        }
    }

    Serial.println();
    Serial.println("END LOG DUMP");
    Serial.println("============================================================");
    Serial.println();
}
