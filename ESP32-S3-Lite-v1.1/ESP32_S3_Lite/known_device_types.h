#pragma once

/*
 * Runtime result type for known_devices.h annotation lookup.
 *
 * Kept outside the .ino so Arduino's automatic function-prototype generator
 * never has to infer this user-defined type. This is RAM-only annotation state.
 * It does not create alerts, alter confidence, or persist observed BLE MACs.
 */
struct KnownDeviceResult {
    bool matched;
    const char* manufacturer;
    const char* assumedDeviceId;
    const char* deviceType;
};
