# Source Manifest / Verification Notes

Current public branch:
- `ESP32_WROOM_Glasses_Scanner_Lite_v1_1_NeoPixel.ino`
- `hardware_config.h` confirms one WS2812/NeoPixel on GPIO 4
- no OLED
- Bluedroid private-address adapter
- LittleFS candidate/session logging
- `LOG` command
- current rule/database headers

Do not substitute the older WROOM OLED branch.

Partition verification:
- package contains no custom `partitions.csv`;
- use normal/default ESP32 Dev Module partition scheme;
- LittleFS failure is non-fatal to BLE scanning and NeoPixel alerts.
