#pragma once

/*
 * Known-device annotation engine.
 *
 * This implementation is deliberately kept in a header instead of as functions
 * defined in the Arduino .ino. Arduino automatically generates prototypes for
 * .ino functions and can place a generated prototype before a user-defined
 * result type. Keeping the lookup implementation here prevents that ordering
 * failure while leaving known_devices.h byte-for-byte unchanged.
 *
 * Annotation only: never changes smart-glasses confidence or alert behavior.
 */

static inline bool knownPayloadRuleIsSentinel(const KnownManufacturerPayloadRule& rule) {
    return rule.companyId == 0xFFFF && rule.asciiIdentifier == nullptr &&
           rule.manufacturerHexPattern == nullptr && rule.manufacturer == nullptr &&
           rule.assumedDeviceId == nullptr && rule.deviceType == nullptr;
}

static inline bool knownDeviceRuleIsSentinel(const KnownDeviceRule& rule) {
    return rule.advertisedPattern == nullptr && rule.companyId == 0xFFFF &&
           rule.manufacturer == nullptr && rule.assumedDeviceId == nullptr &&
           rule.deviceType == nullptr;
}

static inline bool evaluateKnownDevice(BLEAdvertisedDevice& device, KnownDeviceResult& result) {
    result = {};

    String name = device.haveName() ? String(device.getName().c_str()) : String("");
    name.trim();
    uint16_t companyId = getCompanyId(device);
    String manufacturerHex = getManufacturerHex(device);
    String manufacturerHexUpper = manufacturerHex;
    manufacturerHexUpper.toUpperCase();

    // More-specific manufacturer-payload identities take priority.
    for (int i = 0; ; ++i) {
        const KnownManufacturerPayloadRule& rule = KNOWN_MANUFACTURER_PAYLOAD_RULES[i];
        if (knownPayloadRuleIsSentinel(rule)) break;
        if (companyId != rule.companyId) continue;
        if (rule.manufacturerHexPattern == nullptr) continue;

        String needle = String(rule.manufacturerHexPattern);
        needle.toUpperCase();
        if (manufacturerHexUpper.indexOf(needle) < 0) continue;

        result.matched = true;
        result.manufacturer = rule.manufacturer ? rule.manufacturer : "";
        result.assumedDeviceId = rule.assumedDeviceId ? rule.assumedDeviceId : "";
        result.deviceType = rule.deviceType ? rule.deviceType : "";
        return true;
    }

    for (int i = 0; ; ++i) {
        const KnownDeviceRule& rule = KNOWN_DEVICES[i];
        if (knownDeviceRuleIsSentinel(rule)) break;

        if (rule.companyId != 0xFFFF) {
            if (companyId == 0xFFFF || companyId != rule.companyId) continue;
        }

        bool nameMatches = false;
        if (rule.matchMode == KNOWN_COMPANY_ID_ONLY) {
            nameMatches = (rule.companyId != 0xFFFF && companyId == rule.companyId);
        } else {
            if (rule.advertisedPattern == nullptr || !name.length()) continue;
            switch (rule.matchMode) {
                case KNOWN_EXACT:
                    nameMatches = name.equalsIgnoreCase(rule.advertisedPattern);
                    break;
                case KNOWN_PREFIX:
                    nameMatches = startsWithIgnoreCase(name, rule.advertisedPattern);
                    break;
                case KNOWN_CONTAINS:
                default:
                    nameMatches = containsIgnoreCase(name, rule.advertisedPattern);
                    break;
            }
        }

        if (!nameMatches) continue;

        result.matched = true;
        result.manufacturer = rule.manufacturer ? rule.manufacturer : "";
        result.assumedDeviceId = rule.assumedDeviceId ? rule.assumedDeviceId : "";
        result.deviceType = rule.deviceType ? rule.deviceType : "";
        return true;
    }

    return false;
}
