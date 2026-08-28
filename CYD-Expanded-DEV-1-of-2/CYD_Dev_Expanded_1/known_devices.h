#pragma once
#include <stdint.h>


/*
 * ========================= USER-EDITABLE TABLE =========================
 * Add defensible device identities to KNOWN_DEVICES[]. This table ONLY fills
 * assumed_device_id / assumed_device_type; it does not create glasses alerts.
 * If you cannot identify a device defensibly, DO NOT add a row for it. The
 * main sketch will then copy the device's own advertised name into
 * assumed_device_id and leave assumed_device_type blank.
 *
 * Row format:
 * { "advertised pattern", companyId-or-0xFFFF, MATCH_MODE,
 *   "manufacturer", "assumed device id", "device type" },
 *
 * MATCH_MODE: KNOWN_CONTAINS, KNOWN_PREFIX, KNOWN_EXACT, or
 * KNOWN_COMPANY_ID_ONLY. Manufacturer-payload identities are kept separately
 * in KNOWN_MANUFACTURER_PAYLOAD_RULES[] below. Keep each final sentinel LAST.
 * =======================================================================
 */
/*
 * v6.2 known-device / assumed-device lookup.
 * This file annotates raw captures; it does NOT itself create a glasses match.
 * If no row matches, the main sketch uses the raw advertised name as
 * assumed_device_id and leaves assumed_device_type blank.
 */
enum KnownDeviceMatchMode : uint8_t {
    KNOWN_CONTAINS = 0,
    KNOWN_PREFIX   = 1,
    KNOWN_EXACT    = 2,
    KNOWN_COMPANY_ID_ONLY = 3
};

struct KnownDeviceRule {
    const char* advertisedPattern;   // nullptr for company-ID-only fallback
    uint16_t    companyId;           // 0xFFFF = ignore
    KnownDeviceMatchMode matchMode;
    const char* manufacturer;
    const char* assumedDeviceId;
    const char* deviceType;
};

// Annotation-only manufacturer-payload identities. These are checked before
// advertised-name rules because the payload identifier is more specific.
// manufacturerHexPattern is matched as a case-insensitive substring of the
// raw manufacturer-data hex string already captured by the scanner.
struct KnownManufacturerPayloadRule {
    uint16_t    companyId;
    const char* asciiIdentifier;
    const char* manufacturerHexPattern;
    const char* manufacturer;
    const char* assumedDeviceId;
    const char* deviceType;
};

static const KnownManufacturerPayloadRule KNOWN_MANUFACTURER_PAYLOAD_RULES[] = {
    // Wellington Drive / AoFrio controller IDs observed in the capture set.
    // Require Company ID 0x0578 and only match the stable embedded ASCII ID.
    { 0x0578, "23M7703", "32334D37373033", "Wellington Drive / AoFrio", "Commercial Controller", "Commercial Controller" },
    { 0x0578, "23M7637", "32334D37363337", "Wellington Drive / AoFrio", "Commercial Controller", "Commercial Controller" },
    { 0x0578, "23M7675", "32334D37363735", "Wellington Drive / AoFrio", "Commercial Controller", "Commercial Controller" },
    { 0x0578, "23M7641", "32334D37363431", "Wellington Drive / AoFrio", "Commercial Controller", "Commercial Controller" },
    { 0x0578, "23M9300", "32334D39333030", "Wellington Drive / AoFrio", "Commercial Controller", "Commercial Controller" },
    { 0x0578, "23M9306", "32334D39333036", "Wellington Drive / AoFrio", "Commercial Controller", "Commercial Controller" },
    { 0x0578, "23M7685", "32334D37363835", "Wellington Drive / AoFrio", "Commercial Controller", "Commercial Controller" },
    { 0x0578, "23M9289", "32334D39323839", "Wellington Drive / AoFrio", "Commercial Controller", "Commercial Controller" },
    { 0x0578, "23M7708", "32334D37373038", "Wellington Drive / AoFrio", "Commercial Controller", "Commercial Controller" },

    { 0xFFFF, nullptr, nullptr, nullptr, nullptr, nullptr }
};

static const KnownDeviceRule KNOWN_DEVICES[] = {
    // Apple — explicit product-category indicators first, generic fallbacks last.
    { "iPhone",      0xFFFF, KNOWN_CONTAINS, "Apple", "Apple iPhone",      "Phone" },
    { "iPad",        0xFFFF, KNOWN_CONTAINS, "Apple", "Apple iPad",        "Tablet" },
    { "AirPods",     0xFFFF, KNOWN_CONTAINS, "Apple", "Apple AirPods",     "Headphones" },
    { "Beats",       0xFFFF, KNOWN_CONTAINS, "Apple", "Apple Beats",       "Headphones" },
    { "Apple Watch", 0xFFFF, KNOWN_CONTAINS, "Apple", "Apple Watch",       "Watch" },
    { "Apple",       0xFFFF, KNOWN_CONTAINS, "Apple", "Apple device",      "Apple device" },
    { nullptr,        0x004C, KNOWN_COMPANY_ID_ONLY, "Apple", "Apple device", "Apple device" },


    // Samsung phone advertised-name prefixes supplied by the user.
    { "S22", 0xFFFF, KNOWN_PREFIX, "Samsung", "Samsung Galaxy S22 series", "Phone" },
    { "S23", 0xFFFF, KNOWN_PREFIX, "Samsung", "Samsung Galaxy S23 series", "Phone" },
    { "S24", 0xFFFF, KNOWN_PREFIX, "Samsung", "Samsung Galaxy S24 series", "Phone" },
    { "S25", 0xFFFF, KNOWN_PREFIX, "Samsung", "Samsung Galaxy S25 series", "Phone" },
    { "S26", 0xFFFF, KNOWN_PREFIX, "Samsung", "Samsung Galaxy S26 series", "Phone" },
    { "S57", 0xFFFF, KNOWN_PREFIX, "Samsung", "Samsung phone",              "Phone" },

    // LG televisions.
    { "LG webOS", 0xFFFF, KNOWN_CONTAINS, "LG", "LG webOS TV", "Television" },
    { "LG TV",    0xFFFF, KNOWN_CONTAINS, "LG", "LG TV",       "Television" },

    // Skullcandy.
    { "Crusher Evo", 0xFFFF, KNOWN_CONTAINS, "Skullcandy", "Skullcandy Crusher Evo", "Headphones" },
    { "Crusher",     0xFFFF, KNOWN_CONTAINS, "Skullcandy", "Skullcandy Crusher",     "Headphones" },

    // Sony devices seen/identified in captures.
    { "LE_WH-1000XM4", 0xFFFF, KNOWN_CONTAINS, "Sony", "Sony WH-1000XM4", "Headphones" },
    { "LE_WH-1000XM5", 0xFFFF, KNOWN_CONTAINS, "Sony", "Sony WH-1000XM5", "Headphones" },
    { "LE_WH-1000XM6", 0xFFFF, KNOWN_CONTAINS, "Sony", "Sony WH-1000XM6", "Headphones" },
    { "LE_WF-1000XM5", 0xFFFF, KNOWN_CONTAINS, "Sony", "Sony WF-1000XM5", "Earbuds" },
    { "LE_WF-C510",    0xFFFF, KNOWN_CONTAINS, "Sony", "Sony WF-C510",    "Earbuds" },
    { "LE_WF-C710N",   0xFFFF, KNOWN_CONTAINS, "Sony", "Sony WF-C710N",   "Earbuds" },
    { "LE_LinkBuds S", 0xFFFF, KNOWN_CONTAINS, "Sony", "Sony LinkBuds S", "Earbuds" },
    { "LE_ULT WEAR",   0xFFFF, KNOWN_CONTAINS, "Sony", "Sony ULT Wear",   "Headphones" },
    { "LE_ULT FIELD 3",0xFFFF, KNOWN_EXACT,    "Sony", "Sony ULT FIELD 3", "Speaker" },
    { "LE_SRS-XP700",  0xFFFF, KNOWN_CONTAINS, "Sony", "Sony SRS-XP700",  "Speaker" },
    { "ZV-1M2",        0xFFFF, KNOWN_CONTAINS, "Sony", "Sony ZV-1 II",     "Camera" },

    // Epson.
    { "ET-2820 Series", 0xFFFF, KNOWN_CONTAINS, "Epson", "Epson ET-2820", "Printer" },

    // POS / receipt equipment.
    { "Square Reader",   0xFFFF, KNOWN_CONTAINS, "Square",  "Square Reader",   "POS System" },
    { "Square Terminal", 0xFFFF, KNOWN_CONTAINS, "Square",  "Square Terminal", "POS System" },
    { "RW60-",           0xFFFF, KNOWN_PREFIX,   "Element", "Element RW60",     "POS Receipt Printer" },

    // Kmart/Anko Camera Glasses. Official Kmart manual for item 43700141
    // (model JLR-82067) specifies Bluetooth device name "Anko43700141".
    { "Anko43700141", 0xFFFF, KNOWN_EXACT, "Kmart Australia Limited / Anko",
      "Anko Camera Glasses 43700141 (JLR-82067)", "Camera Smart Glasses" },

    // Other defensible matches found in today's raw advertised names.
    { "Daelibs",       0xFFFF, KNOWN_CONTAINS, "Daelibs",    "Daelibs BLE device",            "Workforce/Facilities Beacon" },
    { "Zeus Athletica",0xFFFF, KNOWN_CONTAINS, "Olympia",    "Olympia Zeus Athletica",        "Massage Chair" },
    { "DC Mini 3",     0xFFFF, KNOWN_CONTAINS, "SKYCUT",     "SKYCUT DC Mini 3",              "Cutting Plotter" },
    { "FilmCut_",      0xFFFF, KNOWN_PREFIX,   "FilmCut",    "FilmCut cutting system",        "Cutting Plotter" },
    { "Smart.A5.WIFI", 0xFFFF, KNOWN_EXACT,    "Aroma-Link", "Aroma-Link A5",                 "Scent Diffuser" },
    { "HCHLOR",        0xFFFF, KNOWN_CONTAINS, "AstralPool", "AstralPool Halo Chlorinator",   "Pool Chlorinator" },
    { "TGW01",         0xFFFF, KNOWN_CONTAINS, "VeryFit",    "TGW01 Smartwatch",              "Smartwatch" },
    { "AVINOX",        0xFFFF, KNOWN_PREFIX,   "DJI",        "DJI Avinox",                    "E-Bike Drive System" },
    { "OM8P-",         0xFFFF, KNOWN_PREFIX,   "DJI",        "DJI Osmo Mobile 8P",            "Phone Gimbal" },
    { "ViscontiCANBox_", 0xFFFF, KNOWN_PREFIX, "Visconti Tuning", "Visconti CANBox", "Automotive CAN/BLE Device" },

    // AoFrio / Wellington Drive Technologies commercial refrigeration BLE.
    // Observed in the Woolworths capture set. Require Company ID 0x0578 so
    // the structured C/P name prefixes are not treated as globally unique.
    { "C102E", 0x0578, KNOWN_PREFIX, "Wellington Drive / AoFrio", "Commercial Fridge", "Commercial Refrigeration" },
    { "C113E", 0x0578, KNOWN_PREFIX, "Wellington Drive / AoFrio", "Commercial Fridge", "Commercial Refrigeration" },
    { "P346C", 0x0578, KNOWN_PREFIX, "Wellington Drive / AoFrio", "Commercial Fridge", "Commercial Refrigeration" },


    // ---------------------------------------------------------------------
    // v6.2 Session 06 expansion — defensible ordinary BLE devices from the user's
    // JB Hi-Fi / OPSM capture set. These rules annotate assumed_device_id
    // and assumed_device_type only; they DO NOT create glasses alerts.
    // ---------------------------------------------------------------------

    // Generic smart-light identity seen with Company ID 0x0211.
    { "Smart Light", 0x0211, KNOWN_EXACT, "Generic", "BLE Smart Light", "Smart Light" },

    // JBL audio.
    { "JBL Tune Flex 2", 0xFFFF, KNOWN_CONTAINS, "JBL", "JBL Tune Flex 2", "Earbuds" },
    { "JBL Tune 770NC",  0xFFFF, KNOWN_CONTAINS, "JBL", "JBL Tune 770NC",  "Headphones" },
    { "JBL Live 770NC",  0xFFFF, KNOWN_CONTAINS, "JBL", "JBL Live 770NC",  "Headphones" },
    { "JBL LIVE670NC",   0xFFFF, KNOWN_CONTAINS, "JBL", "JBL Live 670NC",  "Headphones" },
    { "JBL Flip 6",      0xFFFF, KNOWN_CONTAINS, "JBL", "JBL Flip 6",      "Speaker" },
    { "JBL Flip 7",      0xFFFF, KNOWN_EXACT,    "JBL", "JBL Flip 7",      "Speaker" },
    { "JBL Flip 5",      0xFFFF, KNOWN_EXACT,    "JBL", "JBL Flip 5",      "Speaker" },
    { "JBL Clip 5",      0xFFFF, KNOWN_EXACT,    "JBL", "JBL Clip 5",      "Speaker" },
    { "JBL PartyBox 710",0xFFFF, KNOWN_EXACT,    "JBL", "JBL PartyBox 710","Speaker" },
    { "JBL TUNE130NC TWS-LE", 0xFFFF, KNOWN_EXACT, "JBL", "JBL TUNE130NC TWS-LE", "Earbuds" },

    // Bose audio.
    { "Bose QC 45",             0xFFFF, KNOWN_CONTAINS, "Bose", "Bose QuietComfort 45",         "Headphones" },
    { "Bose Flex 2 SoundLink",   0xFFFF, KNOWN_CONTAINS, "Bose", "Bose SoundLink Flex 2nd Gen", "Speaker" },
    { "Bose S1 Pro+",            0xFFFF, KNOWN_CONTAINS, "Bose", "Bose S1 Pro+",                 "PA Speaker" },
    { "Bose Color II SoundLink", 0xFFFF, KNOWN_CONTAINS, "Bose", "Bose SoundLink Color II",     "Speaker" },

    // Samsung wearables.
    { "Galaxy Watch", 0xFFFF, KNOWN_CONTAINS, "Samsung", "Samsung Galaxy Watch", "Smartwatch" },
    { "Galaxy Buds",  0xFFFF, KNOWN_CONTAINS, "Samsung", "Samsung Galaxy Buds",  "Earbuds" },

    // Govee / smart lighting.
    { "ihoment_H6004", 0xFFFF, KNOWN_PREFIX, "Govee", "Govee H6004 Smart LED Bulb",       "Smart Light" },
    { "Govee_H61D4",   0xFFFF, KNOWN_PREFIX, "Govee", "Govee H61D4 Neon Rope Light 2",    "Smart Light" },
    { "Govee_H6056",   0xFFFF, KNOWN_PREFIX, "Govee", "Govee H6056 Flow Plus Light Bars", "Smart Light" },
    { "Govee_H6098",   0xFFFF, KNOWN_PREFIX, "Govee", "Govee H6098 TV Backlight",          "Smart Light" },
    { "Govee_",        0xFFFF, KNOWN_PREFIX, "Govee", "Govee device",                       "Smart Home Device" },
    { "Hue Lamp",      0xFFFF, KNOWN_EXACT,  "Philips Hue", "Philips Hue Lamp",             "Smart Light" },
    { "LEDnetWF",      0xFFFF, KNOWN_PREFIX, "LEDnet", "LEDnetWF device",                    "Lighting Device" },

    // Other audio.
    { "ACTON III", 0xFFFF, KNOWN_CONTAINS, "Marshall",   "Marshall Acton III",     "Speaker" },
    { "HD 450BT",  0xFFFF, KNOWN_EXACT,    "Sennheiser", "Sennheiser HD 450BT",   "Headphones" },

    // Wearables / fitness.
    { "WHOOP",                0xFFFF, KNOWN_CONTAINS, "WHOOP",     "WHOOP wearable",             "Fitness Tracker" },
    { "Xiaomi Band 9 Active", 0xFFFF, KNOWN_CONTAINS, "Xiaomi",    "Xiaomi Smart Band 9 Active", "Fitness Tracker" },
    { "Theragun G6 Prime",    0xFFFF, KNOWN_CONTAINS, "Therabody", "Theragun Prime 6th Gen",     "Massage Gun" },

    // Camera.
    { "EOSR50_", 0xFFFF, KNOWN_PREFIX, "Canon", "Canon EOS R50", "Camera" },

    // Soundbars / home audio.
    { "Hisense AX5140Q", 0xFFFF, KNOWN_CONTAINS, "Hisense", "Hisense AX5140Q", "Soundbar" },
    { "LG S70TY",        0xFFFF, KNOWN_CONTAINS, "LG",      "LG S70TY",        "Soundbar" },
    { "LG S40T",         0xFFFF, KNOWN_CONTAINS, "LG",      "LG S40T",         "Soundbar" },
    { "LG S60T",         0xFFFF, KNOWN_CONTAINS, "LG",      "LG S60T",         "Soundbar" },

    { nullptr, 0xFFFF, KNOWN_CONTAINS, nullptr, nullptr, nullptr }
};
