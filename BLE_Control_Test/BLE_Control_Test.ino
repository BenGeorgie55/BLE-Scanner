/*
  BLE Negative-Control Test Broadcaster
  -------------------------------------
  Purpose: confirm that the smart-glasses detector can receive an ordinary BLE
           advertisement without classifying it as smart glasses.

  The device identifies itself as "CONTROL_TEST" in the advertised name and
  carries the ASCII manufacturer marker "CONTROL". It intentionally contains
  no Meta/Ray-Ban names, company IDs, OUIs, or smart-glasses service UUIDs.

  Target: ESP32 boards supported by Arduino-ESP32, including ESP32-WROOM and
          ESP32-2432S028R/CYD.
  Tested API target: Arduino-ESP32 3.3.11 BLE library interface.

  Expected detector result:
    - Advertisement may appear in the ordinary advertisement log.
    - It should NOT generate a smart-glasses detection or alert.
    - If it alerts, the matching rule should be reviewed for false positives.

  Serial controls at 115200 baud:
    a = start advertising
    s = stop advertising
*/

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEAdvertising.h>
#include <BLEUtils.h>

namespace {

constexpr char DEVICE_NAME[] = "CONTROL_TEST";
constexpr uint16_t CONTROL_COMPANY_ID = 0xFFFF;  // Reserved test value.

BLEAdvertising* advertising = nullptr;
bool isAdvertising = false;

String buildControlManufacturerData() {
  // Company ID is little-endian, followed by the plainly readable CONTROL
  // marker. Logged hex: FFFF434F4E54524F4C
  static constexpr uint8_t payload[] = {
    static_cast<uint8_t>(CONTROL_COMPANY_ID & 0xFF),
    static_cast<uint8_t>((CONTROL_COMPANY_ID >> 8) & 0xFF),
    'C', 'O', 'N', 'T', 'R', 'O', 'L'
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
  Serial.println("CONTROL advertising started: CONTROL_TEST");
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

  // Keep the CONTROL name in the primary packet so both active and passive
  // scanners can see that this is the negative-control transmitter.
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
  Serial.println("BLE CONTROL TEST Broadcaster");
  Serial.println("Expected result: visible advertisement, no glasses alert");

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
