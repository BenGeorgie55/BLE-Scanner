#pragma once
#include <stdint.h>

/*
 * ========================= USER-EDITABLE REFERENCE TABLE =========================
 * v6.2 Session 04 + Session 06 field-data refinements, August 2026.
 *
 * These tables are ANNOTATION / REFERENCE DATA ONLY.
 * They do NOT create a smart-glasses match, do NOT change confidence, and do NOT
 * create an alert. Direct HIGH/MEDIUM/LOW confidence rules remain in their existing
 * confidence headers and are evaluated independently.
 *
 * Company IDs are Bluetooth SIG Company Identifiers.
 * Service UUID rows are context labels: an assigned UUID owner/service does NOT prove
 * that the physical device was manufactured by that organisation.
 * Keep each final sentinel LAST.
 * ================================================================================
 */

struct BleCompanyReference {
    uint16_t companyId;
    const char* companyName;
};

static const BleCompanyReference BLE_COMPANY_REFERENCES[] = {
    { 0x0006, "Microsoft" },
    { 0x0057, "Harman International Industries, Inc." },
    { 0x0065, "HP, Inc." },
    { 0x050C, "OSRAM GmbH" },
    { 0x0B19, "WBS PROJECT H PTY LTD" },
    { 0xFFFF, nullptr }
};

struct BleServiceReference {
    uint16_t uuid16;
    const char* displayName;
    const char* contextType;
};

static const BleServiceReference BLE_SERVICE_REFERENCES[] = {
    { 0xFEAA, "Google Eddystone service UUID", "Google / Eddystone beacon service" },
    { 0xFE03, "Amazon.com Services, Inc. assigned UUID", "Amazon service context" },
    { 0xFE60, "Lierda Science & Technology Group assigned UUID", "Lierda service context" },
    { 0xFE78, "Hewlett-Packard Company assigned UUID", "HP service context" },
    { 0xFD2A, "Sony Corporation assigned UUID", "Sony service context" },
    { 0xFE9F, "Google LLC assigned UUID", "Google service context" },
    { 0xFEA0, "Google LLC assigned UUID", "Google service context" },
    { 0x180A, "Device Information service", "Standard GATT service" },
    { 0x1122, "BasicPrinting profile", "Bluetooth profile context" },
    { 0x0000, nullptr, nullptr }
};
