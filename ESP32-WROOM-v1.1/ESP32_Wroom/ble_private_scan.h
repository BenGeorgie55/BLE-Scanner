#pragma once

/*
 * ESP32 Wroom — Bluedroid PRIVATE NRPA adapter
 * Arduino-ESP32 3.3.11, classic ESP32 / ESP32-WROOM / ESP32 Dev Module.
 */
#include <Arduino.h>
#include <BLEScan.h>
#include <esp_gap_ble_api.h>

#if !defined(CONFIG_BLUEDROID_ENABLED)
#error "ESP32 Wroom requires the Arduino-ESP32 Bluedroid BLE backend. Select a classic ESP32 / ESP32 Dev Module board using ESP32 by Espressif Systems 3.3.11."
#endif

#define BLE_PRIVATE_ADDR_ROTATE_MIN_MS  (12UL * 60UL * 1000UL)
#define BLE_PRIVATE_ADDR_ROTATE_MAX_MS  (18UL * 60UL * 1000UL)
#define BLE_PRIVATE_ADDR_CONFIRM_MS     1000UL

#if BLE_PRIVATE_ADDR_ROTATE_MAX_MS <= BLE_PRIVATE_ADDR_ROTATE_MIN_MS
#error "BLE private-address maximum rotation interval must exceed the minimum"
#endif

template <typename Tag, typename Tag::type Member>
struct BlePrivateMemberAccessor {
    friend typename Tag::type getBlePrivateMember(Tag) { return Member; }
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

static inline esp_err_t applyBleScannerOwnAddressType(BLEScan* scan,
                                                       esp_ble_addr_type_t type) {
    esp_ble_scan_params_t* params = bleScannerScanParams(scan);
    if (params == nullptr) return ESP_ERR_INVALID_ARG;
    params->own_addr_type = type;
    return esp_ble_gap_set_scan_params(params);
}
static inline esp_err_t applyBleScannerOwnAddressRandom(BLEScan* scan) {
    return applyBleScannerOwnAddressType(scan, BLE_ADDR_TYPE_RANDOM);
}
static inline esp_err_t applyBleScannerOwnAddressPublic(BLEScan* scan) {
    return applyBleScannerOwnAddressType(scan, BLE_ADDR_TYPE_PUBLIC);
}
