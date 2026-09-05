/*
  Luxottica / Ray-Ban BLE Positive-Control Emulator
  ------------------------------------------------
  Purpose: trigger the detector's Luxottica + Ray-Ban BLE rule while ensuring
           every logged record is visibly marked as CONTROL test data.

  This is an advertisement-level test emulator. It does not reproduce genuine
  Luxottica or Ray-Ban firmware, pair with an app, provide camera/audio
  functions, or clone a real device address.

  Control identifiers written into the primary BLE advertisement:
    - Advertised name: CONTROL_RAY-BAN
    - Manufacturer marker: CTRL

  Detector signatures exercised:
    - Luxottica company ID 0x0D53
    - Advertised-name substring ray-ban

  Target: ESP32 boards supported by Arduino-ESP32, including ESP32-WROOM and
          ESP32-2432S028R/CYD.
  Tested API target: Arduino-ESP32 3.3.11 BLE library interface.

  Expected detector result:
    - A Luxottica/Ray-Ban test detection or alert should be generated.
    - Logs should contain CONTROL_RAY-BAN and manufacturer marker CTRL.

  Serial controls at 115200 baud:
    a = start advertising
    s = stop advertising
*/

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEAdvertising.h>
#include <BLEUtils.h>

namespace {

constexpr char DEVICE_NAME[] = "CONTROL_RAY-BAN";
constexpr uint16_t LUXOTTICA_COMPANY_ID = 0x0D53;

BLEAdvertising* advertising = nullptr;
bool isAdvertising = false;

String buildControlManufacturerData() {
  // Company ID is little-endian, followed by the plainly readable CTRL marker.
  // Logged hex: 530D4354524C
  static constexpr uint8_t payload[] = {
    static_cast<uint8_t>(LUXOTTICA_COMPANY_ID & 0xFF),
    static_cast<uint8_t>((LUXOTTICA_COMPANY_ID >> 8) & 0xFF),
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
  Serial.println("Luxottica CONTROL advertising started: CONTROL_RAY-BAN");
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

  // Name and manufacturer data total 28 bytes including BLE field headers and
  // flags, so both the Ray-Ban match and CONTROL labels stay in the primary
  // 31-byte legacy advertisement. Passive and active scanners can log them.
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
  Serial.println("Luxottica / Ray-Ban BLE CONTROL TEST Emulator");
  Serial.println("Expected result: Luxottica alert clearly labelled CONTROL");

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
