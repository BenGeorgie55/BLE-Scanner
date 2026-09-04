# BLE-Scanner
Open-source ESP32 privacy-awareness detectors for smart glasses, BLE cameras, microphones and recording devices, with portable and facility-focused builds.

Disclaimer- SOME AI WAS USED IN THE BUILDING OF THESE DEVICES

## Dedication

This project is dedicated to my sister, my mother, E.M., her sister M.B/M., C.C and R.H

## Inspiration and Related Projects

I credit the following projects for inspiration:

- `surveillancewatch/ESP-GlassHole` — ESP32 BLE smart-glasses detector project
- `sh4d0wm45k/glass-detect` — Smart-glasses detection project
- `colonelpanichacks/flock-you` — ESP32 surveillance-infrastructure detection project
- `yjeanrenaud/yj_nearbyglasses` — Nearby Glasses project
- `NullPxl/banrays` — Smart-glasses / anti-recording awareness project
- `colonelpanichacks/ouispy-detector` — OUI/BLE-oriented detector project
- `LuxStatera/flock-hunter-cyd-wifi` — CYD/Wi-Fi detector project
- `haxorthematrix/BLEPTD` — BLE detection/scanning project
- `RamboRogers/esp32-bluetooth-scanner` — General ESP32 Bluetooth scanner project

## Database / Reference Sources

I used the following databases and reference sources during development and research:

- Nordic Semiconductor BLE Database
- Bluetooth SIG BLE database
- IEEE data
- Spectacle Database
- BLE-Payloads lists
- Fingerbank data
- Sparrow data
- My own region specific and environmental specific (Sydney -dense populus area) database, consisting of 4000000 SIGs with more to be classified in the future

** It is possible to modify the onboard database for your own purposes but you may cross legal lines or cross into grey areas. Please Please research, get advice, speak to a lawyer in your specific country, state or region to be aware of the laws. I can only suggest that you only use these devices in a lawful manner and only everr in a test environment**

**This project is likely to go mostly unsupported in the future, please read carefully.**

# ESP32 Smart-Glasses, Camera, Microphone & BLE Privacy Detector

Open-source ESP32 privacy-awareness detectors for smart glasses, BLE cameras, microphones and recording devices, with portable and facility-focused builds.

> **This project detects radio characteristics, not people. Its purpose is privacy awareness, not surveillance.**

> **No detector can establish whether a nearby camera or microphone is currently recording. Treat alerts as information requiring context, not proof of wrongdoing.**

**Disclaimer:** Some AI was used during development and documentation of these devices.

> **Support notice:** This project is likely to become mostly unsupported or only intermittently maintained in the future. Please read the documentation carefully, keep local copies of firmware and build notes, and be prepared to self-maintain your chosen build.

---

## Privacy and Legal — Quick Summary

### Privacy

This project is designed around local processing and limited retention of observed-device identity.

- No companion phone application is required.
- BLE detection does not require cloud processing.
- Exact observed BLE MAC addresses are not intentionally persisted in operational logs.
- Where correlation is needed, firmware uses session-scoped pseudonymous hashes.
- The same observed BLE address should normally receive a different pseudonym in a later session.
- A detection describes radio characteristics, not a person.

> **Full privacy details are provided near the end of this README.**

### Legal and Responsible Use

This project is a privacy-awareness tool, not proof of surveillance or wrongdoing.

A detection means that a nearby BLE advertisement matched one or more configured characteristics or rules.

It does not establish:

- Who owns the device
- Whether recording is occurring
- Whether a camera or microphone is active
- Whether the device is being used unlawfully
- The intent of any nearby person

Radio, privacy, workplace, surveillance and facility rules vary by jurisdiction and deployment.

> **Users are responsible for ensuring their use is lawful and authorised. Full legal and responsible-use information is provided near the end of this README.**

---

## Which Version Should I Build?

> ### Recommended for Most Users: ESP32-S3 Lite
>
> **If you are unsure which version to build, build the ESP32-S3 Lite.**
>
> It is the simplest, most straightforward and most conservative portable build in this project.
>
> It requires no normal soldering or assembly, uses an onboard RGB status LED, can be powered directly from a compatible phone over USB-C, and avoids the additional display, SD-card, external-antenna and multi-board complexity of the CYD builds.

| Build | Recommendation | Skill Level | Intended Use |
|---|---|---|---|
| **ESP32-S3 Lite v1.1** | **Recommended for most users** | Very little practical skill | Everyday portable / travel detector |
| **ESP32-WROOM v1.1** | Budget portable alternative | Basic soldering | Low-cost portable / travel detector |
| **CYD DEV Lite** | Fixed / facility option | Moderate | Facilities, SD logging, Sentry and research |
| **CYD Expanded DEV** | Advanced development platform | Advanced | Stationary research, database development and experimental work |

### Why Start With the ESP32-S3 Lite?

The S3 deliberately keeps the portable detector simple.

There is:

- No external status LED to solder
- No display to configure
- No SD card required for normal operation
- No external antenna modification
- No second ESP32
- No UART link between boards
- No database coprocessor
- No CYD touchscreen/display configuration

For most people, the S3 is the logical starting point.

---

# Build 1 — ESP32-S3 Lite

**STATUS: WORKING / HARDWARE VALIDATED**

**RECOMMENDATION: DEFAULT CHOICE FOR MOST USERS**

**DIFFICULTY: EASIEST**

**INTENDED USE: EVERYDAY PORTABLE / TRAVEL**

The ESP32-S3 Lite is the recommended general-purpose portable detector.

It provides the core purpose of the project without the additional hardware complexity of the WROOM or CYD versions.

If you simply want a small, straightforward detector that can be powered from your phone and carried with you, **start here**.

## ESP32-S3 Lite — Portable Setup

<table>
  <tr>
    <td align="center" width="33%">
      <img src="images/esp32-s3/esp32-s3-phone-mounted.jpg" width="100%"><br>
      <b>Phone-mounted portable setup</b><br>
      ESP32-S3 Lite mounted to the rear of a phone and powered by USB-C.
    </td>
    <td align="center" width="33%">
      <img src="images/esp32-s3/esp32-s3-portable-hardware.jpg" width="100%"><br>
      <b>ESP32-S3 Lite hardware</b><br>
      Compact detector board shown alongside the phone.
    </td>
    <td align="center" width="33%">
      <img src="images/esp32-s3/esp32-s3-phone-powered.jpg" width="100%"><br>
      <b>USB-C phone-powered operation</b><br>
      ESP32-S3 Lite operating from a short USB-C connection.
    </td>
  </tr>
</table>

### Parts

- 1 × ESP32-S3 development board

### Optional Hardware

The yellow pin-header strips commonly supplied with ESP32-S3 development boards are **optional**.

They are not required for normal detector operation.

### Assembly Tools Required

**None.**

No soldering, wire cutting or other assembly work is normally required.

### Programming Requirements

- Computer
- Arduino IDE
- Arduino-ESP32 3.3.11
- USB data cable
- Correct ESP32-S3 board profile
- Serial Monitor: 115200 baud
- Upload speed: 921600 baud where reliable
- Fallback upload speed: 460800 baud
- OTA updating: not used
- Partitioning: use the supplied sketch-local `partitions.csv`

A charging-only USB cable will not work for programming.

### Portable Phone-Powered Design

The S3 is designed to be mounted behind a compatible smartphone and powered using a short USB-C-to-USB-C cable of approximately 10 cm.

Practical design estimate:

**Approximately 20% additional phone battery use over an 8-hour day.**

This is an estimate, not a guaranteed specification.

Actual battery use will vary according to:

- Phone model
- Battery condition
- USB behaviour
- Exact S3 board
- LED activity
- BLE environment
- Scanning conditions

### ESP32-S3 LED States

#### Blue — Boot

The detector is starting.

#### Green — Normal Function

Normal private BLE scanning is operating.

Recommended action:

Continue normally.

Green means the detector is functioning normally and no orange or red alert condition has been reached.

Green does not prove that the surrounding area is free from cameras, microphones, smart glasses or recording equipment.

#### Orange — Be Aware

A BLE advertisement has matched a configured possible-device rule.

Recommended action:

Be aware that a device matching one or more relevant BLE characteristics may be nearby.

Orange is a reason for awareness, not alarm.

It does not establish:

- Exact device identity
- Exact distance
- Ownership
- Whether a camera is active
- Whether a microphone is active
- Whether recording is occurring

#### Red — Strong Alert

A BLE advertisement has reached a stronger smart-glasses or camera/audio classification.

Recommended action:

Increase awareness of the surrounding environment.

Red does not provide an exact distance measurement.

BLE range varies according to:

- Transmit power
- Antenna design
- Walls
- Human-body attenuation
- RF interference
- Device orientation
- Local environment

Red still does not prove:

- Exact distance
- Ownership
- Camera activity
- Microphone activity
- Recording
- Concealment
- Malicious intent
- Unlawful behaviour

#### Flashing Purple — Privacy Setup Failure / Retry

Private-address setup has failed and the detector is retrying.

#### Solid Purple — Public-Address Fallback

After repeated private-address failures, the firmware may enter a clearly indicated public-address fallback mode.

The user should never be led to believe private mode is still operating when it is not.

### Dumping Saved S3 Logs

The ESP32-S3 Lite includes a persistent local log that can be dumped over USB for testing, signature research, false-positive analysis and database development.

#### What You Need

- ESP32-S3 Lite
- USB data cable
- Computer
- Arduino IDE

> A charging-only USB cable will not work. The cable must support data.

#### Dump the Log

1. Connect the ESP32-S3 Lite to the computer using USB.
2. Open **Arduino IDE**.
3. Select the correct port under **Tools → Port**.
4. Open **Tools → Serial Monitor**.
5. Set the Serial Monitor to **115200 baud**.
6. Allow the detector to finish starting.
7. Type:

```text
LOG
```

8. Press **Enter / Send**.
9. The ESP32-S3 Lite will print its stored log to the Serial Monitor.

#### Saving the Log

After the dump has finished:

1. Select the complete output.
2. Copy it.
3. Paste it into a text file.
4. Save it with a useful filename, for example:

```text
S3_LOG_2026-09-02.txt
```

#### What the Log Can Contain

Depending on what has been observed, the log may contain:

- Session information
- Detection confidence
- Classification
- Alert status
- Advertised device name
- Known-device annotation
- Company ID
- Manufacturer-data information
- Relevant service UUID/signature information
- RSSI information
- Session-scoped pseudonymous device hash
- Reset/crash history

The S3 persistent logger records relevant observations at approximately **40% confidence or greater** and deduplicates repeated observations.

#### Raw BLE MAC Address Hashing

The ESP32-S3 Lite does **not intentionally store exact observed BLE MAC addresses in persistent logs**.

A raw BLE MAC address may be used temporarily in RAM for:

- Rule matching
- Deduplication
- OUI/manufacturer lookup
- Generating the session identifier

Before an observed device is written to the persistent log, the raw address is replaced with a **session-scoped pseudonymous hash**.

Example:

```text
Observed temporarily in RAM:
AA:BB:CC:12:34:56

Stored in the log:
MAC-HASH-6351BFCAFC92BE83
```

The hash uses a session-specific random salt that is not intended to be persisted.

This means the same BLE device should normally receive a different stored hash after a restart or new session.

```text
SESSION 1
AA:BB:CC:12:34:56
        |
        v
MAC-HASH-6351BFCAFC92BE83

SESSION 2
AA:BB:CC:12:34:56
        |
        v
MAC-HASH-91A62F834D0E7721
```

This allows observations to be correlated during a session without deliberately creating a permanent cross-session identity for a nearby BLE device.

> **The stored hash is a session pseudonym, not a permanent device identifier.**

Other BLE fields such as advertised names, manufacturer data, Company IDs and service UUIDs may still be distinctive, so hashing the MAC address is a privacy measure rather than a guarantee of complete anonymity.

#### If `LOG` Does Not Work

Check that:

- Serial Monitor is set to **115200 baud**
- The correct ESP32-S3 port is selected
- The USB cable supports data
- The command is typed exactly as:

```text
LOG
```

- No other program is using the serial port

Dumping the log with `LOG` does **not** erase the stored history.

### ESP32-S3 Data Flow

```text
            NEARBY BLE DEVICE
                   |
                   v
        +-----------------------+
        | BLE ADVERTISEMENT     |
        |                       |
        | name                  |
        | Company ID            |
        | manufacturer data     |
        | service UUIDs         |
        +-----------+-----------+
                    |
                    v
        +-----------------------+
        | ESP32-S3 BLE SCANNER  |
        | PRIVATE ADDRESS MODE  |
        +-----------+-----------+
                    |
                    v
        +-----------------------+
        | DETECTION ENGINE      |
        |                       |
        | smart-glasses rules   |
        | camera/audio rules    |
        | known devices         |
        | false-positive rules  |
        +-----------+-----------+
                    |
              +-----+------+
              |            |
              v            v
      +---------------+  +----------------------+
      | LED ALERT     |  | PRIVACY PROCESS      |
      |               |  |                      |
      | green         |  | observed raw MAC     |
      | orange        |  | transient in RAM     |
      | red           |  |        |             |
      | purple        |  |        v             |
      +---------------+  | session-scoped hash  |
                         +----------+-----------+
                                    |
                                    v
                         +----------------------+
                         | SESSION OBSERVATION  |
                         | RECORD               |
                         |                      |
                         | pseudonymous         |
                         | no raw BLE MAC       |
                         | no persistent device |
                         | identity             |
                         +----------------------+
```

---

# Build 2 — ESP32-WROOM

**STATUS: WORKING / HARDWARE VALIDATED**

**DIFFICULTY: MODERATE**

**INTENDED USE: DAILY PORTABLE / TRAVEL**

The WROOM is the lower-cost portable alternative.

It uses inexpensive and widely available classic ESP32 hardware, but requires more practical skill than the S3 because the status LED must be wired and soldered.

### Parts

- 1 × ESP32-WROOM development board
- 1 × WS2812B / NeoPixel LED
- Approximately 30 cm of wire
- Solder
- Flux

### Optional Hardware

The yellow pin-header strips commonly supplied with ESP32-WROOM development boards are **optional**.

They are not required for normal detector operation.

### Tools

- Soldering iron
- Wire cutters
- Wire strippers
- Small screwdriver set
- Tweezers
- Multimeter
- Heat-shrink or electrical tape if required
- Helping hands / PCB holder if required

### Programming Requirements

- Computer
- Arduino IDE
- Arduino-ESP32 3.3.11
- USB data cable
- Board: ESP32 Dev Module
- Upload speed: 115200 baud
- Serial Monitor: 115200 baud
- BLE backend: Bluedroid
- Flash Mode: DIO where applicable
- OTA updating: not used
- Partition Scheme: normal/default ESP32 Dev Module scheme
- No custom `partitions.csv`
- External library: Adafruit NeoPixel

### Important Hardware Note

The current WROOM v1.1 is:

**WS2812B / NeoPixel only.**

It does not use an OLED display.

NeoPixel data pin:

```text
GPIO 4
```

### Portable Phone-Powered Design

The WROOM is also designed for daily portable use behind a compatible smartphone.

Practical design estimate:

**Approximately 20% additional phone battery use over an 8-hour day.**

Actual consumption varies between hardware and phones.

### WROOM LED Guidance

| LED | Meaning | Recommended Response |
|---|---|---|
| **Blue** | Boot/startup | Wait for startup |
| **Green** | Normal private scanning | Continue normally |
| **Orange** | Possible relevant device profile | Be aware; a matching device may be nearby |
| **Red** | Strong/high-confidence match | Increase awareness; a strongly matching device is likely nearby |
| **Purple** | Scanner privacy-address issue | Check privacy status |

LED alerts provide radio-awareness information.

They do not prove recording, ownership, intent, illegality or exact physical distance.

### ESP32-WROOM Data Flow

```text
            NEARBY BLE DEVICE
                   |
                   v
        +-----------------------+
        | BLE ADVERTISEMENT     |
        +-----------+-----------+
                    |
                    v
        +-----------------------+
        | ESP32-WROOM SCANNER   |
        +-----------+-----------+
                    |
                    v
        +-----------------------+
        | RULE DATABASES        |
        |                       |
        | smart glasses         |
        | camera/audio          |
        | known devices         |
        | false positives       |
        +-----------+-----------+
                    |
              +-----+------+
              |            |
              v            v
     +----------------+  +---------------------+
     | WS2812B ALERT  |  | PRIVACY / HASHING   |
     |                |  |                     |
     | blue           |  | raw BLE MAC used    |
     | green          |  | transiently only    |
     | orange         |  |         |           |
     | red            |  |         v           |
     | purple         |  | session-scoped hash |
     +----------------+  +----------+----------+
                                   |
                                   v
                         +----------------------+
                         | SESSION OBSERVATION  |
                         | RECORD               |
                         |                      |
                         | no raw BLE MAC       |
                         | no persistent device |
                         | identity             |
                         +----------------------+
```

---

# Build 3 — CYD DEV Lite

**STATUS: WORKING / HARDWARE VALIDATED**

**BEST FOR: FACILITIES, FIXED DEPLOYMENT AND RESEARCH**

CYD DEV Lite is the stable advanced standalone detector.

It adds a graphical display, SD storage, detailed controls, manual data capture and Sentry Mode.

Suitable authorised deployments can include:

- Childcare facilities
- After-school care
- Schools
- School administration
- School technology departments
- Offices
- Reception areas
- Front desks
- Healthcare facilities
- Aged-care facilities
- Secure facilities
- Correctional administration areas
- Facility entry points
- Long-term BLE research
- Database development

### Parts

- 1 × ESP32-2432S028R CYD
- 1 × CYD case
- 1 × 3 dBi 2.4 GHz U.FL / IPEX antenna
- 1 × SD card

### Preferred CYD Hardware

Preferred board characteristics:

- ESP32-U / external-antenna-capable radio module
- U.FL / IPEX connector
- USB-C power/programming ports preferred over Micro-USB

The exact radio-module revision supplied by sellers can vary.

Do not assume that seller photographs guarantee the same module or antenna-routing configuration.

### Required External-Antenna Resistor Modification

> **THE 0-OHM RF ANTENNA-SELECTOR RESISTOR MUST BE MOVED / RESOLDERED BEFORE THE EXTERNAL U.FL / IPEX ANTENNA WILL BE THE ACTIVE RF PATH.**

On the external-antenna-capable CYD boards used during development, the presence of the U.FL / IPEX socket does not mean the external antenna is automatically selected.

The tiny 0-ohm RF selector resistor must be physically moved from the onboard PCB-antenna routing position to the U.FL / IPEX routing position.

If this resistor is not changed, plugging an external antenna into the IPEX socket does not select that antenna.

#### Procedure

1. Disconnect USB and every other power source from the CYD.
2. Locate the ESP32 radio module, onboard PCB antenna and U.FL / IPEX socket.
3. Locate the tiny **0-ohm RF antenna-selector resistor** in the antenna-routing area.
4. Identify the resistor position currently routing the ESP32 RF feed to the onboard PCB antenna.
5. Using a fine-tip soldering iron, tweezers and magnification, remove the 0-ohm resistor from the PCB-antenna position.
6. **Move/resolder the same 0-ohm resistor into the selector position that routes the ESP32 RF feed to the U.FL / IPEX connector.**
7. On a three-pad selector layout, bridge the common RF-feed pad to the U.FL / IPEX path only. **Do not bridge both antenna paths.**
8. Inspect for solder bridges, lifted pads or other damage.
9. Connect the external antenna.
10. Restore power only after the selector has been changed and the antenna is connected.

> **If the 0-ohm resistor remains in the PCB-antenna position, plugging an antenna into the U.FL / IPEX socket does not select the external antenna.**

> **CYD/module revisions can differ. Verify the RF selector layout on the exact board received before moving the resistor.**

### Power

A suitable USB-C wall adapter or fixed USB power source is recommended.

### Programming Requirements

- Computer
- Arduino IDE
- Arduino-ESP32 3.3.11
- USB data cable
- Target: ESP32-2432S028R CYD
- Serial Monitor: 115200 baud
- OTA updating: not used
- Use the partition configuration supplied with the final CYD package

### Hardware

Display:

```text
ILI9341
320 × 240
Landscape
Rotation 3
```

TFT:

```text
MISO  -> GPIO 12
MOSI  -> GPIO 13
SCLK  -> GPIO 14
CS    -> GPIO 15
DC    -> GPIO 2
RST   -> -1
BL    -> GPIO 21
```

RGB LED:

```text
GPIO 4
GPIO 16
GPIO 17
```

BOOT:

```text
GPIO 0
```

### Main Features

CYD DEV Lite includes:

- 2.8-inch graphical display
- BLE smart-glasses detection
- LOW / POSSIBLE / HIGH rule scoring
- CAM AND AUDIO classification
- Suspicious/development-device review
- Known-device annotation
- SD logging
- Manual BLE context capture
- SYS STAT
- Self test
- Sentry Mode
- Rotating private BLE scanner address
- Session-scoped pseudonymous device hashes

### CYD DEV Lite Confidence Levels

#### LOW — 1 to 59

Low-confidence evidence is primarily a clue, annotation or reference signal.

LOW does not by itself produce a smart-glasses warning.

#### POSSIBLE — 60 to 96

Orange warning.

The advertisement matched a configured smart-glasses-related rule strong enough to warrant user attention.

#### HIGH — 97 to 100

Red warning.

The advertisement matched a configured rule reaching the firmware's HIGH presentation threshold.

A score is a rule-match score.

It should not be interpreted as:

> “There is a 97% probability this is smart glasses.”

### CYD Sentry Mode

Sentry Mode is primarily designed for long-term unattended or lightly attended deployment.

It is particularly useful for:

- Environmental BLE data collection
- Long-duration facility monitoring
- Schools and institutional environments
- Field research
- Baseline building
- Database development
- Signature research
- False-positive refinement
- Understanding BLE activity over time

Sentry is not ESP32 deep sleep.

The detector remains operational while the screen can remain dark and unobtrusive.

The standard configuration waits approximately:

**30 minutes**

between ordinary measurements.

Each measurement lasts approximately:

**10 seconds**

### What Sentry Measures

Sentry counts unique BLE device identities within the measurement window.

It does not simply count every advertisement.

RSSI and estimated distance do not define the Sentry activity metric.

### Baseline Building

The activity threshold is based on the greater of:

- 150% of the existing baseline
- Baseline + 5 devices

A sample must be strictly above the threshold to trigger active learning.

### Active Learning

When significant activity is detected, Sentry enters a fixed:

**60-minute ACTIVE learning period**

During that period, repeated 10-second BLE windows are collected.

The 60-minute timer is fixed.

Additional activity does not continually restart or extend it.

### Classification Continues

During Sentry scan periods, normal classification can continue using:

- Smart-glasses rules
- Camera/audio rules
- Known-device rules
- Suspicious/development-device rules
- False-positive handling

Relevant observation logging can also continue.

### Manual CYD BLE Context Capture

CYD DEV Lite can capture:

- 5 observations before
- 5 observations after

for a total of:

**10 observations**

This provides context around an event of interest for later database analysis.

### Manual Logging for Signature Development and Database Research

The CYD versions include manual logging tools intended specifically for **signature development, confirmation, database building and research**.

Manual logging is useful for:

- Adding new smart-glasses signatures
- Adding new camera/audio signatures
- Confirming suspected device characteristics
- Comparing repeated observations from the same known product
- Identifying useful advertised names
- Recording Company IDs
- Recording manufacturer data
- Recording service UUIDs
- Comparing address types and advertisement behaviour
- Distinguishing genuine product characteristics from environmental noise
- Reducing false positives
- Building known-device annotations
- Improving HIGH / MEDIUM / LOW rule tables
- Expanding the camera/audio database
- Field research and long-term database refinement

### Recommended Signature-Confirmation Process

1. Physically confirm the product being tested.
2. Capture multiple BLE observations with the CYD.
3. Repeat the capture in more than one session where practical.
4. Compare advertised name, Company ID, manufacturer data, UUIDs and other repeatable fields.
5. Separate stable product characteristics from changing addresses or temporary values.
6. Check whether the same characteristic appears on unrelated devices.
7. Classify the evidence as **CONFIRMED**, **DOCUMENTED**, **FIELD DERIVED**, **REFERENCE ONLY** or **EXPERIMENTAL**.
8. Add a detection rule only when the evidence is strong enough for the intended confidence level.
9. Re-test the rule in normal public environments to check for false positives.

### Research Principle

Manual logging is intended to answer:

> **Which BLE characteristics repeatedly belong to this physically confirmed device?**

It should not be used to build permanent histories of unknown nearby people or devices.

The goal is **signature research, confirmation, false-positive reduction and database improvement**.

---

# Build 4 — CYD Expanded DEV

**STATUS: WORKING / EXPERIMENTAL / MOSTLY UNTESTED**

**CAPABILITY: HIGHEST**

**INTENDED USE: STATIONARY / DEVELOPMENT**

CYD Expanded DEV is the highest-capability version of the current project.

It combines:

- ESP32-2432S028R CYD
- External 8 dBi U.FL / IPEX antenna
- Required 0-ohm RF antenna-selector resistor modification
- SD storage
- Separate ESP32 database coprocessor
- Local CYD rules
- Expanded database architecture
- Asynchronous UART database lookups
- Future/planned Wi-Fi development

Potential authorised environments can include:

- Schools
- School IT departments
- Healthcare facilities
- Aged-care facilities
- Front offices
- Reception areas
- Secure facilities
- Correctional facilities
- Controlled-access locations
- Development laboratories

Actual reception and detection range depends heavily on:

- Target-device transmit power
- Antenna installation
- Walls
- Obstructions
- RF interference
- Building construction
- Antenna orientation
- Local RF conditions

No fixed detection distance should be guaranteed without direct measurement.

### Parts

- 1 × ESP32-2432S028R CYD with onboard U.FL / IPEX-capable radio hardware
- 1 × CYD case
- 1 × 8 dBi U.FL / IPEX antenna
- 1 × additional ESP32 database coprocessor
- 1 × SD card
- Approximately 30 cm wire
- Solder
- Flux

### Mandatory Before Using the 8 dBi Antenna

> **THE CYD'S 0-OHM RF ANTENNA-SELECTOR RESISTOR MUST BE CHANGED.**

The resistor must be moved / resoldered from the PCB-antenna position to the U.FL / IPEX position.

The external 8 dBi antenna will not become the selected RF path simply because it is plugged into the U.FL / IPEX socket.

If the 0-ohm selector resistor remains in the PCB-antenna position, the ESP32 radio remains routed to the onboard PCB antenna.

For the CYD Expanded DEV hardware described here, moving this resistor is a required assembly step, not an optional range modification.

### Power

A suitable fixed USB/wall power source is required.

### CYD Expanded External-Antenna Hardware Check

Before flashing or commissioning CYD Expanded DEV, verify:

- The CYD actually has an onboard U.FL / IPEX connector.
- The delivered radio-module revision is suitable for external-antenna routing.
- The 0-ohm RF selector resistor has been physically moved / resoldered from the PCB-antenna route to the U.FL / IPEX route.
- The solder work has been inspected for bridges or damage.
- The 8 dBi antenna is connected only after the RF selector has been changed.

If the selector modification has not been completed, the external antenna is not the selected RF path.

### Important: CYD Expanded Uses Two Firmware Packages

#### CYD Expanded DEV — 1 of 2

Flash to:

**ESP32-2432S028R CYD**

This is the main:

- BLE scanner
- Local detection engine
- Display
- User interface
- Privacy engine
- Logging system

#### CYD Expanded DEV — 2 of 2

Flash to:

**Separate ESP32 database coprocessor**

This is the expanded database-processing half.

```text
1 OF 2  ----------------->  ESP32-2432S028R CYD

2 OF 2  ----------------->  SEPARATE ESP32
                             DATABASE COPROCESSOR
```

> **Do not reverse them.**

### CYD Expanded Programming Requirements

- Computer
- Arduino IDE
- Arduino-ESP32 3.3.11
- USB data cable for each ESP32
- OTA updating: not used
- CYD Serial Monitor: 115200 baud
- CYD-to-database UART: 460800 baud, 8N1
- Database ESP: classic ESP32 Dev Module
- Database flash target: approximately 4 MB
- No PSRAM required
- Use the build-specific partition configuration supplied with the final firmware

### CYD Expanded UART

Baud:

```text
460800
```

Format:

```text
8N1
```

Pins:

```text
CYD TX: GPIO 22
CYD RX: GPIO 27
Database ESP RX: GPIO 16
Database ESP TX: GPIO 17
```

Wiring:

```text
CYD GPIO22 TX  ---------->  Database ESP GPIO16 RX

CYD GPIO27 RX  <----------  Database ESP GPIO17 TX

CYD GND        -----------  Database ESP GND
```

The protocol uses framed binary messages and CRC16 integrity checking.

The external database is supplementary.

If the database processor becomes unavailable, the CYD continues local BLE scanning and local rule evaluation.

### CYD Expanded Busy-Environment Mode

The Expanded build includes a database-servicing strategy for busy BLE environments.

Current design:

- Rolling 60-second activity window
- Busy mode enters when unique devices are strictly greater than 20 per minute
- Busy mode exits when activity is strictly below 15
- Five-advertisement median RSSI
- Stronger/nearer requests are serviced before weaker/farther requests
- Weaker/farther observations are not discarded
- Local CYD rules continue normally
- Busy mode changes database service order only
- Busy mode does not change local confidence scoring

### Updating the Bluetooth Company ID Database

CYD Expanded DEV 2 of 2 includes:

```text
generate_company_db.py
```

The script generates:

```text
company_ids_generated.h
```

#### macOS

```text
python3 --version
cd /path/to/ESP32_BLE_DATABASE_COPROCESSOR_v1
python3 generate_company_db.py
```

Using local JSON:

```text
python3 generate_company_db.py --input company_ids.json
```

#### Windows

```text
py --version
cd "C:\path\to\ESP32_BLE_DATABASE_COPROCESSOR_v1"
py generate_company_db.py
```

#### Linux

```text
python3 --version
cd /path/to/ESP32_BLE_DATABASE_COPROCESSOR_v1
python3 generate_company_db.py
```

After regenerating `company_ids_generated.h`:

1. Recompile CYD Expanded DEV 2 of 2.
2. Reflash the database ESP.

The CYD does not need to be reflashed solely because this generated header changed.

Future Wi-Fi capabilities may change this update process.

---

# Build 5 — All Projects

This section contains information that applies across the detector family rather than to one specific hardware build.

## About This Project

This project is designed to make practical privacy-awareness technology accessible to ordinary people, not only developers, electronics specialists or security researchers.

The detectors listen for observable Bluetooth Low Energy advertisements and compare them against databases containing known, documented, field-derived, reference and experimental characteristics associated with:

- Smart glasses
- Camera-equipped wearables
- BLE-enabled cameras
- Consumer hidden-camera products
- Microphones
- Wireless microphones
- Recording devices
- Consumer “bug”-type devices
- Other camera/audio-capable BLE equipment

Privacy matters to everyone, but particular consideration has been given to people and environments where privacy, safeguarding, security or personal safety may be especially important.

Potential users and environments include:

- Children
- Parents and families
- Women
- People affected by domestic or family violence
- Childcare centres
- After-school care services
- Schools
- Teachers concerned about privacy or safeguarding
- School IT and technology staff
- School administration
- Healthcare facilities
- Medical administration areas
- Aged-care facilities
- Offices
- Reception and front-desk areas
- Correctional facilities and prisons
- Secure facilities
- Controlled-access facilities
- Organisations with legitimate privacy or security requirements

Use in schools, healthcare environments, prisons, workplaces or other controlled facilities should always be authorised and consistent with applicable law, organisational policy, employment requirements and local procedures.

This project is an awareness tool.

It is not designed to identify people, track people, access nearby equipment, interfere with communications or determine someone's intent.

A detection means:

> **A nearby BLE advertisement resembles a known or suspected device profile.**

It does not mean:

> **The person carrying that device is recording.**

## Intended Detection Scope

This project is primarily designed to identify or flag BLE characteristics associated with consumer-grade and commercially available devices.

Examples include:

- Consumer smart glasses
- Camera-equipped wearables
- Consumer BLE cameras
- Hidden-camera products sold through normal retail channels
- Wireless microphones
- Recording devices
- Consumer surveillance products
- Consumer “bug”-type products
- Other commercially available BLE-enabled camera/audio equipment

## Target BLE Devices

> **Important:** A detected BLE device is not automatically malicious, illegal or being used improperly. Detection means that the device matched one or more identifiers, signatures, keywords or behavioural rules used by the project.

### Smart Glasses and Camera Glasses

The detector is primarily focused on smart glasses and camera-equipped eyewear, including:

- Ray-Ban Meta smart glasses
- Oakley Meta smart glasses
- Kmart / Anko camera glasses
- HeyCyan smart glasses
- Rokid AI glasses
- RayNeo smart glasses
- VITURE smart glasses and accessories
- Huawei AI glasses
- Generic AI glasses
- Generic camera glasses
- Other BLE-advertising smart glasses
- Unknown devices matching known smart-glasses BLE signatures

### Expanded Smart-Glasses Target List

The project database may also include rules or signatures relating to:

- Meta Glasses Adventurer
- Meta Glasses Fury
- Meta Starfire Kylie Edition
- VITURE Helix
- Acer GI0
- Willit AI Glasses
- RayNeo V4
- Samsung × Warby Parker × Google
- Samsung × Gentle Monster × Google
- B by Lenskart
- Moonix Pro
- Huawei AI Glasses
- Ray-Ban Meta Optics Blayzer
- Ray-Ban Meta Optics Scriber
- Rokid AI Glasses Style
- Alibaba Quark AI Glasses G1
- DPVR G5
- DPVR G1

> Some products may be represented by database rules before sufficient real-world BLE captures are available. A listed product should not be interpreted as a guarantee that every revision of that product can currently be identified.

### Wireless Microphones

The project also looks for BLE evidence associated with wireless microphone and audio-recording equipment, including:

- RØDE wireless microphone systems
- DJI Mic systems
- Hollyland wireless microphones
- Sennheiser wireless microphone equipment
- Generic wireless microphones
- Bluetooth microphones
- Wireless lavalier microphone systems
- Wireless audio transmitters
- Wireless audio receivers

Relevant advertised-name clues may include terms such as:

```text
Wireless Mic
Bluetooth Mic
MIC-
```

Keyword matches alone should not normally be treated as definitive device identification.

A specific brand or product should not be described as supported by a firmware release unless its corresponding rule is actually present in that firmware.

### Cameras and Recording Devices

Where detectable through BLE, the project may identify or score:

- Bluetooth-enabled cameras
- Wearable cameras
- Portable cameras
- Camera-control devices
- Action cameras
- Camera remotes
- Unknown BLE devices matching known camera-related signatures

### Audio Recording Devices

The project may also identify BLE evidence relating to:

- Bluetooth-capable audio recorders
- Portable recording equipment
- Wireless recording accessories
- Audio transmitters
- Audio receivers
- Unknown devices matching known microphone or audio signatures

## Detection Evidence

Depending on the firmware version and hardware configuration, classification can use combinations of:

- Advertised BLE device name
- Bluetooth Company ID
- Manufacturer-specific data
- Service UUIDs
- Known BLE payload signatures
- High-confidence keywords
- Known-device database matches
- Device and manufacturer annotations
- RSSI / proximity information
- Hard-coded detection rules
- External database-coprocessor results

No single field should be assumed to provide perfect identification.

## Background BLE Devices

Common BLE devices are expected to appear during normal operation and are generally treated as background traffic unless stronger evidence is present.

Examples include:

- Smartphones
- Tablets
- Smartwatches
- Fitness trackers
- Earbuds
- Headphones
- Bluetooth speakers
- Keyboards
- Mice
- TVs
- Vehicles
- Game controllers
- Smart-home sensors
- Retail beacons
- Medical wearables
- Generic unnamed BLE devices

The presence of one of these devices should not, by itself, produce a high-confidence privacy alert.

## Classification Principle

The project is intended to favour multiple supporting indicators rather than relying only on a device name.

A simplified evidence hierarchy is:

1. Known device-specific BLE signature
2. Known manufacturer and product-specific evidence
3. Manufacturer-data or service-UUID match
4. Camera, microphone, smart-glasses or recording-related evidence
5. Relevant advertised-name keyword
6. Generic or unknown BLE device

The purpose of this approach is to remain sensitive to relevant devices while reducing false positives from ordinary Bluetooth equipment.

## Why Dedicated Hardware Instead of Only a Phone App?

Bluetooth-scanning applications already exist and can be useful.

This project takes a different approach: the ESP32 itself performs BLE scanning and classification.

For the portable S3 and WROOM builds, the phone is primarily used as a convenient USB-C power source.

The detector does not require access to:

- Contacts
- Messages
- Photos
- User accounts
- Phone location history or active location
- The phone camera
- The phone microphone

It also does not require:

- A companion phone application
- Wi-Fi for current BLE detection
- Cloud processing
- A cloud account

A dedicated detector can remain focused on BLE scanning without depending on a phone application remaining open or receiving unrestricted background execution from the phone operating system.

This does not mean every Bluetooth-scanning app is insecure.

Different applications have different privacy models, background-operation limitations and operating-system restrictions.

This project instead provides a separate, inspectable hardware detector with local processing.

## What Is Bluetooth Low Energy?

Bluetooth Low Energy, normally abbreviated BLE, is the low-power part of the Bluetooth standard.

BLE was designed to allow devices to exchange relatively small amounts of information while using substantially less energy than continuously active conventional radio communication.

This makes BLE particularly useful in products that are:

- Physically small
- Battery powered
- Expected to run for long periods
- Limited in available battery capacity
- Wearable
- Portable
- Sensor based
- Required to send only small amounts of information

Common BLE applications include:

- Wearables
- Smart watches
- Earbuds
- Headphones
- Sensors
- Fitness equipment
- Medical accessories
- Smart-home equipment
- Electronic tags
- Smart glasses
- Cameras and accessories
- Wireless audio equipment

BLE is not limited to small devices, but low power consumption is one of the main reasons it is widely used in compact battery-powered electronics.

## How BLE Advertising Works

Bluetooth Low Energy uses a mechanism called advertising.

Advertising is a fundamental part of BLE discovery and connectionless communication.

A BLE device that wants to announce its presence or make itself available for discovery can transmit short advertising packets without first establishing a connection.

Another BLE receiver can therefore hear those advertisements without:

- Pairing
- Connecting
- Authenticating
- Accessing the device
- Opening a GATT connection
- Activating a camera
- Activating a microphone
- Retrieving stored files

Advertising is normal BLE behaviour and is not caused by this detector.

## Advertising Timing

BLE advertising frequency depends on the device and configuration.

Traditional BLE advertising intervals can commonly range from approximately:

**20 milliseconds**

to:

**10.24 seconds**

A manufacturer may choose a shorter interval for fast discovery or a longer interval to reduce power consumption.

## What Can a BLE Advertisement Contain?

Depending on the product, BLE advertisements may expose characteristics such as:

- Advertised name
- Bluetooth Company Identifier
- Manufacturer-specific data
- Service UUIDs
- Service data
- Address information
- Address type
- Manufacturer-specific patterns
- Product-specific patterns
- Capability information

The detector compares combinations of these characteristics against its rule and reference databases.

The detector does not need to:

- Pair with the nearby device
- Connect to it
- Access its operating system
- Retrieve files
- Activate its camera
- Activate its microphone

## What About Devices That Change or Hide Their BLE Address?

Modern BLE devices often use randomised, private or rotating Bluetooth addresses.

A changing BLE address does not automatically defeat these detectors.

The detection engines are deliberately designed not to rely exclusively on a permanent BLE MAC address.

They can also examine characteristics such as:

- Advertised name
- Company ID
- Manufacturer data
- Service UUIDs
- Service data
- Manufacturer patterns
- Known product signatures
- Keywords
- Combinations of several signals

Example:

```text
ADVERTISEMENT 1
Address: A1:B2:C3:D4:E5:F6
Name: ExampleGlasses
Company ID: XXXX
Manufacturer pattern: ABCD...

             address rotates

ADVERTISEMENT 2
Address: 62:19:8A:44:71:C0
Name: ExampleGlasses
Company ID: XXXX
Manufacturer pattern: ABCD...
```

The second advertisement can still be evaluated using its other observable BLE characteristics.

This is important because BLE address rotation is itself a legitimate privacy feature.

However, there are limits.

If a product:

- Rotates its address
- Removes its name
- Randomises its manufacturer data
- Changes UUID or service information
- Obfuscates useful payload fields
- Stops advertising

then classification may become weaker or impossible.

The project does not claim to defeat every possible BLE privacy, randomisation, encryption or obfuscation method.

## There Is No Permanent Universal BLE Signature

There is no single permanent BLE signature that every BLE device is guaranteed to continuously transmit.

This project should therefore be treated as an additional privacy-awareness system, not as a guarantee that every smart-glasses, camera, microphone or recording device will be detected.

## Detection Limitations

BLE detection has inherent limitations.

A target device may:

- Not advertise continuously
- Disable Bluetooth
- Use changing private/random addresses
- Change firmware or advertisement format
- Advertise without a readable name
- Use identifiers shared with unrelated products
- Be outside practical radio range
- Be shielded by walls, people, vehicles or other obstacles

Likewise, a non-target device can occasionally resemble a known target.

For these reasons, project detections should be treated as **indicators requiring context**, not proof that a particular person is recording, tracking or acting unlawfully.

This project is intended primarily for awareness of consumer-grade and commercially available BLE devices.

It should not be relied upon to detect military-grade, intelligence-grade, specialist covert or purpose-built advanced surveillance equipment.

The absence of an alert must never be interpreted as proof that an environment contains no recording, surveillance, camera or microphone equipment.

## Requirements Common to Every Build

Every version requires:

- A computer
- Arduino IDE
- A suitable USB data cable
- The correct Espressif ESP32 board package
- The libraries required by that firmware package

A charging-only USB cable will not work for programming.

Arduino IDE and the USB cable are programming/setup requirements rather than assembly tools.

## Firmware Updating and Partitions

Current public builds are intended to be programmed directly over USB.

OTA firmware updating is not part of the normal public update workflow.

Firmware changes are normally installed by:

1. Connecting the ESP32 to a computer.
2. Opening the correct Arduino project.
3. Compiling the firmware.
4. Uploading it over USB.

Partition layouts can vary between builds because each detector uses flash differently.

Always use the partition configuration supplied or specified for the firmware being flashed.

Do not substitute another partition scheme unless you understand why it is being changed.

Examples:

- S3 Lite uses its supplied sketch-local `partitions.csv`
- WROOM uses its verified normal/default partition layout
- CYD builds should use the configuration supplied with their final package
- The Expanded database ESP should use the verified application-space configuration supplied for that firmware

## General Assembly Tools

Where assembly is required:

- Soldering iron
- Wire cutters
- Wire strippers
- Small screwdriver set
- Tweezers
- Multimeter
- Heat-shrink or electrical tape if required
- Helping hands / PCB holder if required

The ESP32-S3 Lite v1.1 requires no assembly tools.

## Flashing Overview

1. Install Arduino IDE.
2. Install the correct Espressif ESP32 board package.
3. Place the `.ino` and required project files together.
4. Connect using a USB data cable.
5. Select the correct board.
6. Select the correct serial port.
7. Confirm the required partition configuration.
8. Confirm the required upload speed.
9. Select Verify.
10. Resolve compile errors.
11. Select Upload.
12. Allow flashing to complete.
13. Restart if required.
14. Confirm expected LED/display behaviour.

### S3 Flashing Summary

- Arduino-ESP32: 3.3.11
- Serial Monitor: 115200
- Upload: 921600
- Fallback Upload: 460800
- OTA: not used
- Partition: supplied `partitions.csv`

Expected startup:

```text
BLUE
 |
 v
GREEN
```

### WROOM Flashing Summary

- Board: ESP32 Dev Module
- Arduino-ESP32: 3.3.11
- BLE: Bluedroid
- Upload: 115200
- Serial: 115200
- Flash mode: DIO where applicable
- Partition: normal/default
- OTA: not used
- External library: Adafruit NeoPixel
- No OLED libraries required

Expected startup:

```text
BLUE
 |
 v
GREEN
```

### CYD DEV Lite Flashing Summary

Target:

- ESP32-2432S028R CYD
- Arduino-ESP32: 3.3.11
- Serial: 115200
- OTA: not used
- SD card recommended/required for normal logging functionality
- Use the released CYD partition configuration

After flashing:

1. TFT initialises.
2. Startup/legal interface appears.
3. Private BLE setup completes.
4. Scanning becomes available.
5. Main interface appears.
6. Check SD status.
7. Use SYS STAT to verify operation.

### CYD Expanded Flashing Warning

> **DO NOT FLASH BOTH FIRMWARE HALVES TO THE SAME BOARD.**

```text
CYD EXPANDED DEV — 1 OF 2
        |
        +------> ESP32-2432S028R CYD

CYD EXPANDED DEV — 2 OF 2
        |
        +------> SEPARATE ESP32
                 DATABASE COPROCESSOR
```

## Troubleshooting Decision Tree

```text
DEVICE DOES NOT POWER
        |
        +-- Check USB cable
        |
        +-- Try another power source
        |
        +-- Phone-mounted build?
        |      |
        |      +-- Confirm phone supplies USB accessory power
        |
        +-- CYD?
               |
               +-- Try suitable wall USB power

DEVICE POWERS BUT WILL NOT FLASH
        |
        +-- Correct board selected?
        |
        +-- Correct serial port?
        |
        +-- USB DATA cable?
        |
        +-- Correct partition setting?
        |
        +-- Correct upload speed?
        |
        +-- Try BOOT
        |
        +-- Try slower upload speed

UPLOAD SUCCEEDS BUT NO GREEN STATE
        |
        +-- Private BLE setup issue?
        |
        +-- Purple status?
        |
        +-- Restart detector
        |
        +-- Confirm Arduino-ESP32 version

CYD DISPLAY WORKS BUT SD FAILS
        |
        +-- SD card installed?
        |
        +-- Reinsert card
        |
        +-- Check format
        |
        +-- Try another card

CYD EXTERNAL ANTENNA APPEARS TO HAVE NO EFFECT
        |
        +-- Does the CYD actually have U.FL / IPEX?
        |
        +-- Is it the external-antenna-capable radio revision?
        |
        +-- HAS THE 0-OHM RF SELECTOR RESISTOR BEEN MOVED?
        |      |
        |      +-- NO -> External antenna is NOT selected
        |      |        Move/resolder resistor from
        |      |        PCB-antenna route to U.FL / IPEX route
        |      |
        |      +-- YES -> Inspect solder joint / connector / antenna
        |
        +-- Do not assume plugging in the antenna changes the RF path

CYD EXPANDED DATABASE FAILURE
        |
        +-- 1 of 2 flashed to CYD?
        |
        +-- 2 of 2 flashed to database ESP?
        |
        +-- Database ESP powered?
        |
        +-- Common ground connected?
        |
        +-- TX connected to RX?
        |
        +-- RX connected to TX?
        |
        +-- UART set to 460800 / 8N1?
        |
        +-- Correct firmware on each device?
```

## Detection Database Development

The database combines different forms of evidence.

Public documentation uses categories such as:

- **CONFIRMED**
- **DOCUMENTED**
- **FIELD DERIVED**
- **REFERENCE ONLY**
- **EXPERIMENTAL**

### Confirmed / Documented

Can include:

- Official advertised names
- Confirmed manufacturer fingerprints
- Documented manufacturer information
- Documented service information
- Strong multi-signal combinations
- Physically confirmed observations

### Field Derived

Part of the database was manually developed and refined through approximately two weeks of BLE signature collection and real-world field testing in Sydney, Australia.

That work has been used to:

- Identify ordinary BLE products
- Reduce false positives
- Identify headphones and speakers
- Identify commercial equipment
- Investigate smart-glasses advertisements
- Examine camera/audio candidates
- Refine known-device annotation
- Test behaviour in busy BLE environments

### Reference Only

Examples include:

- Bluetooth Company Identifiers
- Service UUID assignments
- Manufacturer reference data

A Company ID identifies an assigned organisation.

It does not prove the physical product type.

### Experimental

Some rules remain exploratory and may be based on:

- Advertised names
- Product/model names
- Incomplete public information
- User-supplied information
- Unverified BLE characteristics

Experimental rules should not be presented as equivalent to confirmed device fingerprints.

## Database / Reference Sources

The following databases and reference sources were used during development and research:

- Nordic Semiconductor BLE Database
- Bluetooth SIG BLE database
- IEEE data
- Spectacle Database
- BLE-Payloads lists
- Fingerbank data
- Sparrow data
- Region-specific and environment-specific data collected during Sydney field testing

The project also contains a large locally developed signature/reference dataset, with further classification work planned.

## Known Kmart / Anko Camera Glasses Example

One documented product-specific example is:

```text
Item: 43700141
Model: JLR-82067
Documented Bluetooth name: Anko43700141
```

The refined rule uses the exact advertised name as a strong product-specific clue.

An advertised name can potentially be imitated, so it should not be described as cryptographic proof of identity.

## Contributing a New BLE Signature

Suggested submission format:

```text
Product:
Manufacturer:
Model:
Advertised name:
Company ID:
Manufacturer data:
Service UUIDs:
Observed repeatedly: YES / NO
Physically confirmed device: YES / NO
Source/documentation:
Notes:
```

Please do not submit another person's captured raw BLE MAC address.

The strongest submissions are:

- Captured from equipment you own
- Captured from equipment you can positively identify
- Repeated observations
- Physically confirmed
- Supported by documentation where possible

## Version Maturity

### WORKING

Reported operational on physical hardware.

### HARDWARE VALIDATED

Compiled, flashed and tested against the intended hardware and major functionality.

### EXPERIMENTAL

Development firmware that operates but still requires broader validation.

| Build | Maturity |
|---|---|
| **ESP32-S3 Lite v1.1** | **WORKING / HARDWARE VALIDATED** |
| **ESP32-WROOM v1.1** | **WORKING / HARDWARE VALIDATED** |
| **CYD DEV Lite** | **WORKING / HARDWARE VALIDATED** |
| **CYD Expanded DEV — 1 of 2** | **WORKING / EXPERIMENTAL / MOSTLY UNTESTED** |
| **CYD Expanded DEV — 2 of 2** | **WORKING / EXPERIMENTAL / MOSTLY UNTESTED** |

## Real-World Development and Testing

The project has been refined through real-world BLE field testing rather than only simulated data.

Part of the database was developed through approximately two weeks of BLE collection and field testing in Sydney, Australia.

Testing has included:

- Public transport buses, metros, trains and stations
- Shopping environments
- Offices
- Commercial areas
- High-density BLE environments

## Future Development Roadmap

Planned development includes:

- Additional verified smart-glasses signatures
- Additional camera/audio signatures
- Australian consumer surveillance-device research
- Additional verified microphone signatures
- Continued field testing
- False-positive reduction
- Improved database provenance
- Expanded Company ID/reference information
- Continued CYD Expanded validation
- Database-coprocessor improvements
- Community-submitted signatures
- Improved installation documentation
- Improved troubleshooting documentation
- Future Wi-Fi scanning with dedicated processing for Wi-Fi classification
- Further development of proximity/"fox hunting" behaviour in classification to help future-proof changing manufacturer information
- Migration of proven CYD features into S3/WROOM where appropriate

## Wi-Fi Roadmap

Wi-Fi scanning is planned/experimental for CYD Expanded DEV.

The intended direction is for Wi-Fi observations to supplement BLE evidence rather than replace BLE detection.

Current BLE-only releases should not be described as already providing completed Wi-Fi detection.

## Accessibility

> **Smart glasses have legitimate purposes, particularly as accessibility tools.**

The project is not intended to oppose accessibility technology or technological progress.

The aim is to support appropriate privacy safeguards, transparency and responsible use while recognising legitimate accessibility applications.

## Project Development and Proposed Regulation

The Australian Government has considered and proposed measures affecting digital privacy, surveillance, data collection and related powers.

Whether development of this project continues at the same pace may depend on future regulatory changes and the developer's other obligations.

This project may therefore become unsupported or only intermittently maintained.

---

# Full Privacy Information

## Privacy by Design

Privacy is a core engineering requirement of this project.

The detector does not need access to:

- Contacts
- Photos
- Messages
- User accounts
- Phone location history
- Phone camera
- Phone microphone

The detector itself does not require:

- A camera
- A microphone
- A companion application
- Wi-Fi for current BLE operation
- Cloud processing

## No Persistent Observed-Device Identity

No observed BLE device is intentionally assigned a persistent identity across sessions.

An observed raw BLE MAC address may temporarily exist in RAM while an advertisement is being processed.

It may be used transiently for:

- Exact-rule matching
- OUI/manufacturer lookup
- Deduplication
- Session correlation
- Creation of a session-scoped pseudonym

Exact observed BLE MAC addresses are not intentionally written into persistent operational logs.

Where correlation is useful, firmware may store a session-scoped pseudonymous value such as:

```text
MAC-HASH-7A3F91C2D4E5F607
```

The session salt:

- Is randomly generated
- Exists only for that session
- Is not intentionally persisted
- Changes between sessions

The same observed BLE address should therefore normally receive a different pseudonym in another session.

This allows observations to be compared during a session without deliberately creating a permanent cross-session device identity.

## Persistent Observation Files

Some builds retain observation, diagnostic or detection logs.

These files may contain:

- Observations
- Classification results
- Confidence/rule scores
- Counters
- Session-scoped pseudonyms
- Manufacturer information
- Advertised names
- Service information
- RSSI data

A persistent observation record does not mean the observed device itself has been assigned a permanent identity.

## Pseudonymisation Is Not Anonymity

Session hashing reduces retention of exact hardware addresses, but it does not make every field anonymous.

Other BLE advertisement information may still be distinctive, including:

- Advertised names
- Manufacturer payloads
- Company IDs
- Service UUID combinations
- Product-specific data

Users should consider this when storing, sharing or publishing exported logs.

## Protecting the Detector's Own BLE Identity

Where supported, the detector uses a private scanner-originating BLE address instead of continuously exposing one fixed public identity.

A new private address is generated at a random interval of approximately:

**12–18 minutes**

Rotation occurs while scanning is idle.

This reduces exposure of a stable scanner identity.

It does not make the detector invisible or undetectable.

## Private-Address Failure

### CYD DEV Lite — Fail Closed

If the private NRPA cannot be established and verified:

- Active BLE scanning remains blocked
- Firmware retries private-address setup
- It does not silently continue normal active scanning using its public identity

This is deliberate fail-closed behaviour.

### ESP32-S3 Lite / ESP32-WROOM

Portable builds make privacy problems visible.

- **Flashing purple:** private setup failed and is retrying
- **Solid purple:** clearly indicated public-address fallback where supported

The user should never be given the impression that private mode remains active when it has failed.

## Manual Logging and Research

CYD manual logging is intended for:

- Signature confirmation
- Database development
- False-positive investigation
- Field research
- Known-device classification

It is not intended to create permanent histories of unknown nearby people or devices.

Where device correlation is needed, the same session-scoped privacy rules apply.

## Handling Exported Logs

Exported data should be treated as potentially sensitive technical observation data.

Even without raw BLE MAC addresses, fields such as advertised names, manufacturer payloads or product-specific signatures may still reveal useful identifying information about devices.

Do not treat pseudonymised logs as automatically anonymous.

---

# Full Legal and Responsible-Use Information

## General Principle

This project observes BLE radio characteristics.

It does not establish:

- Device ownership
- Human identity
- Recording activity
- Camera activity
- Microphone activity
- Intent
- Malicious conduct
- Illegal conduct

A detection should always be interpreted in context.

## Authorised Deployment

Use in locations such as:

- Schools
- Childcare facilities
- Healthcare environments
- Workplaces
- Correctional facilities
- Secure facilities
- Controlled-access areas

should be authorised and consistent with:

- Applicable law
- Workplace requirements
- Organisational policies
- Privacy obligations
- Facility procedures

## Radio and Antenna Use — Australia / NSW

Bluetooth and Wi-Fi radio operation in NSW is regulated primarily under Australian Commonwealth radiocommunications rules administered by ACMA.

Higher-gain antennas are not automatically unlawful, but the complete transmitter/antenna configuration must remain within applicable technical, equipment-compliance and EIRP limits.

For this project:

- CYD DEV Lite uses a 3 dBi 2.4 GHz external antenna.
- CYD Expanded DEV uses an 8 dBi 2.4 GHz external antenna.
- The higher-gain antenna is intended primarily to improve practical BLE reception.
- Do not increase ESP32 transmitter power simply because a higher-gain antenna has been installed.
- Do not add external RF power amplifiers without confirming applicable requirements.

The complete RF configuration, rather than antenna gain alone, determines compliance.

For current Australian requirements, consult ACMA and the applicable Low Interference Potential Devices class-licensing/equipment-compliance material.

## External-Antenna Hardware Modification

For supported CYD external-antenna builds, the 0-ohm RF antenna-selector resistor must be moved from the onboard PCB-antenna path to the U.FL / IPEX path.

This hardware modification can alter the RF configuration of the original development board.

Users supplying, selling or commercially modifying radio equipment may have additional equipment-compliance obligations.

## Surveillance and Privacy Laws

Radio reception, observation logging, workplace monitoring and surveillance laws can differ according to:

- Jurisdiction
- Deployment location
- Information collected
- Data retention
- Purpose
- Employment relationship
- Facility type
- How collected information is subsequently used

The project should therefore not be described as automatically lawful for every use.

## Not Evidence of Wrongdoing

An alert should never, by itself, be treated as evidence that another person:

- Is recording
- Is spying
- Is carrying a prohibited device
- Has malicious intent
- Has committed an offence

BLE advertisements can be imitated, altered or shared across legitimate products.

## User Responsibility

Users are responsible for:

- Understanding local laws
- Obtaining required authorisation
- Following organisational policy
- Handling collected observations responsibly
- Configuring radio equipment appropriately
- Avoiding unlawful interference
- Avoiding misuse of detection results

## Legal Disclaimer

This project and its documentation provide technical and privacy-awareness information.

**They do not constitute legal advice.**

If a deployment raises significant legal, workplace, privacy, surveillance or radio-compliance questions, obtain appropriate professional advice before deployment.

---

# Inspiration and Related Projects

The following projects are credited for inspiration:

- `surveillancewatch/ESP-GlassHole` — ESP32 BLE smart-glasses detector project
- `sh4d0wm45k/glass-detect` — Smart-glasses detection project
- `colonelpanichacks/flock-you` — ESP32 surveillance-infrastructure detection project
- `yjeanrenaud/yj_nearbyglasses` — Nearby Glasses project
- `NullPxl/banrays` — Smart-glasses / anti-recording awareness project
- `colonelpanichacks/ouispy-detector` — OUI/BLE-oriented detector project
- `LuxStatera/flock-hunter-cyd-wifi` — CYD/Wi-Fi detector project
- `haxorthematrix/BLEPTD` — BLE detection/scanning project
- `RamboRogers/esp32-bluetooth-scanner` — General ESP32 Bluetooth scanner project

---

# Dedication

This project is dedicated to my sister, my mother, E.M., her sister M.B/M., C.C and R.H.

---

# Support and Maintenance

This project is likely to become mostly unsupported or only intermittently maintained.

Please:

- Read the documentation carefully before building
- Keep local copies of firmware and documentation
- Record the exact board revision you use
- Keep the supplied partition files with each firmware version
- Keep known-working firmware packages
- Do not assume future updates or direct support will remain available
- Be prepared to self-maintain the hardware and firmware if development stops

Experimental builds, especially CYD Expanded DEV, should not be treated as fully validated production systems.

---

# Final Project Principle

This project is not intended to create fear around Bluetooth devices, cameras, microphones or smart glasses.

Its purpose is to give people additional information about the radio environment around them.

The detector cannot determine another person's intentions.

It cannot determine whether recording is occurring.

It cannot prove that a camera is active.

It cannot prove that a microphone is active.

It cannot establish that somebody is acting unlawfully.

It can observe BLE radio characteristics and report when those characteristics resemble known or suspected device profiles.

> **This project detects radio characteristics, not people. Its purpose is privacy awareness, not surveillance.**
