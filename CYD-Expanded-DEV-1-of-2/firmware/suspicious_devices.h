#pragma once
#include <stdint.h>

/*
 * ======================= USER-EDITABLE REVIEW RULES ======================
 * This table is a separate defensive/review layer. It does NOT create,
 * upgrade, downgrade, or suppress smart-glasses confidence matches.
 *
 * Rules are intended to identify BLE development platforms, DIY devices,
 * unusual beacons, and other devices the operator may want to review.
 * A match is NOT proof of malicious intent.
 *
 * Name matching is case-insensitive. The primary pattern is the main model /
 * family indicator. The optional secondary pattern is a supporting name clue;
 * when it is also present, the review confidence is increased by
 * secondaryBoost points (capped at 99). The secondary pattern alone never
 * triggers a rule.
 *
 * Optional companyId and oui24 fields are provided for future stronger clues.
 * Leave companyId = -1 and oui24 = 0 when they are not required.
 *
 * Keep the final nullptr sentinel LAST.
 * ========================================================================
 */

enum SuspiciousNameMatchMode : uint8_t {
    SUSPICIOUS_CONTAINS = 0,
    SUSPICIOUS_PREFIX   = 1,
    SUSPICIOUS_EXACT    = 2
};

struct SuspiciousDeviceRule {
    const char* category;
    const char* deviceLabel;
    const char* primaryNamePattern;
    const char* secondaryNamePattern;
    SuspiciousNameMatchMode nameMode;
    int32_t     companyId;       // -1 = not required
    uint32_t    oui24;           // 0 = not required
    uint8_t     confidence;      // review confidence, 1..99
    uint8_t     secondaryBoost;  // added only when secondary name also matches
    const char* reason;
};

// --------------------------- ALERT SETTINGS -----------------------------
#define SUSPICIOUS_ALERT_MIN_CONFIDENCE  60
#define SUSPICIOUS_ALERT_MS              10000UL
#define SUSPICIOUS_DEVICE_COOLDOWN_MS    20000UL

// On-screen wording. Keep each line short enough for the 320x240 CYD TFT.
#define SUSPICIOUS_TITLE_LINE_1  "HOMEMADE / SUSPICIOUS"
#define SUSPICIOUS_TITLE_LINE_2  "BLUETOOTH DEVICE"
#define SUSPICIOUS_CAUTION_LINE_1 "DEVICE TYPES LIKE THIS MAY BE USED"
#define SUSPICIOUS_CAUTION_LINE_2 "FOR LEGITIMATE OR HARMFUL PURPOSES."
#define SUSPICIOUS_CAUTION_LINE_3 "A MATCH DOES NOT PROVE MALICIOUS INTENT."
#define SUSPICIOUS_CAUTION_LINE_4 "EXERCISE CAUTION:"
#define SUSPICIOUS_CAUTION_LINE_5 "AN UNUSUAL BLUETOOTH DEVICE IS NEARBY."

/*
 * Review categories represented below:
 *   - homemade / DIY BLE device
 *   - unknown ESP32-family device
 *   - unknown Nordic nRF-family device
 *   - BLE development board
 *   - unusual beacon
 *   - named development / research platforms
 *
 * Rotating/private BLE addresses are common on ordinary phones and wearables,
 * so address rotation is NOT automatically treated as suspicious here.
 * Add a behavioural rule only if you have a defensible local use case.
 */
static const SuspiciousDeviceRule SUSPICIOUS_DEVICE_RULES[] = {
    // Flipper Zero / controller naming.
    { "DEV PLATFORM", "POSSIBLE FLIPPER ZERO", "Flipper Zero", "Control", SUSPICIOUS_CONTAINS, -1, 0, 92, 5, "Flipper Zero advertised-name clue" },
    { "DEV PLATFORM", "POSSIBLE FLIPPER ZERO", "Flipper",      "Control", SUSPICIOUS_CONTAINS, -1, 0, 88, 5, "Flipper-family advertised-name clue" },

    // Espressif / ESP32 families. Bare "ESP" is intentionally not used.
    { "DIY / DEV", "POSSIBLE ESP32-S3", "ESP32-S3", "Espressif", SUSPICIOUS_CONTAINS, -1, 0, 78, 7, "ESP32-S3 model-name clue" },
    { "DIY / DEV", "POSSIBLE ESP32-C3", "ESP32-C3", "Espressif", SUSPICIOUS_CONTAINS, -1, 0, 78, 7, "ESP32-C3 model-name clue" },
    { "DIY / DEV", "POSSIBLE ESP32-C6", "ESP32-C6", "Espressif", SUSPICIOUS_CONTAINS, -1, 0, 78, 7, "ESP32-C6 model-name clue" },
    { "DIY / DEV", "POSSIBLE ESP32-H2", "ESP32-H2", "Espressif", SUSPICIOUS_CONTAINS, -1, 0, 78, 7, "ESP32-H2 model-name clue" },
    { "DIY / DEV", "POSSIBLE ESP32",    "ESP32",    "Espressif", SUSPICIOUS_CONTAINS, -1, 0, 72, 7, "ESP32-family advertised-name clue" },
    { "DIY / DEV", "POSSIBLE ESP32",    "ESP32-",   "ESP_",      SUSPICIOUS_PREFIX,   -1, 0, 72, 5, "ESP32-style advertised-name prefix" },

    // CYD / ESP32-2432S028R indicators.
    { "DIY / DEV", "POSSIBLE CYD", "ESP32-2432S028R", "CYD", SUSPICIOUS_CONTAINS, -1, 0, 90, 5, "CYD board model-name clue" },
    { "DIY / DEV", "POSSIBLE CYD", "ESP32-2432S028",  "CYD", SUSPICIOUS_CONTAINS, -1, 0, 88, 5, "CYD board model-name clue" },
    { "DIY / DEV", "POSSIBLE CYD", "2432S028R",       "CYD", SUSPICIOUS_CONTAINS, -1, 0, 85, 5, "CYD board model-name clue" },
    { "DIY / DEV", "POSSIBLE CYD", "Cheap Yellow Display", "CYD", SUSPICIOUS_CONTAINS, -1, 0, 85, 5, "CYD descriptive-name clue" },

    // M5Stack / Cardputer / Atom / Stick / Core / Stamp families.
    { "DEV PLATFORM", "POSSIBLE M5 CARDPUTER", "M5Cardputer", "M5", SUSPICIOUS_CONTAINS, -1, 0, 90, 5, "M5 Cardputer name clue" },
    { "DEV PLATFORM", "POSSIBLE M5 CARDPUTER", "Cardputer",   "M5", SUSPICIOUS_CONTAINS, -1, 0, 88, 5, "Cardputer name clue" },
    { "DEV PLATFORM", "POSSIBLE M5 CARDPUTER ADV", "Cardputer-Adv", "M5", SUSPICIOUS_CONTAINS, -1, 0, 90, 5, "Cardputer Adv name clue" },
    { "DEV PLATFORM", "POSSIBLE M5 CORES3", "M5CoreS3", "M5", SUSPICIOUS_CONTAINS, -1, 0, 86, 5, "M5 CoreS3 name clue" },
    { "DEV PLATFORM", "POSSIBLE M5 CORES3", "CoreS3",   "M5", SUSPICIOUS_CONTAINS, -1, 0, 84, 5, "CoreS3 name clue" },
    { "DEV PLATFORM", "POSSIBLE M5 CORE2",  "M5Core2",  "M5", SUSPICIOUS_CONTAINS, -1, 0, 84, 5, "M5 Core2 name clue" },
    { "DEV PLATFORM", "POSSIBLE M5 STICK",  "M5StickC", "M5", SUSPICIOUS_CONTAINS, -1, 0, 84, 5, "M5StickC name clue" },
    { "DEV PLATFORM", "POSSIBLE M5 STICK",  "StickS3",  "M5", SUSPICIOUS_CONTAINS, -1, 0, 84, 5, "M5 StickS3 name clue" },
    { "DEV PLATFORM", "POSSIBLE M5 ATOM",   "M5Atom",   "M5", SUSPICIOUS_CONTAINS, -1, 0, 82, 5, "M5 Atom name clue" },
    { "DEV PLATFORM", "POSSIBLE M5 ATOM",   "AtomS3",   "M5", SUSPICIOUS_CONTAINS, -1, 0, 82, 5, "M5 AtomS3 name clue" },
    { "DEV PLATFORM", "POSSIBLE M5 STAMP",  "M5StampS3","M5", SUSPICIOUS_CONTAINS, -1, 0, 82, 5, "M5 StampS3 name clue" },
    { "DEV PLATFORM", "POSSIBLE M5 STAMP",  "StampS3",  "M5", SUSPICIOUS_CONTAINS, -1, 0, 80, 5, "StampS3 name clue" },
    { "DEV PLATFORM", "POSSIBLE M5 DIAL",   "M5Dial",   "M5", SUSPICIOUS_CONTAINS, -1, 0, 82, 5, "M5 Dial name clue" },
    { "DEV PLATFORM", "POSSIBLE M5 DEVICE", "M5Stack",  "M5", SUSPICIOUS_CONTAINS, -1, 0, 76, 5, "M5Stack family-name clue" },

    // Arduino BLE-capable product/family names.
    { "DEV PLATFORM", "POSSIBLE ARDUINO NANO 33 BLE", "Nano 33 BLE", "Arduino", SUSPICIOUS_CONTAINS, -1, 0, 88, 5, "Arduino Nano 33 BLE name clue" },
    { "DEV PLATFORM", "POSSIBLE ARDUINO NANO 33 BLE", "Nano33BLE",    "Arduino", SUSPICIOUS_CONTAINS, -1, 0, 88, 5, "Arduino Nano33BLE name clue" },
    { "DEV PLATFORM", "POSSIBLE ARDUINO UNO R4 WIFI", "UNO R4 WiFi", "Arduino", SUSPICIOUS_CONTAINS, -1, 0, 86, 5, "Arduino UNO R4 WiFi name clue" },
    { "DEV PLATFORM", "POSSIBLE ARDUINO NANO ESP32",  "Nano ESP32",  "Arduino", SUSPICIOUS_CONTAINS, -1, 0, 86, 5, "Arduino Nano ESP32 name clue" },
    { "DEV PLATFORM", "POSSIBLE ARDUINO NICLA", "Nicla Sense ME", "Arduino", SUSPICIOUS_CONTAINS, -1, 0, 84, 5, "Arduino Nicla Sense ME name clue" },
    { "DEV PLATFORM", "POSSIBLE ARDUINO NICLA", "Nicla Vision",   "Arduino", SUSPICIOUS_CONTAINS, -1, 0, 84, 5, "Arduino Nicla Vision name clue" },
    { "DEV PLATFORM", "POSSIBLE ARDUINO NICLA", "Nicla Voice",    "Arduino", SUSPICIOUS_CONTAINS, -1, 0, 84, 5, "Arduino Nicla Voice name clue" },
    { "DEV PLATFORM", "POSSIBLE ARDUINO PORTENTA", "Portenta H7", "Arduino", SUSPICIOUS_CONTAINS, -1, 0, 82, 5, "Arduino Portenta H7 name clue" },
    { "DEV PLATFORM", "POSSIBLE ARDUINO GIGA R1",   "GIGA R1",     "Arduino", SUSPICIOUS_CONTAINS, -1, 0, 82, 5, "Arduino GIGA R1 name clue" },
    { "DEV PLATFORM", "POSSIBLE ARDUINO", "Arduino", nullptr, SUSPICIOUS_CONTAINS, -1, 0, 68, 0, "Generic Arduino advertised-name clue" },

    // Nordic Semiconductor / nRF development-family names.
    { "DEV PLATFORM", "POSSIBLE NRF52840 DK", "nRF52840 DK", "Nordic", SUSPICIOUS_CONTAINS, -1, 0, 90, 5, "Nordic nRF52840 DK name clue" },
    { "DEV PLATFORM", "POSSIBLE NRF52 DK",    "nRF52 DK",    "Nordic", SUSPICIOUS_CONTAINS, -1, 0, 88, 5, "Nordic nRF52 DK name clue" },
    { "DEV PLATFORM", "POSSIBLE NRF5340 DK",  "nRF5340 DK",  "Nordic", SUSPICIOUS_CONTAINS, -1, 0, 90, 5, "Nordic nRF5340 DK name clue" },
    { "DEV PLATFORM", "POSSIBLE NRF5340 AUDIO DK", "nRF5340 Audio DK", "Nordic", SUSPICIOUS_CONTAINS, -1, 0, 92, 5, "Nordic nRF5340 Audio DK name clue" },
    { "DEV PLATFORM", "POSSIBLE NRF54L15 DK", "nRF54L15 DK", "Nordic", SUSPICIOUS_CONTAINS, -1, 0, 90, 5, "Nordic nRF54L15 DK name clue" },
    { "DIY / DEV", "POSSIBLE NRF52840", "nRF52840", "Nordic", SUSPICIOUS_CONTAINS, -1, 0, 80, 5, "nRF52840 family-name clue" },
    { "DIY / DEV", "POSSIBLE NRF52833", "nRF52833", "Nordic", SUSPICIOUS_CONTAINS, -1, 0, 80, 5, "nRF52833 family-name clue" },
    { "DIY / DEV", "POSSIBLE NRF52832", "nRF52832", "Nordic", SUSPICIOUS_CONTAINS, -1, 0, 80, 5, "nRF52832 family-name clue" },
    { "DIY / DEV", "POSSIBLE NRF5340",  "nRF5340",  "Nordic", SUSPICIOUS_CONTAINS, -1, 0, 80, 5, "nRF5340 family-name clue" },
    { "DIY / DEV", "POSSIBLE NRF54L15", "nRF54L15", "Nordic", SUSPICIOUS_CONTAINS, -1, 0, 80, 5, "nRF54L15 family-name clue" },
    { "DIY / DEV", "POSSIBLE NRF DEVICE", "Nordic Semiconductor", "nRF", SUSPICIOUS_CONTAINS, -1, 0, 76, 5, "Nordic Semiconductor name clue" },

    // Raspberry Pi / RP-family names. These are review clues, not proof that a
    // Linux Raspberry Pi itself is the BLE advertiser.
    { "DEV PLATFORM", "POSSIBLE RASPBERRY PI", "Raspberry Pi", "RPi", SUSPICIOUS_CONTAINS, -1, 0, 82, 5, "Raspberry Pi advertised-name clue" },
    { "DEV PLATFORM", "POSSIBLE RASPBERRY PI", "RaspberryPi",  "RPi", SUSPICIOUS_CONTAINS, -1, 0, 82, 5, "RaspberryPi advertised-name clue" },
    { "DEV PLATFORM", "POSSIBLE RASPBERRY PI 5", "RPi5", "Raspberry", SUSPICIOUS_CONTAINS, -1, 0, 82, 5, "RPi5 name clue" },
    { "DEV PLATFORM", "POSSIBLE RASPBERRY PI 4", "RPi4", "Raspberry", SUSPICIOUS_CONTAINS, -1, 0, 82, 5, "RPi4 name clue" },
    { "DEV PLATFORM", "POSSIBLE PI ZERO 2 W", "Pi Zero 2 W", "Raspberry", SUSPICIOUS_CONTAINS, -1, 0, 84, 5, "Pi Zero 2 W name clue" },
    { "DEV PLATFORM", "POSSIBLE COMPUTE MODULE", "Compute Module 5", "Raspberry", SUSPICIOUS_CONTAINS, -1, 0, 82, 5, "Compute Module name clue" },
    { "DEV PLATFORM", "POSSIBLE PICO 2 W", "Pico 2 W", "Raspberry", SUSPICIOUS_CONTAINS, -1, 0, 80, 5, "Pico 2 W name clue" },
    { "DEV PLATFORM", "POSSIBLE PICO W",   "Pico W",   "Raspberry", SUSPICIOUS_CONTAINS, -1, 0, 78, 5, "Pico W name clue" },

    // Explicit DIY/development naming.
    { "DIY / DEV", "POSSIBLE DIY BLE DEVICE", "DIY BLE",    "Custom", SUSPICIOUS_CONTAINS, -1, 0, 72, 5, "DIY BLE advertised-name clue" },
    { "DIY / DEV", "POSSIBLE CUSTOM BLE DEVICE", "Custom BLE", "DIY", SUSPICIOUS_CONTAINS, -1, 0, 72, 5, "Custom BLE advertised-name clue" },
    { "DIY / DEV", "POSSIBLE BLE DEV BOARD", "BLE Dev",    "Board", SUSPICIOUS_CONTAINS, -1, 0, 70, 5, "BLE development-board name clue" },
    { "DIY / DEV", "POSSIBLE DEV BOARD",     "Dev Board",  "BLE",   SUSPICIOUS_CONTAINS, -1, 0, 68, 5, "Development-board advertised-name clue" },

    // Beacon names are deliberately low-review-confidence because legitimate
    // beacon deployments are common. They still meet the default orange-review
    // threshold so operators can decide whether they are unusual locally.
    { "UNUSUAL BEACON", "POSSIBLE IBEACON",    "iBeacon",   "BLE", SUSPICIOUS_CONTAINS, -1, 0, 64, 3, "Beacon advertised-name clue" },
    { "UNUSUAL BEACON", "POSSIBLE EDDYSTONE",  "Eddystone", "BLE", SUSPICIOUS_CONTAINS, -1, 0, 64, 3, "Beacon advertised-name clue" },
    { "UNUSUAL BEACON", "POSSIBLE ALTBEACON",  "AltBeacon", "BLE", SUSPICIOUS_CONTAINS, -1, 0, 64, 3, "Beacon advertised-name clue" },
    { "UNUSUAL BEACON", "POSSIBLE BLE BEACON", "BLE Beacon","Beacon", SUSPICIOUS_CONTAINS, -1, 0, 62, 3, "Generic BLE beacon name clue" },

    { nullptr, nullptr, nullptr, nullptr, SUSPICIOUS_CONTAINS, -1, 0, 0, 0, nullptr }
};
