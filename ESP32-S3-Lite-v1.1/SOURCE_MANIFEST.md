# Source Manifest / Verification Notes

Current source lineage identifies:
- `ESP32_S3_Glasses_Scanner_Lite_v1_1.ino`
- `status_led.h`
- `ble_private_scan.h`
- `lite_counters.h`
- `lite_persistent_log.h`
- rule/database headers
- `camera_audio_matches.h`
- `partitions.csv`
- validation/source-lineage/readme/legal files as supplied

Verified source facts used by this wrapper:
- active BLE scan ~2 s every ~5 s;
- private address rotation 12–18 minutes;
- 3 private startup failures can lead to public fallback;
- >=40 configured logging threshold;
- raw observed BLE MACs are not intentionally persisted;
- session salt is RAM-only;
- custom 4 MB+ no-OTA partition table provides a large LittleFS region.
