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


    // ---------------------------------------------------------------------
    // v6.3 DEV — user-requested smart-glasses model expansion.
    // These are HIGH-TIER advertised-name rules at confidence 90. Under the
    // existing v6.3 UI policy, 90 remains orange POSSIBLE; only 97..100 is
    // red HIGH. Short/ambiguous model names use exact-name matching.
    // No Company ID, OUI or manufacturer fingerprint is inferred here.
    // ---------------------------------------------------------------------
    { "Samsung / Gentle Monster / Google", "Samsung x Gentle Monster x Google", 90, -1, nullptr, nullptr, 0, "Gentle Monster", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Meta", "Meta Glasses Adventurer", 90, -1, nullptr, nullptr, 0, "Adventurer", true, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Meta", "Meta Glasses Fury", 90, -1, nullptr, nullptr, 0, "Fury", true, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Meta", "Meta Starfire Kylie Edition", 90, -1, nullptr, nullptr, 0, "Starfire Kylie", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "VITURE", "VITURE Helix", 90, -1, nullptr, nullptr, 0, "Helix", true, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Acer", "Acer GI0", 90, -1, nullptr, nullptr, 0, "Acer GI0", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Willit", "Willit AI Glasses", 90, -1, nullptr, nullptr, 0, "Willit AI Glasses", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "RayNeo", "RayNeo V4", 90, -1, nullptr, nullptr, 0, "RayNeo V4", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Samsung / Warby Parker / Google", "Samsung x Warby Parker x Google", 90, -1, nullptr, nullptr, 0, "Warby Parker", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Lenskart", "B by Lenskart", 90, -1, nullptr, nullptr, 0, "B by Lenskart", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Moonix", "Moonix Pro", 90, -1, nullptr, nullptr, 0, "Moonix Pro", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Huawei", "Huawei AI Glasses", 90, -1, nullptr, nullptr, 0, "Huawei AI Glasses", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Ray-Ban / Meta", "Ray-Ban Meta Optics Blayzer", 90, -1, nullptr, nullptr, 0, "Blayzer", true, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Ray-Ban / Meta", "Ray-Ban Meta Optics Scriber", 90, -1, nullptr, nullptr, 0, "Scriber", true, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Rokid", "Rokid AI Glasses Style", 90, -1, nullptr, nullptr, 0, "Rokid AI Glasses Style", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Alibaba / Quark", "Alibaba Quark AI Glasses G1", 90, -1, nullptr, nullptr, 0, "Quark AI Glasses G1", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "DPVR", "DPVR G5", 90, -1, nullptr, nullptr, 0, "DPVR G5", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "DPVR", "DPVR G1", 90, -1, nullptr, nullptr, 0, "DPVR G1", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Rokid / Bolon", "Rokid x Bolon AI Glasses", 90, -1, nullptr, nullptr, 0, "Bolon AI Glasses", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Ray-Ban / Meta", "Ray-Ban Meta Gen 2", 90, -1, nullptr, nullptr, 0, "Ray-Ban Meta", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Oakley / Meta", "Oakley Meta Vanguard", 90, -1, nullptr, nullptr, 0, "Oakley Meta Vanguard", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Jio", "JioFrames", 90, -1, nullptr, nullptr, 0, "JioFrames", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "HTC", "HTC VIVE Eagle", 90, -1, nullptr, nullptr, 0, "VIVE Eagle", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Mentra", "Mentra Live", 90, -1, nullptr, nullptr, 0, "Mentra Live", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Xiaomi", "Xiaomi AI Glasses", 90, -1, nullptr, nullptr, 0, "Xiaomi AI Glasses", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Oakley / Meta", "Oakley Meta HSTN", 90, -1, nullptr, nullptr, 0, "Oakley Meta HSTN", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Lucyd / Reebok", "Reebok Glasses by Lucyd", 90, -1, nullptr, nullptr, 0, "Reebok Glasses", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Xiaomi", "Xiaomi Mijia 2", 90, -1, nullptr, nullptr, 0, "Xiaomi Mijia 2", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "BleeqUp", "BleeqUp Ranger", 90, -1, nullptr, nullptr, 0, "BleeqUp Ranger", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Nuance Audio", "Nuance Audio Hearing Glasses", 90, -1, nullptr, nullptr, 0, "Nuance Audio", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Solos", "Solos AirGo 3 Vision", 90, -1, nullptr, nullptr, 0, "AirGo 3 Vision", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "RayNeo", "RayNeo iO", 90, -1, nullptr, nullptr, 0, "RayNeo iO", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Halliday", "Halliday G2", 90, -1, nullptr, nullptr, 0, "Halliday G2", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Raven", "Raven Prism", 90, -1, nullptr, nullptr, 0, "Raven Prism", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "ThinkAR", "ThinkAR AiLENS V1", 90, -1, nullptr, nullptr, 0, "AiLENS V1", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Monako", "Monako Glass", 90, -1, nullptr, nullptr, 0, "Monako Glass", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "iFLYTEK", "iFLYTEK AI Glasses", 90, -1, nullptr, nullptr, 0, "iFLYTEK AI Glasses", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "XGIMI", "XGIMI MemoMind One", 90, -1, nullptr, nullptr, 0, "MemoMind One", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Even Realities", "Even Realities G2", 90, -1, nullptr, nullptr, 0, "Even Realities G2", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Lenovo", "Lenovo Vision AI Glasses V1", 90, -1, nullptr, nullptr, 0, "Vision AI Glasses V1", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Alibaba / Quark", "Alibaba Quark AI Glasses S1", 90, -1, nullptr, nullptr, 0, "Quark AI Glasses S1", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "INMO", "INMO GO 3", 90, -1, nullptr, nullptr, 0, "INMO GO 3", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Everysight", "Everysight Maverick Gen 2", 90, -1, nullptr, nullptr, 0, "Maverick Gen 2", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Meta / Ray-Ban", "Meta Ray-Ban Display", 90, -1, nullptr, nullptr, 0, "Meta Ray-Ban Display", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Mira", "Mira", 90, -1, nullptr, nullptr, 0, "Mira", true, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Brilliant Labs", "Brilliant Labs Halo", 90, -1, nullptr, nullptr, 0, "Brilliant Labs Halo", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "XRAI / Leion", "XRAI AR2 / Leion Hey2", 90, -1, nullptr, nullptr, 0, "XRAI AR2", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Engo", "Engo 3", 90, -1, nullptr, nullptr, 0, "Engo 3", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Rokid", "Rokid Glasses", 90, -1, nullptr, nullptr, 0, "Rokid Glasses", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Google", "Google Monocular Display Prototype", 90, -1, nullptr, nullptr, 0, "Google Monocular", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Google", "Google Binocular Display Prototype", 90, -1, nullptr, nullptr, 0, "Google Binocular", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Halliday", "Halliday Glasses", 90, -1, nullptr, nullptr, 0, "Halliday Glasses", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "INMO", "INMO GO 2", 90, -1, nullptr, nullptr, 0, "INMO GO 2", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Meizu", "Meizu StarV Air 2", 90, -1, nullptr, nullptr, 0, "StarV Air 2", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Even Realities", "Even Realities G1", 90, -1, nullptr, nullptr, 0, "Even Realities G1", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Engo", "Engo 2", 90, -1, nullptr, nullptr, 0, "Engo 2", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Everysight", "Everysight Maverick Gen 1", 90, -1, nullptr, nullptr, 0, "Maverick Gen 1", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Vuzix", "Vuzix Z100", 90, -1, nullptr, nullptr, 0, "Vuzix Z100", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Acer", "Acer AR Vision GR0", 90, -1, nullptr, nullptr, 0, "AR Vision GR0", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "RayNeo", "RayNeo GT", 90, -1, nullptr, nullptr, 0, "RayNeo GT", true, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "XREAL", "XREAL xbx a01", 90, -1, nullptr, nullptr, 0, "xbx a01", true, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "RayNeo", "RayNeo GT Max", 90, -1, nullptr, nullptr, 0, "RayNeo GT Max", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "RayNeo", "RayNeo Air 4 Pro", 90, -1, nullptr, nullptr, 0, "RayNeo Air 4 Pro", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "ASUS / XREAL", "ASUS ROG XREAL R1", 90, -1, nullptr, nullptr, 0, "ROG XREAL R1", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "XREAL", "XREAL 1S", 90, -1, nullptr, nullptr, 0, "Xreal 1S", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "RayNeo", "RayNeo Air 3S Pro", 90, -1, nullptr, nullptr, 0, "RayNeo Air 3S Pro", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "VITURE", "VITURE Beast", 90, -1, nullptr, nullptr, 0, "Viture Beast", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Inair", "Inair 2 Pro", 90, -1, nullptr, nullptr, 0, "Inair 2 Pro", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Lenovo", "Lenovo Legion Glasses Gen 2", 90, -1, nullptr, nullptr, 0, "Legion Glasses Gen 2", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "INMO", "INMO Air 3", 90, -1, nullptr, nullptr, 0, "INMO Air 3", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Meizu", "Meizu StarV View", 90, -1, nullptr, nullptr, 0, "StarV View", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "ASUS", "ASUS AirVision M1", 90, -1, nullptr, nullptr, 0, "AirVision M1", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Rokid", "Rokid AR Spatial", 90, -1, nullptr, nullptr, 0, "Rokid AR Spatial", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Rokid", "Rokid AR", 90, -1, nullptr, nullptr, 0, "Rokid AR", true, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Snap", "Snap Specs 6th Gen", 90, -1, nullptr, nullptr, 0, "Snap Specs", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "VITURE", "VITURE Luma Ultra", 90, -1, nullptr, nullptr, 0, "Luma Ultra", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "XREAL", "XREAL Aura", 90, -1, nullptr, nullptr, 0, "Xreal Aura", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "RayNeo", "RayNeo X3 Pro", 90, -1, nullptr, nullptr, 0, "RayNeo X3 Pro", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Meta", "Meta Orion", 90, -1, nullptr, nullptr, 0, "Meta Orion", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Snap", "Snap Spectacles 5th Gen", 90, -1, nullptr, nullptr, 0, "Snap Spectacles", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "XREAL", "XREAL Air 2 Ultra", 90, -1, nullptr, nullptr, 0, "Xreal Air 2 Ultra", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "XREAL", "XREAL One Pro", 90, -1, nullptr, nullptr, 0, "Xreal One Pro", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "Rokid", "Rokid AR Studio", 90, -1, nullptr, nullptr, 0, "Rokid AR Studio", false, 0, false, false, "User-supplied smart-glasses model/name rule" },
    { "DigiLens", "DigiLens ARGO", 90, -1, nullptr, nullptr, 0, "DigiLens ARGO", false, 0, false, false, "User-supplied smart-glasses model/name rule" },


    // ---------------------------------------------------------------------
    // JSON keyword expansion supplied by user.
    // These rules add useful BLE advertised-name/payload keyword forms while
    // preserving the existing 90-point HIGH-tier database policy. Short,
    // generic product names are exact-match only to reduce false positives.
    // ---------------------------------------------------------------------

    // Meta Ray-Ban confirmed payload under Meta Platforms Inc Company ID.
    // ASCII "META_RB_GLASS" = 4D45544152425F474C415353.
    { "Meta Platforms", "Meta Ray-Ban", 90,
      0x01AB, nullptr, "4D45544152425F474C415353", 0, nullptr, false, 0,
      true, true, "Meta Company ID + META_RB_GLASS payload keyword" },

    // TCL/RayNeo: require both TCL Company ID and RayNeo name keyword.
    { "TCL / RayNeo", "TCL RayNeo", 90,
      0x0BC6, nullptr, nullptr, 0, "RayNeo", false, 0,
      true, true, "TCL Company ID + RayNeo advertised-name keyword" },

    // Even Realities keyword forms. Bare G1 is exact-only because G1 is
    // otherwise too generic for substring matching.
    { "Even Realities", "Even Realities G1", 90,
      -1, nullptr, nullptr, 0, "Even Realities", false, 0,
      false, true, "Even Realities advertised-name keyword" },
    { "Even Realities", "Even Realities G1", 90,
      -1, nullptr, nullptr, 0, "G1", true, 0,
      false, true, "Exact G1 advertised name from user keyword database" },

    // Solos AirGo keyword forms.
    { "Solos", "Solos AirGo", 90,
      -1, nullptr, nullptr, 0, "AirGo", false, 0,
      false, true, "AirGo advertised-name keyword" },
    { "Solos", "Solos AirGo", 90,
      -1, nullptr, nullptr, 0, "Solos", false, 0,
      false, true, "Solos advertised-name keyword" },

    // XREAL / legacy Nreal brand keywords.
    { "XREAL", "XREAL smart glasses", 90,
      -1, nullptr, nullptr, 0, "XREAL", false, 0,
      false, true, "XREAL advertised-name keyword" },
    { "XREAL / Nreal", "Nreal smart glasses", 90,
      -1, nullptr, nullptr, 0, "Nreal", false, 0,
      false, true, "Nreal advertised-name keyword" },

    // Brilliant Labs short product names. Exact-only because these words are
    // generic outside the Brilliant Labs context.
    { "Brilliant Labs", "Brilliant Labs Monocle", 90,
      -1, nullptr, nullptr, 0, "Monocle", true, 0,
      true, true, "Exact Monocle advertised name from user keyword database" },
    { "Brilliant Labs", "Brilliant Labs Frame", 90,
      -1, nullptr, nullptr, 0, "Frame", true, 0,
      true, true, "Exact Frame advertised name from user keyword database" },

    { nullptr, nullptr, 0, -1, nullptr, nullptr, 0, nullptr, false, 0, false, false, nullptr }
};
