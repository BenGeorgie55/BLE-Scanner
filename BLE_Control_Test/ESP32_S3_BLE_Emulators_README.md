# ESP32-S3 BLE Smart-Glasses Control Emulators

These Arduino sketches turn a spare ESP32-S3 into a BLE positive-control transmitter for bench-testing the smart-glasses detector project.

Each sketch broadcasts project-specific test characteristics that should activate one detector rule. Every transmitted packet also contains a clear `CONTROL` or `CTRL` marker so test records can be separated from genuine field observations.

> [!IMPORTANT]
> These sketches emulate BLE advertisements only. They do not contain manufacturer firmware, operate a camera or microphone, pair with a glasses application, reproduce the complete behaviour of real glasses, or clone a genuine device address.

## Included emulators

| Sketch | Detector rule exercised | Logged control identifiers | Expected result |
| --- | --- | --- | --- |
| `MetaRB_BLE_Control_Test.ino` | Meta test company ID `0x058E`, `META_RB_GLASS`, service UUID `0xFD5F`, and `META_RB` name | Name `CONTROL_META_RB`; marker `META_RB_GLASS_CONTROL` | Meta Ray-Ban test alert |
| `Luxottica_BLE_Control_Test.ino` | Luxottica company ID `0x0D53` plus a name containing `ray-ban` | Name `CONTROL_RAY-BAN`; marker `CTRL` | Luxottica/Ray-Ban test alert |
| `Snap_Spectacles_BLE_Control_Test.ino` | Snap company ID `0x03C2` plus a name containing `Spectacles` | Name `CONTROL_SPECTACLES`; marker `CTRL` | Snap Spectacles test alert |

The Snap database rules currently expect a HIGH score of 90 for the company-ID and name combination, with a MEDIUM score of 85 for the name-only fallback.

## Hardware and software

- One spare ESP32-S3 board for the emulator
- A separate ESP32 detector running the firmware being tested
- A data-capable USB cable
- Arduino IDE
- Espressif `esp32` board package

The sketches use the BLE library supplied with the Espressif Arduino board package. No third-party BLE library is required. The source uses the Arduino `String` interface expected by Arduino-ESP32 3.3.11.

## Arduino IDE setup

1. Create a folder with the same name as the selected sketch, without `.ino`.
2. Place the sketch inside that folder. For example:

   `Snap_Spectacles_BLE_Control_Test/Snap_Spectacles_BLE_Control_Test.ino`

3. Open the `.ino` in Arduino IDE.
4. Install or select **esp32 by Espressif Systems** in Boards Manager.
5. Select the exact ESP32-S3 board when it is listed. Otherwise use **ESP32S3 Dev Module**.
6. Select the board's USB port.
7. Enable **USB CDC On Boot** if the board requires it for the Serial Monitor.
8. Compile and upload the sketch.
9. Open Serial Monitor at **115200 baud** if status messages are required.

If uploading waits indefinitely, hold the board's **BOOT** button, briefly press **RESET**, begin the upload, and release **BOOT** when writing starts. Button behaviour varies between ESP32-S3 boards.

## Running a controlled test

1. Flash only one emulator sketch at a time.
2. Keep the emulator separate from the detector; one ESP32 broadcasts while the other scans.
3. Start a clearly identified detector test session and record its start time.
4. Power the emulator near the detector.
5. Confirm the expected `CONTROL` name and manufacturer data appear in the advertisement log.
6. Confirm the detector produces the intended product classification or alert.
7. Stop the emulator before returning to ordinary field collection.
8. Keep control-test records out of real-world training, validation, and detection statistics.

The emulator begins advertising automatically after boot.

Serial Monitor commands:

- `s` — stop advertising
- `a` — start advertising

## Expected log evidence

| Emulator | Advertised name | Company ID | Manufacturer data or marker |
| --- | --- | --- | --- |
| Meta Ray-Ban | `CONTROL_META_RB` | `0x058E` | Contains `META_RB_GLASS_CONTROL` |
| Luxottica/Ray-Ban | `CONTROL_RAY-BAN` | `0x0D53` | `530D4354524C` (`0x0D53` + `CTRL`) |
| Snap Spectacles | `CONTROL_SPECTACLES` | `0x03C2` | `C2034354524C` (`0x03C2` + `CTRL`) |

Bluetooth company IDs are encoded little-endian in manufacturer-specific advertisement data. This is why `0x0D53` appears as `530D` and `0x03C2` appears as `C203` in a raw hexadecimal log.

For the Meta control, the manufacturer marker is in the primary advertisement. Its control name and `0xFD5F` service UUID are in the scan response. A passive-only scanner may therefore record the manufacturer marker without recording the name or service UUID. The marker still ends in `CONTROL`.

The Luxottica and Snap controls keep their company ID, matching name, and control marker in the primary advertisement, allowing passive and active scanners to record them.

## Pass criteria

A test passes when:

- the advertisement is received and logged;
- the log contains an explicit `CONTROL` or `CTRL` identifier;
- the expected detector rule activates;
- no unrelated product rule overrides the intended result; and
- stopping or powering off the emulator prevents new detections after the detector's normal cache or observation window expires.

If an advertisement is logged but no alert appears, check the recorded name, company ID, manufacturer data, RSSI, enabled confidence tiers, and whether active scanning is enabled where a scan response is required.

## Privacy and dataset handling

The emulator does not scan nearby devices or create its own log files. It broadcasts using the ESP32-S3's address; it does not copy an observed glasses address.

Detector firmware must continue to follow the project's privacy requirement: an exact observed BLE MAC address may be used temporarily in RAM, but must never be written to persistent logs. Session-scoped pseudonymous identifiers may be stored instead.

Control sessions must remain visibly labelled and must not be represented as detections of genuine glasses or people. The detector identifies radio characteristics, not a person, and every alert requires context.

## Limitations

- These are synthetic positive controls built around the project's detector rules.
- Passing proves that a configured rule and logging path work; it does not prove that every real product will advertise continuously or identically.
- Real glasses may change advertisements with firmware, operating state, pairing state, or phone connection.
- Radio range and RSSI vary with antenna, enclosure, orientation, interference, and distance.
- A test alert must not be included as evidence of a real-world device encounter.

## Current source hashes

| File | SHA-256 |
| --- | --- |
| `MetaRB_BLE_Control_Test.ino` | `22b624d7561a0b2b59b185d2cbd4caa9bb92b8fa28bb64ba911142b24f17c297` |
| `Luxottica_BLE_Control_Test.ino` | `6b4b8621dd4dd90903015e5b59c593ba255df1f470203aadfa53480a4f5af7c7` |
| `Snap_Spectacles_BLE_Control_Test.ino` | `2df28fb02192bd1a863b9206c9338236baa5bac05e5c8c23bb336390c5ea65cc` |

These hashes identify the current unvalidated source files. Recalculate them after any source change.

## Validation status

The source files target the Arduino-ESP32 3.3.11 BLE API. They remain hardware unvalidated until each sketch is compiled, flashed to the intended ESP32-S3 board, detected by the target firmware, and confirmed in the resulting logs.
