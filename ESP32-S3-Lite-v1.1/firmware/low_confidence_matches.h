#pragma once
#include <stdint.h>


/*
 * ========================= USER-EDITABLE TABLE =========================
 * LOW is normally used for Company-ID-alone manufacturer clues. It may also
 * hold a deliberately downgraded non-alert service clue such as Google FE9F.
 * Edit the rows in LOW_CONFIDENCE_RULES[] and keep the final nullptr sentinel
 * LAST. Scores in
 * this file must remain 1..59. LOW never creates a TFT/LED alert by itself.
 *
 * Field order is the same GlassesConfidenceRule structure used by the other
 * confidence headers. For LOW, normally only company/companyId/confidence and
 * descriptive fields should be populated.
 * =======================================================================
 */
#ifndef GLASSES_CONFIDENCE_RULE_TYPE_DEFINED
#define GLASSES_CONFIDENCE_RULE_TYPE_DEFINED
struct GlassesConfidenceRule {
    const char* company;
    const char* product;
    uint8_t     confidence;
    int32_t     companyId;
    const char* macExact;
    const char* manufacturerHex;
    uint16_t    serviceUuid16;
    const char* namePattern;
    bool        exactName;
    uint32_t    oui24;
    bool        hasCamera;
    bool        cameraKnown;
    const char* reason;
};
#endif

/*
 * LOW RULE TIER — 1..59 confidence.
 * v6.1 policy: LOW is a non-alert BLE clue. Most rows are Company ID alone;
 * FE9F is intentionally retained here only as a generic Google BLE clue.
 * LOW rules are logged/annotated only. They do NOT light orange/red and do
 * NOT enter detections.csv by themselves.
 *
 * The v3.4 Company-ID coverage is retained here so the baseline database is
 * not lost; only its alert semantics change.
 */
static const GlassesConfidenceRule LOW_CONFIDENCE_RULES[] = {
    { "Meta Platforms",              "Meta device",                50, 0x01AB, nullptr, nullptr, 0, nullptr, false, 0, true,  false, "Company ID alone - possible manufacturer match" },
    { "Meta Platforms Technologies", "Meta technology device",     50, 0x058E, nullptr, nullptr, 0, nullptr, false, 0, true,  false, "Company ID alone - possible manufacturer match" },
    { "Luxottica Group",             "Luxottica device",           55, 0x0D53, nullptr, nullptr, 0, nullptr, false, 0, true,  false, "Company ID alone - possible manufacturer match" },
    { "Snapchat Inc",                "Snap device",                50, 0x03C2, nullptr, nullptr, 0, nullptr, false, 0, true,  false, "Company ID alone - possible manufacturer match" },
    { "Vuzix Corporation",           "Vuzix device",               55, 0x060C, nullptr, nullptr, 0, nullptr, false, 0, true,  false, "Company ID alone - possible manufacturer match" },
    { "Google",                      "Google device",              45, 0x00E0, nullptr, nullptr, 0, nullptr, false, 0, true,  false, "Company ID alone - possible manufacturer match" },
    { "Google LLC",                  "Google device",              45, 0x018E, nullptr, nullptr, 0, nullptr, false, 0, true,  false, "Company ID alone - possible manufacturer match" },
    { "Google",                      "Google BLE device",          45, -1,     nullptr, nullptr, 0xFE9F, nullptr, false, 0, false, false, "Google FE9F service UUID - generic Google BLE clue only" },
    { "TCL Communication",           "TCL device",                 40, 0x0BC6, nullptr, nullptr, 0, nullptr, false, 0, true,  false, "Company ID alone - possible manufacturer match" },
    { "Meizu Technology",            "Meizu device",               40, 0x03AB, nullptr, nullptr, 0, nullptr, false, 0, true,  false, "Company ID alone - possible manufacturer match" },
    { "Thalmic Labs (North)",        "North device",               45, 0x0562, nullptr, nullptr, 0, nullptr, false, 0, false, false, "Company ID alone - possible manufacturer match" },
    { "Kopin Corporation",           "Kopin device",               45, 0x041F, nullptr, nullptr, 0, nullptr, false, 0, false, false, "Company ID alone - possible manufacturer match" },
    { "Sony Corporation",            "Sony device",                30, 0x012D, nullptr, nullptr, 0, nullptr, false, 0, true,  false, "Company ID alone - possible manufacturer match" },
    { "Seiko Epson",                 "Epson device",               30, 0x0040, nullptr, nullptr, 0, nullptr, false, 0, true,  false, "Company ID alone - possible manufacturer match" },
    { "Apple",                       "Apple device",               25, 0x004C, nullptr, nullptr, 0, nullptr, false, 0, false, false, "Company ID alone - possible manufacturer match" },
    { "Xiaomi",                      "Xiaomi device",              25, 0x038F, nullptr, nullptr, 0, nullptr, false, 0, true,  false, "Company ID alone - possible manufacturer match" },
    { "Oppo",                        "Oppo device",                25, 0x079A, nullptr, nullptr, 0, nullptr, false, 0, true,  false, "Company ID alone - possible manufacturer match" },
    { "Huawei Technologies",         "Huawei device",              25, 0x027D, nullptr, nullptr, 0, nullptr, false, 0, true,  false, "Company ID alone - possible manufacturer match" },
    { "Samsung Electronics",         "Samsung device",             25, 0x0075, nullptr, nullptr, 0, nullptr, false, 0, false, false, "Company ID alone - possible manufacturer match" },
    { "Amazon",                      "Amazon device",              25, 0x0171, nullptr, nullptr, 0, nullptr, false, 0, true,  false, "Company ID alone - possible manufacturer match" },
    { "Bose Corporation",            "Bose device",                25, 0x009E, nullptr, nullptr, 0, nullptr, false, 0, false, false, "Company ID alone - possible manufacturer match" },
    { "Lenovo",                      "Lenovo device",              25, 0x02C5, nullptr, nullptr, 0, nullptr, false, 0, true,  false, "Company ID alone - possible manufacturer match" },
    { "HTC Corporation",             "HTC device",                 25, 0x02ED, nullptr, nullptr, 0, nullptr, false, 0, true,  false, "Company ID alone - possible manufacturer match" },
    { "ByteDance",                   "ByteDance device",           25, 0x0B24, nullptr, nullptr, 0, nullptr, false, 0, true,  false, "Company ID alone - possible manufacturer match" },
    { "Razer Inc",                   "Razer device",               25, 0x068E, nullptr, nullptr, 0, nullptr, false, 0, false, false, "Company ID alone - possible manufacturer match" },
    { "Fauna Audio",                 "Fauna device",               25, 0x0976, nullptr, nullptr, 0, nullptr, false, 0, false, false, "Company ID alone - possible manufacturer match" },

    { nullptr, nullptr, 0, -1, nullptr, nullptr, 0, nullptr, false, 0, false, false, nullptr }
};
