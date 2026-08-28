#pragma once
#include <stdint.h>


/*
 * ========================= USER-EDITABLE TABLE =========================
 * Edit ONLY the rows inside HIGH_CONFIDENCE_RULES[] unless you are changing
 * the rule engine itself. Keep the final nullptr sentinel as the LAST row.
 *
 * Field order:
 * company, product, confidence, companyId, macExact, manufacturerHex,
 * serviceUuid16, namePattern, exactName, oui24, hasCamera, cameraKnown, reason
 *
 * Use -1 / nullptr / 0 for criteria that are not required. Every populated
 * criterion in a row must match the SAME BLE advertisement.
 *
 * v6.1 alert rule: confidence 90..96 = orange POSSIBLE; 97..100 = red HIGH.
 * =======================================================================
 */
#ifndef GLASSES_CONFIDENCE_RULE_TYPE_DEFINED
#define GLASSES_CONFIDENCE_RULE_TYPE_DEFINED
struct GlassesConfidenceRule {
    const char* company;
    const char* product;
    uint8_t     confidence;          // 0..100
    int32_t     companyId;           // -1 = not required
    const char* macExact;            // nullptr = not required
    const char* manufacturerHex;     // nullptr = not required; case-insensitive hex substring
    uint16_t    serviceUuid16;       // 0 = not required
    const char* namePattern;         // nullptr = not required
    bool        exactName;           // only used when namePattern != nullptr
    uint32_t    oui24;               // 0 = not required, e.g. 0x7C2A9E
    bool        hasCamera;
    bool        cameraKnown;
    const char* reason;
};
#endif

/*
 * HIGH RULE TIER — 90..100 confidence.
 * IMPORTANT v6.1 alert rule:
 *   - A rule score below 97 NEVER produces the red HIGH alert.
 *   - 90..96 is displayed as an orange POSSIBLE alert.
 *   - Confidence 97..100 produces red HIGH tiles.
 *
 * User-selected scoring:
 *   exact confirmed MAC          = 90
 *   confirmed mfg fingerprint    = 90
 *   strong multi-signal rule     = 90
 *
 * There are intentionally no automatic 100-point rules in this initial v6.1
 * database. The red 97..100 path is implemented and ready for future rules.
 *
 * All populated criteria in a rule must match the same advertisement.
 */
static const GlassesConfidenceRule HIGH_CONFIDENCE_RULES[] = {
    // Exact confirmed instance — useful, but BLE MACs can vary/rotate.
    { "Google", "Google Glass", 90,
      -1, "40:a3:b2:6c:24:46", nullptr, 0, nullptr, false, 0,
      true, true, "Exact confirmed Google Glass MAC" },

    // Confirmed manufacturer fingerprints.
    { "Google", "Google Glass", 90,
      0x00E0, nullptr, "E0000090CA64E064", 0, nullptr, false, 0,
      true, true, "Confirmed Google Glass manufacturer fingerprint" },

    { "Meta Platforms Technologies", "Meta Ray-Ban", 90,
      0x058E, nullptr, "4D45544152425F474C415353", 0, nullptr, false, 0,
      true, true, "Confirmed META_RB_GLASS manufacturer fingerprint" },

    // Strong multi-signal rules. All remain 90 per the v6.1 scoring policy.
    { "Luxottica Group", "Ray-Ban Meta", 90,
      0x0D53, nullptr, nullptr, 0, "ray-ban", false, 0,
      true, true, "Luxottica Company ID + Ray-Ban name" },

    { "Luxottica Group", "Ray-Ban Meta", 90,
      0x0D53, nullptr, nullptr, 0, "ray ban", false, 0,
      true, true, "Luxottica Company ID + Ray Ban name" },

    { "Luxottica Group", "Oakley Meta", 90,
      0x0D53, nullptr, nullptr, 0, "oakley", false, 0,
      true, true, "Luxottica Company ID + Oakley name" },

    { "Meta Platforms", "Meta smart-glasses candidate", 90,
      0x01AB, nullptr, nullptr, 0xFD5F, nullptr, false, 0,
      true, true, "Meta Company ID + Meta service UUID" },

    { "Sony Corporation", "Sony SmartEyeglass", 90,
      0x012D, nullptr, nullptr, 0, "SmartEyeglass", false, 0,
      true, true, "Sony Company ID + SmartEyeglass name" },

    { "Seiko Epson", "Epson Moverio", 90,
      0x0040, nullptr, nullptr, 0, "Moverio", false, 0,
      true, true, "Epson Company ID + Moverio name" },

    { "Snapchat Inc", "Snap Spectacles", 90,
      0x03C2, nullptr, nullptr, 0, "Spectacles", false, 0,
      true, true, "Snap Company ID + Spectacles name" },

    { "Vuzix Corporation", "Vuzix Smart Glasses", 90,
      0x060C, nullptr, nullptr, 0, "Vuzix", false, 0,
      true, true, "Vuzix Company ID + Vuzix name" },

    { nullptr, nullptr, 0, -1, nullptr, nullptr, 0, nullptr, false, 0, false, false, nullptr }
};
