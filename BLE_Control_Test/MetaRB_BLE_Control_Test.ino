/*
  Meta Ray-Ban Positive-Control Test Broadcaster
  ----------------------------------------------
  Purpose: trigger the smart-glasses detector's Meta Ray-Ban rules while making
           every resulting test record clearly identifiable as CONTROL data.

  This is an advertisement-level test emulator. It does not reproduce Meta
  firmware, pair with the Meta app, provide camera/audio functions, or clone a
  real glasses address.

  Control identifiers written into the BLE packet:
    - Advertised name: CONTROL_META_RB
    - Manufacturer marker: META_RB_GLASS_CONTROL

  Detector signatures exercised:
    - Company ID 0x058E
    - Manufacturer substring META_RB_GLASS
    - Service UUID 0xFD5F
    - Advertised-name substring META_RB

  Target: ESP32 boards supported by Arduino-ESP32, including ESP32-WROOM and
          ESP32-2432S028R/CYD.
  Tested API target: Arduino-ESP32 3.3.11 BLE library interface.

  Expected detector result:
    - A Meta Ray-Ban test detection/alert should be generated.
    - Advertisement and detection logs should contain CONTROL in the name
      and/or manufacturer marker so the record cannot be mistaken for a real
      field detection.

  Serial controls at 115200 baud:
    a = start advertising
    s = stop advertising
*/

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEAdvertising.h>
#include <BLEUtils.h>

namespace {

constexpr char DEVICE_NAME[] = "CONTROL_META_RB";
constexpr uint16_t META_TEST_COMPANY_ID = 0x058E;
constexpr uint16_t META_TEST_SERVICE_UUID = 0xFD5F;

BLEAdvertising* advertising = nullptr;
bool isAdvertising = false;

String buildMetaControlManufacturerData() {
  // Company ID is little-endian. The marker begins with the detector's
  // META_RB_GLASS signature and ends with CONTROL for unambiguous test logs.
  static constexpr uint8_t payload[] = {
    static_cast<uint8_t>(META_TEST_COMPANY_ID & 0xFF),
    static_cast<uint8_t>((META_TEST_COMPANY_ID >> 8) & 0xFF),
    'M', 'E', 'T', 'A', '_', 'R', 'B', '_', 'G', 'L', 'A', 'S', 'S',
    '_', 'C', 'O', 'N', 'T', 'R', 'O', 'L'
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
  Serial.println("Meta Ray-Ban CONTROL advertising started: CONTROL_META_RB");
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
  BLEAdvertisementData scanResponseData;

  // General discoverable + Bluetooth Low Energy only.
  primaryData.setFlags(0x06);

  // Keep both the Meta detector marker and CONTROL label in the primary packet.
  // With the flags, this occupies 28 of the 31 legacy-advertisement bytes.
  primaryData.setManufacturerData(buildMetaControlManufacturerData());

  // The scan response has room for the explicit control name and Meta service.
  scanResponseData.setName(DEVICE_NAME);
  scanResponseData.setCompleteServices(BLEUUID(META_TEST_SERVICE_UUID));

  advertising = BLEDevice::getAdvertising();
  advertising->setAdvertisementData(primaryData);
  advertising->setScanResponseData(scanResponseData);
  advertising->setScanResponse(true);

  // 0.625 ms units: 160 = 100 ms, 240 = 150 ms.
  advertising->setMinInterval(160);
  advertising->setMaxInterval(240);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println("Meta Ray-Ban CONTROL TEST Broadcaster");
  Serial.println("Expected result: Meta alert clearly labelled CONTROL");

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
