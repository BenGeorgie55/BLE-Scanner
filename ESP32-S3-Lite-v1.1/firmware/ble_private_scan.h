#pragma once

/*
 * ==========================================================================
 * ESP32 S3 Glasses Scanner Lite v1.1 — NimBLE PRIVATE NRPA adapter
 * ==========================================================================
 * Target:
 *   Arduino-ESP32 3.3.11
 *   ESP32-S3
 *   built-in Arduino BLE library using the NimBLE backend
 *
 * The CYD v6.3 development firmware uses a Bluedroid-specific private-address
 * adapter. ESP32-S3 under Arduino-ESP32 3.3.11 uses NimBLE, so Lite v1.1 uses
 * NimBLE's identity API instead.
 *
 * PRIVATE path:
 *   ble_hs_id_gen_rnd(1, ...) -> generate an NRPA
 *   BLEDevice::setOwnAddr(...) -> install the random address
 *   BLEDevice::setOwnAddrType(BLE_OWN_ADDR_RANDOM)
 *
 * Arduino-ESP32 BLEScan::start() passes BLEDevice's selected own-address type
 * directly to NimBLE's ble_gap_disc(), so active Scan Requests use the
 * configured random/NRPA address.
 * ==========================================================================
 */

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEAddress.h>
#include <string.h>
#include "sdkconfig.h"

#if !defined(CONFIG_NIMBLE_ENABLED)
#error "ESP32 S3 Glasses Scanner Lite v1.1 requires the Arduino-ESP32 NimBLE backend. Select an ESP32-S3 board using ESP32 by Espressif Systems 3.3.11."
#endif

#include <host/ble_hs.h>
#include <host/ble_gap.h>

#define BLE_PRIVATE_ADDR_ROTATE_MIN_MS  (12UL * 60UL * 1000UL)
#define BLE_PRIVATE_ADDR_ROTATE_MAX_MS  (18UL * 60UL * 1000UL)
#define BLE_PRIVATE_ADDR_CONFIRM_MS     1000UL

#if BLE_PRIVATE_ADDR_ROTATE_MAX_MS <= BLE_PRIVATE_ADDR_ROTATE_MIN_MS
#error "BLE private-address maximum rotation interval must exceed the minimum"
#endif

/*
 * NimBLE address byte order:
 *   ble_hs_id_* APIs use host byte order (little-endian).
 *   For a valid NRPA the two most-significant address bits are 00, therefore
 *   in this native buffer they are bits 7..6 of byte [5].
 */
static inline bool bleNimbleNativeAddressIsNrpa(const uint8_t addr[6]) {
    return addr != nullptr && ((addr[5] & 0xC0U) == 0x00U);
}

static inline bool bleNimbleGenerateNrpa(uint8_t outAddr[6]) {
    if (outAddr == nullptr) return false;

    ble_addr_t generated = {};
    int rc = ble_hs_id_gen_rnd(1, &generated);  // 1 = non-resolvable private
    if (rc != 0) return false;
    if (!bleNimbleNativeAddressIsNrpa(generated.val)) return false;

    memcpy(outAddr, generated.val, 6);
    return true;
}

static inline bool bleNimblePrivateNrpaConfirmed(const uint8_t expectedAddr[6]) {
    if (expectedAddr == nullptr) return false;

    uint8_t configured[6] = {0};
    int isNrpa = 0;
    int rc = ble_hs_id_copy_addr(BLE_ADDR_RANDOM, configured, &isNrpa);
    if (rc != 0) return false;
    if (isNrpa != 1) return false;
    if (!bleNimbleNativeAddressIsNrpa(configured)) return false;
    if (memcmp(configured, expectedAddr, 6) != 0) return false;

    // BLEDevice::getAddress() resolves through Arduino-ESP32's currently
    // selected m_ownAddrType. Comparing its native bytes therefore also checks
    // that RANDOM is the selected address source, not merely that an NRPA exists.
    BLEAddress selected = BLEDevice::getAddress();
    return memcmp(selected.getNative(), expectedAddr, 6) == 0;
}

static inline bool bleNimbleApplyPrivateNrpa(const uint8_t addr[6]) {
    if (addr == nullptr || !bleNimbleNativeAddressIsNrpa(addr)) return false;

    // BLEDevice::setOwnAddr() forwards to NimBLE's ble_hs_id_set_rnd().
    uint8_t writableAddr[6];
    memcpy(writableAddr, addr, sizeof(writableAddr));
    if (!BLEDevice::setOwnAddr(writableAddr)) return false;

    // BLEScan::start() later uses this selected own-address type in ble_gap_disc().
    if (!BLEDevice::setOwnAddrType(BLE_OWN_ADDR_RANDOM)) return false;

    return bleNimblePrivateNrpaConfirmed(addr);
}

static inline bool bleNimblePublicAddressConfirmed() {
    uint8_t publicAddr[6] = {0};
    int isNrpa = 0;
    int rc = ble_hs_id_copy_addr(BLE_ADDR_PUBLIC, publicAddr, &isNrpa);
    if (rc != 0) return false;

    BLEAddress selected = BLEDevice::getAddress();
    return memcmp(selected.getNative(), publicAddr, 6) == 0;
}

static inline bool bleNimbleApplyPublicAddress() {
    if (!BLEDevice::setOwnAddrType(BLE_OWN_ADDR_PUBLIC)) return false;
    return bleNimblePublicAddressConfirmed();
}
