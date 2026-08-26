# ESP32-S3 Lite v1.1

**Status:** WORKING  
**Target:** ESP32-S3  
**Use:** easiest portable/travel build

## Build settings

- Arduino-ESP32: 3.3.11
- BLE backend: NimBLE
- Serial Monitor: 115200 baud
- Upload: 921600 where reliable
- Fallback upload: 460800
- OTA workflow: not used
- Keep the supplied sketch-local `partitions.csv`
- Supplied no-OTA table: app `0x1C0000` (~1.75 MiB), filesystem `0x230000` (~2.19 MiB)
- First v1.1 installation: erase-all-flash may be required by the supplied validation checklist

## Privacy/storage

- LittleFS retains qualifying candidate/session history.
- Raw observed BLE MAC addresses are not intentionally persisted.
- Device correlation is session-scoped/pseudonymous.
- `LOG` retrieves retained field-test history.
