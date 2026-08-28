#pragma once

/*
 * ============================================================================
 * CYD Dev Expanded 1 — PRIVATE BLE SCANNER ADDRESS ADAPTER
 * ============================================================================
 *
 * Target stack:
 *   Arduino-ESP32 3.3.11
 *   ESP-IDF 5.5.5
 *   Bluedroid BLE backend
 *
 * v8 retains the validated v6.3 Non-Resolvable Private Address (NRPA) path as the CYD scanner's own
 * address for ACTIVE scan requests. The address rotates at a fresh random
 * interval between 12 and 18 minutes and is changed only while scanning is idle.
 *
 * Arduino-ESP32 BLEScan keeps esp_ble_scan_params_t private and initializes
 * own_addr_type to BLE_ADDR_TYPE_PUBLIC. BLEScan::start() re-applies those
 * private parameters before every scan, so setting GAP scan parameters once
 * outside BLEScan is insufficient. This compile-time accessor changes only
 * BLEScan::m_scan_params.own_addr_type to BLE_ADDR_TYPE_RANDOM while preserving
 * the library's scan type, interval, window, filter and duplicate settings.
 *
 * If Arduino-ESP32 changes the BLEScan private layout in a future release, this
 * header is intended to fail compilation rather than silently lose RF privacy.
 * ============================================================================
 */

#if !defined(CONFIG_BLUEDROID_ENABLED)
#error "CYD Dev Expanded 1 PRIVATE NRPA scanner-address support requires the Arduino-ESP32 Bluedroid BLE backend"
#endif

#include <esp_gap_ble_api.h>

#define BLE_PRIVATE_ADDR_ROTATE_MIN_MS  (12UL * 60UL * 1000UL)
#define BLE_PRIVATE_ADDR_ROTATE_MAX_MS  (18UL * 60UL * 1000UL)
#define BLE_PRIVATE_ADDR_CONFIRM_MS     1000UL
#define BLE_PRIVATE_ADDR_RETRY_MS      60000UL

#if BLE_PRIVATE_ADDR_ROTATE_MAX_MS <= BLE_PRIVATE_ADDR_ROTATE_MIN_MS
#error "BLE private-address maximum rotation interval must exceed the minimum"
#endif

template <typename Tag, typename Tag::type Member>
struct BlePrivateMemberAccessor {
    friend typename Tag::type getBlePrivateMember(Tag) {
        return Member;
    }
};

struct BleScanParamsMemberTag {
    using type = esp_ble_scan_params_t BLEScan::*;
    friend type getBlePrivateMember(BleScanParamsMemberTag);
};

template struct BlePrivateMemberAccessor<BleScanParamsMemberTag, &BLEScan::m_scan_params>;

static inline esp_ble_scan_params_t* bleScannerScanParams(BLEScan* scan) {
    if (scan == nullptr) return nullptr;
    auto member = getBlePrivateMember(BleScanParamsMemberTag{});
    return &(scan->*member);
}

static inline void forceBleScannerOwnAddressRandom(BLEScan* scan) {
    esp_ble_scan_params_t* params = bleScannerScanParams(scan);
    if (params == nullptr) return;
    params->own_addr_type = BLE_ADDR_TYPE_RANDOM;
}

/*
 * IMPORTANT ORDERING NOTE — Arduino-ESP32 3.3.11 / ESP-IDF 5.5.5
 * --------------------------------------------------------------------------
 * esp_ble_gap_set_rand_addr() installs the random address, but Bluedroid's
 * current own-address type remains PUBLIC until scan/advertising parameters
 * that request BLE_ADDR_TYPE_RANDOM are processed. Therefore the previous v6.2
 * sequence (set random address -> change only BLEScan's private member -> call
 * esp_ble_gap_get_local_used_addr()) could fail closed before any scan began.
 *
 * This helper both keeps BLEScan::start() configured for RANDOM and immediately
 * applies the same persistent BLEScan parameter structure to GAP. The caller can
 * then poll esp_ble_gap_get_local_used_addr() and verify the address/type before
 * allowing the first active scan. BLEScan::start() will re-apply these same
 * parameters on every later scan.
 */
static inline esp_err_t applyBleScannerOwnAddressRandom(BLEScan* scan) {
    esp_ble_scan_params_t* params = bleScannerScanParams(scan);
    if (params == nullptr) return ESP_ERR_INVALID_ARG;
    params->own_addr_type = BLE_ADDR_TYPE_RANDOM;
    return esp_ble_gap_set_scan_params(params);
}
