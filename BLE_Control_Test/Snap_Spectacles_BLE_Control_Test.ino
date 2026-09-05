/*
  Snap Spectacles BLE Positive-Control Emulator
  ---------------------------------------------
  Purpose: trigger the detector's Snap Spectacles BLE rules while ensuring
           every logged record is visibly marked as CONTROL test data.

  This is an advertisement-level test emulator. It does not reproduce genuine
  Snap firmware, pair with the Spectacles app, provide camera/audio functions,
  or clone a real device address.

  Control identifiers written into the primary BLE advertisement:
    - Advertised name: CONTROL_SPECTACLES
    - Manufacturer marker: CTRL

  Detector signatures exercised:
    - Snap/Snapchat company ID 0x03C2
    - Advertised-name substring Spectacles

  Expected database matches:
    - HIGH score 90: Snap Company ID + Spectacles name
    - MEDIUM score 85 fallback: Spectacles advertised-name match

  Target: ESP32 boards supported by Arduino-ESP32, including ESP32-WROOM and
          ESP32-2432S028R/CYD.
  Tested API target: Arduino-ESP32 3.3.11 BLE library interface.

  Serial controls at 115200 baud:
    a = start advertising
    s = stop advertising
*/

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEAdvertising.h>
#include <BLEUtils.h>

namespace {

constexpr char DEVICE_NAME[] = "CONTROL_SPECTACLES";
constexpr uint16_t SNAP_COMPANY_ID = 0x03C2;

BLEAdvertising* advertising = nullptr;
bool isAdvertising = false;

String buildControlManufacturerData() {
  // Company ID is little-endian, followed by the plainly readable CTRL marker.
  // Logged hex: C2034354524C
  static constexpr uint8_t payload[] = {
    static_cast<uint8_t>(SNAP_COMPANY_ID & 0xFF),
    static_cast<uint8_t>((SNAP_COMPANY_ID >> 8) & 0xFF),
    'C', 'T', 'R', 'L'
  };

  String manufacturerData;
  manufacturerData.reserve(sizeof(payload));

  for (const uint8_t byte : payload) {
    manufacturerData += static_cast<char>(byte);
  }

  return manufacturerData;
}

void startAdvertising() {
  if (isAdvertising) {
    return;
  }

  advertising->start();
  isAdvertising = true;
  Serial.println("Snap CONTROL advertising started: CONTROL_SPECTACLES");
}

void stopAdvertising() {
  if (!isAdvertising) {
    return;
  }

  advertising->stop();
  isAdvertising = false;
  Serial.println("CONTROL advertising stopped");
}

void configureAdvertising() {
  BLEAdvertisementData primaryData;

  // General discoverable + Bluetooth Low Energy only.
  primaryData.setFlags(0x06);

  // Flags, name, and manufacturer data occupy exactly the available 31 bytes.
  // Both active and passive scanners can therefore log the Snap match and the
  // CONTROL labels without relying on a scan response.
  primaryData.setName(DEVICE_NAME);
  primaryData.setManufacturerData(buildControlManufacturerData());

  advertising = BLEDevice::getAdvertising();
  advertising->setAdvertisementData(primaryData);
  advertising->setScanResponse(false);

  // 0.625 ms units: 160 = 100 ms, 240 = 150 ms.
  advertising->setMinInterval(160);
  advertising->setMaxInterval(240);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println("Snap Spectacles BLE CONTROL TEST Emulator");
  Serial.println("Expected result: Snap alert clearly labelled CONTROL");

  BLEDevice::init(DEVICE_NAME);
  configureAdvertising();
  startAdvertising();
}

void loop() {
  if (Serial.available() > 0) {
    const char command = static_cast<char>(Serial.read());

    if (command == 'a' || command == 'A') {
      startAdvertising();
    } else if (command == 's' || command == 'S') {
      stopAdvertising();
    }
  }

  delay(20);
}
