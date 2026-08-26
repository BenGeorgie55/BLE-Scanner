# ESP32-WROOM v1.1 — NeoPixel

**Status:** WORKING  
**Target:** classic ESP32-WROOM / ESP32 Dev Module  
**Use:** lowest-cost portable/travel build

## Important

This public branch is **NeoPixel only**. It does **not** use an OLED.

## Build settings

- Arduino-ESP32: 3.3.11
- BLE backend: Bluedroid
- Adafruit NeoPixel required
- NeoPixel data: GPIO 4
- Serial Monitor: 115200 baud
- Upload: 115200 baud
- Flash Mode: DIO where applicable
- **No custom `partitions.csv`**
- Tools -> Partition Scheme: leave the normal/default ESP32 Dev Module scheme selected
- LittleFS logging is available only where the selected normal layout provides a compatible filesystem area; BLE scanning/alerts remain operational if LittleFS cannot mount
- OTA workflow: not used

## Privacy/storage

- Current source includes LittleFS candidate/session logging and `LOG`.
- Raw observed BLE MAC addresses are not intentionally persisted.
- Retained correlation is session-scoped/pseudonymous.
