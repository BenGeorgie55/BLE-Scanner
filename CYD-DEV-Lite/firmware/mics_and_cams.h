#pragma once
#include <stdint.h>

/*
 * ===================== USER-EDITABLE CAM AND AUDIO RULES ====================
 * This table powers the separate CAM AND AUDIO BLE review engine.
 * It is independent from both the smart-glasses confidence engine and
 * suspicious_devices.h.
 *
 * The rules below are intentionally human-readable. Nothing is hashed.
 * Add, remove, or edit rows here without changing the main .ino file.
 *
 * A rule may use one or more observable BLE advertisement clues:
 *   - advertised name / name pattern
 *   - Bluetooth Company ID
 *   - manufacturer-data hex substring
 *   - advertised 16-bit service UUID
 *   - public MAC OUI prefix
 *
 * IMPORTANT MATCHING RULE:
 * Every populated criterion in one row must match the same advertisement.
 * Leave a field disabled when you do not want it considered:
 *   namePattern     = nullptr
 *   companyId       = -1
 *   manufacturerHex = nullptr
 *   serviceUuid16   = 0
 *   oui24           = 0
 *
 * This database should contain only camera/audio-related BLE indicators that
 * are defensible from observed captures or reliable public documentation.
 * Do not add a Wi-Fi-only product merely because it contains a camera or mic.
 *
 * A match means the BLE advertisement resembles camera/audio-capable hardware.
 * It does NOT prove that a camera/microphone is concealed, enabled, recording,
 * or being used for surveillance.
 *
 * Keep the final nullptr sentinel LAST.
 * ============================================================================
 */

enum MicCamNameMatchMode : uint8_t {
    MICCAM_CONTAINS = 0,
    MICCAM_PREFIX   = 1,
    MICCAM_EXACT    = 2
};

enum MicCamCategory : uint8_t {
    MICCAM_CATEGORY_CAMERA = 0,
    MICCAM_CATEGORY_MICROPHONE,
    MICCAM_CATEGORY_CAMERA_AUDIO,
    MICCAM_CATEGORY_RECORDER
};

struct MicCamRule {
    const char* deviceLabel;          // Human-readable alert label
    MicCamCategory category;          // CAMERA / MICROPHONE / CAMERA+AUDIO / RECORDER
    const char* namePattern;          // nullptr = ignore advertised name
    MicCamNameMatchMode nameMode;
    int32_t companyId;                // -1 = ignore Bluetooth Company ID
    const char* manufacturerHex;      // nullptr = ignore manufacturer-data substring
    uint16_t serviceUuid16;           // 0 = ignore 16-bit advertised service UUID
    uint32_t oui24;                   // 0 = ignore public MAC OUI
    uint8_t confidence;               // review confidence, 1..99
    const char* reason;
};

// --------------------------- ALERT SETTINGS -----------------------------
#define MICCAM_ALERT_MIN_CONFIDENCE  60
#define MICCAM_ALERT_MS              10000UL
#define MICCAM_DEVICE_COOLDOWN_MS    20000UL

// User-facing mode / warning wording.
#define MICCAM_TITLE_LINE_1           "CAM AND AUDIO"
#define MICCAM_TITLE_LINE_2           "POSSIBLE CAMERA / MIC DEVICE"
#define MICCAM_CAUTION_LINE_1         "CAMERA OR AUDIO-CAPABLE BLE"
#define MICCAM_CAUTION_LINE_2         "HARDWARE MAY BE NEARBY."
#define MICCAM_CAUTION_LINE_3         "A BLUETOOTH MATCH DOES NOT PROVE"
#define MICCAM_CAUTION_LINE_4         "THAT RECORDING IS OCCURRING."

/*
 * Initial active seed rules.
 *
 * These are deliberately conservative. Retail products for which Bluetooth is
 * disclosed but no observable BLE fingerprint is known are NOT added as active
 * rules. Add those later after capturing/confirming the advertisement.
 */
static const MicCamRule MIC_CAM_RULES[] = {
    // CamSC/CamSC-Pro style provisioning name seen in camera/audio products.
    { "CAMERA / AUDIO DEVICE", MICCAM_CATEGORY_CAMERA_AUDIO,
      "CAM-", MICCAM_PREFIX, -1, nullptr, 0, 0, 72,
      "CAM- Bluetooth provisioning-name clue used by camera/audio products" },

    // Explicit camera-development-board advertised names. These are camera-
    // specific and therefore belong here rather than as generic ESP32 rules.
    { "ESP32-CAM DEVICE", MICCAM_CATEGORY_CAMERA,
      "ESP32-CAM", MICCAM_CONTAINS, -1, nullptr, 0, 0, 88,
      "Explicit ESP32-CAM advertised-name clue" },

    { "M5 CAMERA DEVICE", MICCAM_CATEGORY_CAMERA,
      "M5Camera", MICCAM_CONTAINS, -1, nullptr, 0, 0, 86,
      "Explicit M5Camera advertised-name clue" },

    // Sentinel -- KEEP LAST.
    { nullptr, MICCAM_CATEGORY_CAMERA, nullptr, MICCAM_CONTAINS,
      -1, nullptr, 0, 0, 0, nullptr }
};
