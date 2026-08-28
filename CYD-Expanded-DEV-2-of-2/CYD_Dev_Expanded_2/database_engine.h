#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <string.h>
#include "ble_db_protocol.h"

#define TIER_HIGH   0
#define TIER_MEDIUM 1
#define TIER_LOW    2

#include "high_confidence_matches.h"
#include "medium_confidence_matches.h"
#include "low_confidence_matches.h"
#include "known_devices.h"
#include "false_positives.h"
#include "mics_and_cams.h"
#include "ble_reference_data.h"
#include "company_ids_generated.h"

inline void dbSafeCopy(char* dst, size_t dstLen, const char* src) {
    if (dst == nullptr || dstLen == 0) return;
    if (src == nullptr) {
        dst[0] = '\0';
        return;
    }
    strlcpy(dst, src, dstLen);
}

inline bool dbEqualsIgnoreCase(const char* a, const char* b) {
    if (a == nullptr || b == nullptr) return false;
    while (*a && *b) {
        char ca = (char)tolower((unsigned char)*a++);
        char cb = (char)tolower((unsigned char)*b++);
        if (ca != cb) return false;
    }
    return *a == '\0' && *b == '\0';
}

inline bool dbStartsWithIgnoreCase(const char* text, const char* prefix) {
    if (text == nullptr || prefix == nullptr) return false;
    while (*prefix) {
        if (!*text) return false;
        if (tolower((unsigned char)*text++) != tolower((unsigned char)*prefix++)) return false;
    }
    return true;
}

inline bool dbContainsIgnoreCase(const char* haystack, const char* needle) {
    if (haystack == nullptr || needle == nullptr || *needle == '\0') return false;
    size_t needleLen = strlen(needle);
    for (const char* p = haystack; *p; ++p) {
        size_t i = 0;
        while (i < needleLen && p[i] &&
               tolower((unsigned char)p[i]) == tolower((unsigned char)needle[i])) {
            ++i;
        }
        if (i == needleLen) return true;
    }
    return false;
}

inline bool dbServiceUuid16Present(const char* serviceUuids, uint16_t uuid16) {
    if (uuid16 == 0 || serviceUuids == nullptr || *serviceUuids == '\0') return false;
    char needle[5];
    snprintf(needle, sizeof(needle), "%04X", uuid16);
    return dbContainsIgnoreCase(serviceUuids, needle);
}

inline uint32_t dbOuiFromMac(const char* mac) {
    if (mac == nullptr) return 0;
    unsigned int a = 0, b = 0, c = 0;
    if (sscanf(mac, "%02x:%02x:%02x", &a, &b, &c) != 3) return 0;
    return ((uint32_t)(a & 0xFFU) << 16) |
           ((uint32_t)(b & 0xFFU) << 8) |
           (uint32_t)(c & 0xFFU);
}

inline const char* dbCompanyName(uint16_t companyId) {
    if (companyId == 0xFFFF) return nullptr;
    for (uint16_t i = 0; GENERATED_COMPANY_IDS[i].companyName != nullptr; ++i) {
        if (GENERATED_COMPANY_IDS[i].companyId == companyId) {
            return GENERATED_COMPANY_IDS[i].companyName;
        }
    }
    for (uint16_t i = 0; BLE_COMPANY_REFERENCES[i].companyName != nullptr; ++i) {
        if (BLE_COMPANY_REFERENCES[i].companyId == companyId) {
            return BLE_COMPANY_REFERENCES[i].companyName;
        }
    }
    return nullptr;
}

inline bool dbGlassesRuleMatches(const GlassesConfidenceRule& rule,
                                 const BleDbLookupPayload& q) {
    bool criterion = false;

    if (rule.companyId >= 0) {
        criterion = true;
        if (q.companyId == 0xFFFF || q.companyId != (uint16_t)rule.companyId) return false;
    }

    if (rule.macExact != nullptr) {
        criterion = true;
        if (!dbEqualsIgnoreCase(q.mac, rule.macExact)) return false;
    }

    if (rule.manufacturerHex != nullptr) {
        criterion = true;
        if (!dbContainsIgnoreCase(q.manufacturerHex, rule.manufacturerHex)) return false;
    }

    if (rule.serviceUuid16 != 0) {
        criterion = true;
        if (!dbServiceUuid16Present(q.serviceUuids, rule.serviceUuid16)) return false;
    }

    if (rule.namePattern != nullptr) {
        criterion = true;
        if (q.name[0] == '\0') return false;
        bool nameMatch = rule.exactName
            ? dbEqualsIgnoreCase(q.name, rule.namePattern)
            : dbContainsIgnoreCase(q.name, rule.namePattern);
        if (!nameMatch) return false;
    }

    if (rule.oui24 != 0) {
        criterion = true;
        if (dbOuiFromMac(q.mac) != rule.oui24) return false;
    }

    return criterion;
}

inline bool dbFindBestGlasses(const BleDbLookupPayload& q,
                              const GlassesConfidenceRule*& bestRule,
                              uint8_t& bestTier) {
    bestRule = nullptr;
    bestTier = TIER_LOW;

    struct RuleSet { const GlassesConfidenceRule* rules; uint8_t tier; };
    const RuleSet sets[] = {
        { HIGH_CONFIDENCE_RULES, TIER_HIGH },
        { MEDIUM_CONFIDENCE_RULES, TIER_MEDIUM },
        { LOW_CONFIDENCE_RULES, TIER_LOW }
    };

    for (const RuleSet& set : sets) {
        for (uint16_t i = 0; set.rules[i].company != nullptr; ++i) {
            const GlassesConfidenceRule& rule = set.rules[i];
            if (!dbGlassesRuleMatches(rule, q)) continue;
            if (bestRule != nullptr && rule.confidence <= bestRule->confidence) continue;
            bestRule = &rule;
            bestTier = set.tier;
        }
    }
    return bestRule != nullptr;
}

inline bool dbMicCamRuleMatches(const MicCamRule& rule,
                                const BleDbLookupPayload& q) {
    bool criterion = false;

    if (rule.namePattern != nullptr) {
        criterion = true;
        bool ok = false;
        if (rule.nameMode == MICCAM_EXACT) ok = dbEqualsIgnoreCase(q.name, rule.namePattern);
        else if (rule.nameMode == MICCAM_PREFIX) ok = dbStartsWithIgnoreCase(q.name, rule.namePattern);
        else ok = dbContainsIgnoreCase(q.name, rule.namePattern);
        if (!ok) return false;
    }

    if (rule.companyId >= 0) {
        criterion = true;
        if (q.companyId == 0xFFFF || q.companyId != (uint16_t)rule.companyId) return false;
    }

    if (rule.manufacturerHex != nullptr) {
        criterion = true;
        if (!dbContainsIgnoreCase(q.manufacturerHex, rule.manufacturerHex)) return false;
    }

    if (rule.serviceUuid16 != 0) {
        criterion = true;
        if (!dbServiceUuid16Present(q.serviceUuids, rule.serviceUuid16)) return false;
    }

    if (rule.oui24 != 0) {
        criterion = true;
        if (dbOuiFromMac(q.mac) != rule.oui24) return false;
    }

    return criterion;
}

inline const char* dbMicCamCategoryText(MicCamCategory c) {
    switch (c) {
        case MICCAM_CATEGORY_CAMERA: return "Camera";
        case MICCAM_CATEGORY_MICROPHONE: return "Microphone";
        case MICCAM_CATEGORY_CAMERA_AUDIO: return "Camera/Audio";
        case MICCAM_CATEGORY_RECORDER: return "Recorder";
        default: return "Camera/Audio";
    }
}

inline bool dbKnownPayloadMatch(const BleDbLookupPayload& q,
                                const KnownManufacturerPayloadRule*& outRule) {
    outRule = nullptr;
    for (uint16_t i = 0; KNOWN_MANUFACTURER_PAYLOAD_RULES[i].manufacturer != nullptr; ++i) {
        const KnownManufacturerPayloadRule& r = KNOWN_MANUFACTURER_PAYLOAD_RULES[i];
        if (q.companyId != r.companyId) continue;
        if (r.manufacturerHexPattern != nullptr &&
            !dbContainsIgnoreCase(q.manufacturerHex, r.manufacturerHexPattern)) continue;
        outRule = &r;
        return true;
    }
    return false;
}

inline bool dbKnownDeviceMatch(const BleDbLookupPayload& q,
                               const KnownDeviceRule*& outRule) {
    outRule = nullptr;
    for (uint16_t i = 0; KNOWN_DEVICES[i].manufacturer != nullptr; ++i) {
        const KnownDeviceRule& r = KNOWN_DEVICES[i];
        if (r.companyId != 0xFFFF && q.companyId != r.companyId) continue;

        if (r.matchMode == KNOWN_COMPANY_ID_ONLY) {
            outRule = &r;
            return true;
        }
        if (r.advertisedPattern == nullptr || q.name[0] == '\0') continue;

        bool ok = false;
        if (r.matchMode == KNOWN_EXACT) ok = dbEqualsIgnoreCase(q.name, r.advertisedPattern);
        else if (r.matchMode == KNOWN_PREFIX) ok = dbStartsWithIgnoreCase(q.name, r.advertisedPattern);
        else ok = dbContainsIgnoreCase(q.name, r.advertisedPattern);

        if (ok) {
            outRule = &r;
            return true;
        }
    }
    return false;
}

inline bool dbFalsePositiveMatch(const BleDbLookupPayload& q,
                                 const FalsePositiveRule*& outRule) {
    outRule = nullptr;
    if (q.name[0] == '\0') return false;
    for (uint16_t i = 0; FALSE_POSITIVE_RULES[i].namePattern != nullptr; ++i) {
        const FalsePositiveRule& r = FALSE_POSITIVE_RULES[i];
        if (dbContainsIgnoreCase(q.name, r.namePattern)) {
            outRule = &r;
            return true;
        }
    }
    return false;
}

inline void dbEvaluateLookup(const BleDbLookupPayload& q, BleDbResultPayload& result) {
    memset(&result, 0, sizeof(result));
    result.companyId = q.companyId;

    // Priority mirrors the project's safety intent: specific smart-glasses rules
    // first, then camera/audio, explicit known devices, false-positive annotations,
    // then generic Company-ID reference information.
    const GlassesConfidenceRule* glasses = nullptr;
    uint8_t glassesTier = TIER_LOW;
    if (dbFindBestGlasses(q, glasses, glassesTier)) {
        result.matched = 1;
        result.classification = BLE_DB_CLASS_SMART_GLASSES;
        result.evidence = BLE_DB_EVIDENCE_STRONG;
        result.tier = glassesTier;
        result.confidenceSuggestion = glasses->confidence;
        result.hasCamera = glasses->hasCamera ? 1 : 0;
        result.cameraKnown = glasses->cameraKnown ? 1 : 0;
        dbSafeCopy(result.vendor, sizeof(result.vendor), glasses->company);
        dbSafeCopy(result.product, sizeof(result.product), glasses->product);
        dbSafeCopy(result.deviceType, sizeof(result.deviceType), "Smart Glasses");
        dbSafeCopy(result.reason, sizeof(result.reason), glasses->reason);
        return;
    }

    for (uint16_t i = 0; MIC_CAM_RULES[i].deviceLabel != nullptr; ++i) {
        const MicCamRule& r = MIC_CAM_RULES[i];
        if (!dbMicCamRuleMatches(r, q)) continue;
        result.matched = 1;
        result.classification = BLE_DB_CLASS_CAMERA_AUDIO;
        result.evidence = BLE_DB_EVIDENCE_STRONG;
        result.confidenceSuggestion = r.confidence;
        result.hasCamera = (r.category == MICCAM_CATEGORY_CAMERA ||
                            r.category == MICCAM_CATEGORY_CAMERA_AUDIO) ? 1 : 0;
        result.cameraKnown = result.hasCamera;
        const char* company = dbCompanyName(q.companyId);
        dbSafeCopy(result.vendor, sizeof(result.vendor), company ? company : "");
        dbSafeCopy(result.product, sizeof(result.product), r.deviceLabel);
        dbSafeCopy(result.deviceType, sizeof(result.deviceType), dbMicCamCategoryText(r.category));
        dbSafeCopy(result.reason, sizeof(result.reason), r.reason);
        return;
    }

    const KnownManufacturerPayloadRule* payloadRule = nullptr;
    if (dbKnownPayloadMatch(q, payloadRule)) {
        result.matched = 1;
        result.classification = BLE_DB_CLASS_KNOWN_DEVICE;
        result.evidence = BLE_DB_EVIDENCE_KNOWN;
        dbSafeCopy(result.vendor, sizeof(result.vendor), payloadRule->manufacturer);
        dbSafeCopy(result.product, sizeof(result.product), payloadRule->assumedDeviceId);
        dbSafeCopy(result.deviceType, sizeof(result.deviceType), payloadRule->deviceType);
        dbSafeCopy(result.reason, sizeof(result.reason), "Known manufacturer-payload reference match");
        return;
    }

    const KnownDeviceRule* knownRule = nullptr;
    if (dbKnownDeviceMatch(q, knownRule)) {
        result.matched = 1;
        result.classification = BLE_DB_CLASS_KNOWN_DEVICE;
        result.evidence = BLE_DB_EVIDENCE_KNOWN;
        dbSafeCopy(result.vendor, sizeof(result.vendor), knownRule->manufacturer);
        dbSafeCopy(result.product, sizeof(result.product), knownRule->assumedDeviceId);
        dbSafeCopy(result.deviceType, sizeof(result.deviceType), knownRule->deviceType);
        dbSafeCopy(result.reason, sizeof(result.reason), "Known-device reference match");
        return;
    }

    const FalsePositiveRule* fp = nullptr;
    if (dbFalsePositiveMatch(q, fp)) {
        result.matched = 1;
        result.classification = BLE_DB_CLASS_FALSE_POSITIVE;
        result.evidence = BLE_DB_EVIDENCE_KNOWN;
        dbSafeCopy(result.product, sizeof(result.product), fp->assumedDevice);
        dbSafeCopy(result.deviceType, sizeof(result.deviceType), fp->assumedType);
        dbSafeCopy(result.reason, sizeof(result.reason), fp->reason);
        return;
    }

    const char* company = dbCompanyName(q.companyId);
    if (company != nullptr) {
        result.matched = 1;
        result.classification = BLE_DB_CLASS_REFERENCE;
        result.evidence = BLE_DB_EVIDENCE_REFERENCE;
        dbSafeCopy(result.vendor, sizeof(result.vendor), company);
        dbSafeCopy(result.product, sizeof(result.product), "Company-ID reference only");
        dbSafeCopy(result.deviceType, sizeof(result.deviceType), "Reference");
        dbSafeCopy(result.reason, sizeof(result.reason), "Bluetooth Company Identifier reference; not a product identification");
        return;
    }

    result.matched = 0;
    result.classification = BLE_DB_CLASS_UNKNOWN;
    result.evidence = BLE_DB_EVIDENCE_NONE;
    dbSafeCopy(result.product, sizeof(result.product), "Unknown BLE device");
    dbSafeCopy(result.deviceType, sizeof(result.deviceType), "Unknown");
    dbSafeCopy(result.reason, sizeof(result.reason), "No external database rule matched");
}

inline uint16_t dbGlassesRuleCount() {
    return (uint16_t)(
        (sizeof(HIGH_CONFIDENCE_RULES) / sizeof(HIGH_CONFIDENCE_RULES[0]) - 1U) +
        (sizeof(MEDIUM_CONFIDENCE_RULES) / sizeof(MEDIUM_CONFIDENCE_RULES[0]) - 1U) +
        (sizeof(LOW_CONFIDENCE_RULES) / sizeof(LOW_CONFIDENCE_RULES[0]) - 1U)
    );
}

inline uint16_t dbKnownRuleCount() {
    return (uint16_t)(sizeof(KNOWN_DEVICES) / sizeof(KNOWN_DEVICES[0]) - 1U);
}

inline uint16_t dbMicCamRuleCount() {
    return (uint16_t)(sizeof(MIC_CAM_RULES) / sizeof(MIC_CAM_RULES[0]) - 1U);
}
