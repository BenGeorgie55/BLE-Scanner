# BLE-Scanner
Open-source ESP32 privacy-awareness detectors for smart glasses, BLE cameras, microphones and recording devices, with portable and facility-focused builds.
Disclaimer- SOME AI WAS USED IN THIS BUILD ESPECIALLY FOR CUSTOM DATABASE BUILDING.

This project is likely to go mostly unsupported in the future, please read carefully.

ESP32 Smart-Glasses, Camera, Microphone & BLE Privacy Detector

This project detects radio characteristics, not people. Its purpose is privacy awareness, not surveillance.

No detector can establish whether a nearby camera or microphone is currently recording. Treat alerts as information requiring context, not proof of wrongdoing.

About This Project

This project is designed to make practical privacy-awareness technology accessible to ordinary people, not only developers, electronics specialists, or security researchers.

The detectors listen for observable Bluetooth Low Energy (BLE) advertisements and compare them against databases containing known, documented, field-derived, reference, and experimental characteristics associated with:

Smart glasses

Camera-equipped wearables

BLE-enabled cameras

Consumer hidden-camera products

Microphones

Wireless microphones

Recording devices

Consumer “bug”-type devices

Other camera/audio-capable BLE equipment

Privacy matters to everyone, but particular consideration has been given to people and environments where privacy, safeguarding, security, or personal safety may be especially important.

Potential authorised users and environments include:

Children

Parents and families

Women

People affected by domestic or family violence

Childcare centres

After-school care services

Schools

Teachers concerned about privacy or safeguarding

School IT and technology staff

School administration

Healthcare facilities

Medical administration areas

Aged-care facilities

Workplaces

Offices

Reception and front-desk areas

Correctional facilities and prisons

Secure facilities

Controlled-access facilities

Organisations with legitimate privacy or security requirements

Use in schools, healthcare environments, prisons, workplaces, or other controlled facilities should always be authorised and consistent with applicable law, organisational policy, employment requirements, and local procedures.

This project is an awareness tool.

It is not designed to identify people, track people, access nearby equipment, interfere with communications, or determine someone's intent.

A detection means:

A nearby BLE advertisement resembles a known or suspected device profile.

It does not mean:

The person carrying that device is recording.

Intended Detection Scope

This project is primarily designed to identify or flag BLE characteristics associated with consumer-grade and commercially available devices.

Examples include:

Consumer smart glasses

Camera-equipped wearables

Consumer BLE cameras

Hidden-camera products sold through normal retail channels

Wireless microphones

Recording devices

Consumer surveillance products

Consumer “bug”-type products

Other commercially available BLE-enabled camera/audio equipment

The project is not designed or represented as a reliable detector for military-grade, intelligence-grade, specialist government, highly specialised covert, or purpose-built professional surveillance equipment.

Such equipment may:

Use radio systems other than BLE

Disable BLE completely

Transmit only under specific conditions

Use proprietary protocols

Minimise radio emissions

Use heavily changing or encrypted radio characteristics

Use wired communications

Use Wi-Fi, cellular, proprietary RF, or other technologies

Be deliberately engineered to resist ordinary consumer detection methods

Accordingly:

This project is intended primarily for awareness of consumer-grade and commercially available BLE devices. It should not be relied upon to detect military-grade, intelligence-grade, specialist covert, or purpose-built advanced surveillance equipment.

The absence of an alert must never be interpreted as proof that an environment contains no recording, surveillance, camera, or microphone equipment.

Why Dedicated Hardware Instead of Only a Phone App?

Bluetooth-scanning applications already exist and can be useful.

This project takes a different approach: the ESP32 itself performs BLE scanning and classification.

For the portable S3 and WROOM builds, the phone is primarily used as a convenient USB-C power source.

The detector does not require access to:

Contacts

Messages

Photos

User accounts

Phone location history

The phone camera

The phone microphone

It also does not require:

A companion phone application

Wi-Fi for current BLE detection

Cloud processing

A cloud account

A dedicated detector can remain focused on BLE scanning without depending on a phone application remaining open or receiving unrestricted background execution from the phone operating system.

This does not mean every Bluetooth-scanning app is insecure. Different applications have different privacy models, background-operation limitations, and operating-system restrictions.

This project instead provides a separate, inspectable hardware detector with local processing.

What Is Bluetooth Low Energy?

Bluetooth Low Energy, normally abbreviated BLE, is the low-power part of the Bluetooth standard.

BLE was designed to allow devices to exchange relatively small amounts of information while using substantially less energy than continuously active conventional radio communication.

This makes BLE particularly useful in products that are:

Physically small

Battery powered

Expected to run for long periods

Limited in available battery capacity

Wearable

Portable

Sensor based

Required to send only small amounts of information

Common BLE applications include:

Wearables

Smart watches

Earbuds

Headphones

Sensors

Fitness equipment

Medical accessories

Smart-home equipment

Electronic tags

Smart glasses

Cameras and accessories

Wireless audio equipment

BLE is not limited to small devices, but low power consumption is one of the main reasons it is widely used in compact battery-powered electronics.

How BLE Advertising Works

Bluetooth Low Energy uses a mechanism called advertising.

Advertising is a fundamental part of BLE discovery and connectionless communication.

A BLE device that wants to announce its presence or make itself available for discovery can transmit short advertising packets without first establishing a connection.

Another BLE receiver can therefore hear those advertisements without:

Pairing

Connecting

Authenticating

Accessing the device

Opening a GATT connection

Activating a camera

Activating a microphone

Retrieving stored files

Advertising is normal BLE behaviour and is not caused by this detector.

However, not every BLE device advertises continuously.

Products may:

Advertise only under certain conditions

Advertise intermittently

Stop advertising

Change advertising mode

Remove identifying fields

Change payload contents after firmware updates

Advertising Timing

Normal BLE advertising does not necessarily occur every few milliseconds.

Traditional BLE advertising intervals can commonly range from approximately:

20 milliseconds

to:

10.24 seconds

depending on the device and configuration.

Legacy advertising also includes a small pseudo-random delay of up to approximately:

0–10 milliseconds

between advertising events.

This helps reduce repeated radio collisions when many BLE devices share the same 2.4 GHz spectrum.

A manufacturer may choose a shorter interval for fast discovery or a longer interval to reduce power consumption.

What Can a BLE Advertisement Contain?

Depending on the product, BLE advertisements may expose characteristics such as:

Advertised name

Bluetooth Company Identifier

Manufacturer-specific data

Service UUIDs

Service data

Address information

Address type

Manufacturer-specific patterns

Product-specific patterns

Capability information

The detector compares combinations of these characteristics against its rule and reference databases.

The detector does not need to:

Pair with the nearby device

Connect to it

Access its operating system

Retrieve files

Activate its camera

Activate its microphone

What About Devices That Change or Hide Their BLE Address?

Modern BLE devices often use randomised, private, or rotating Bluetooth addresses.

A changing BLE address does not automatically defeat these detectors.

The detection engines are deliberately designed not to rely exclusively on a permanent BLE MAC address.

They can also examine characteristics such as:

Advertised name

Company ID

Manufacturer data

Service UUIDs

Service data

Manufacturer patterns

Known product signatures

Keywords

Combinations of several signals

For example:

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

The second advertisement can still be evaluated using its other observable BLE characteristics.

This is important because BLE address rotation is itself a legitimate privacy feature.

However, there are limits.

If a product:

Rotates its address

Removes its name

Randomises its manufacturer data

Changes UUID or service information

Obfuscates useful payload fields

Stops advertising

then classification may become weaker or impossible.

The project does not claim to defeat every possible BLE privacy, randomisation, encryption, or obfuscation method.

There Is No Permanent Universal BLE Signature

There is no single permanent BLE signature that every BLE device is guaranteed to continuously transmit.

Devices may:

Rotate addresses

Stop advertising

Change advertised names

Change payload contents

Change behaviour after firmware updates

Advertise only occasionally

Provide too little identifying information

Use changing or encrypted payloads

Operate without BLE

This project should therefore be treated as an additional privacy-awareness system, not as a guarantee that every smart-glasses, camera, microphone, or recording device will be detected.

Which Version Should I Build?

Version

Cost / Availability

Skill Level

Intended Use

ESP32-S3 Lite v1.1

Slightly more expensive and sometimes harder to source

Very little practical skill

Simplest daily portable/travel detector

ESP32-WROOM v1.1

Cheapest and widely available

Some soldering and practical knowledge

Budget daily portable/travel detector

CYD DEV Lite

More expensive and larger

Some technical knowledge

Schools, childcare, offices, healthcare, aged care, facilities and fixed deployment

CYD Expanded DEV

Highest cost and complexity

Advanced technical/development skill

Highest-capability stationary detector and development platform

Requirements Common to Every Build

Every version requires:

A computer

Arduino IDE

A suitable USB data cable

The correct Espressif ESP32 board package

The libraries required by that firmware package

A charging-only USB cable will not work for programming.

Arduino IDE and the USB cable are programming/setup requirements rather than assembly tools.

Firmware Updating and Partitions

Current public builds are intended to be programmed directly over USB.

OTA firmware updating is not part of the normal public update workflow.

Firmware changes are normally installed by:

Connecting the ESP32 to a computer

Opening the correct Arduino project

Compiling the firmware

Uploading it over USB

Partition layouts can vary between builds because each detector uses flash differently.

Always use the partition configuration supplied or specified for the firmware being flashed.

Do not substitute another partition scheme unless you understand why it is being changed.

Examples:

S3 Lite uses its supplied sketch-local partitions.csv

WROOM uses its verified normal/default partition layout

CYD builds should use the configuration supplied with their final package

The Expanded database ESP should use the verified application-space configuration supplied for that firmware

Where a build specifically uses a configuration such as:

No OTA / approximately 2 MB application / filesystem space

follow the build-specific instructions rather than assuming that setting applies universally.

General Assembly Tools

Where assembly is required:

Soldering iron

Wire cutters

Wire strippers

Small screwdriver set

Tweezers

Multimeter

Heat-shrink or electrical tape if required

Helping hands / PCB holder if required

The ESP32-S3 Lite v1.1 requires no assembly tools.

ESP32-S3 Lite v1.1

STATUS: WORKING

DIFFICULTY: EASIEST

INTENDED USE: DAILY PORTABLE / TRAVEL

ASSEMBLY TOOLS REQUIRED: NONE

The ESP32-S3 Lite is intended to be the easiest entry point into the project.

Parts

1 × ESP32-S3 board

Programming Requirements

Computer

Arduino IDE

Arduino-ESP32 3.3.11

USB data cable

Correct ESP32-S3 board profile

Serial Monitor: 115200 baud

Upload speed: 921600 baud where reliable

Fallback upload speed: 460800 baud

OTA updating: not used

Partitioning: use the supplied sketch-local partitions.csv

No soldering, wire cutting, or other assembly work is normally required.

Portable Phone-Powered Design

The S3 is designed to be mounted behind a compatible smartphone and powered using a short USB-C-to-USB-C cable of approximately 10 cm.

Practical design estimate:

Approximately 20% additional phone battery use over an 8-hour day.

This is an estimate, not a guaranteed specification.

Actual battery use will vary according to:

Phone model

Battery condition

USB behaviour

Exact S3 board

LED activity

BLE environment

Scanning conditions

ESP32-S3 LED States

Blue — Boot

The detector is starting.

Green — Normal Function

Normal private BLE scanning is operating.

Recommended action:

Continue normally.

Green means the detector is functioning normally and no orange or red alert condition has been reached.

Green does not prove that the surrounding area is free from cameras, microphones, smart glasses, or recording equipment.

Orange — Be Aware

A BLE advertisement has matched a configured possible-device rule.

Recommended action:

Be aware that a device matching one or more relevant BLE characteristics may be nearby.

Remain aware of your surroundings and consider the context.

Orange is a reason for awareness, not alarm.

It does not establish:

Exact device identity

Exact distance

Ownership

Whether a camera is active

Whether a microphone is active

Whether recording is occurring

Red — Strong Alert

A BLE advertisement has reached a stronger smart-glasses or camera/audio classification.

Recommended action:

Be aware that a strongly matching device is highly likely to be nearby.

Increase awareness of the surrounding environment.

Red does not provide an exact distance measurement.

BLE range varies according to:

Transmit power

Antenna design

Walls

Human-body attenuation

RF interference

Device orientation

Local environment

Red still does not prove:

Exact distance

Ownership

Camera activity

Microphone activity

Recording

Concealment

Malicious intent

Unlawful behaviour

Flashing Purple — Privacy Setup Failure / Retry

Private-address setup has failed and the detector is retrying.

Solid Purple — Public-Address Fallback

After repeated private-address failures, the firmware may enter a clearly indicated public-address fallback mode.

The user should never be led to believe private mode is still operating when it is not.

ESP32-S3 Data Flow

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

ESP32-WROOM v1.1

STATUS: WORKING

DIFFICULTY: MODERATE

INTENDED USE: DAILY PORTABLE / TRAVEL

The WROOM provides the lowest-cost version of the detector using inexpensive and widely available classic ESP32 hardware.

It requires more practical skill than the S3.

Parts

1 × ESP32-WROOM development board

1 × WS2812B / NeoPixel LED

Approximately 30 cm of wire

Solder

Flux

Tools

Soldering iron

Wire cutters

Wire strippers

Small screwdriver set

Tweezers

Multimeter

Heat-shrink or electrical tape if required

Helping hands / PCB holder if required

Programming Requirements

Computer

Arduino IDE

Arduino-ESP32 3.3.11

USB data cable

Board: ESP32 Dev Module

Upload speed: 115200 baud

Serial Monitor: 115200 baud

BLE backend: Bluedroid

Flash Mode: DIO where applicable

OTA updating: not used

Partition Scheme: normal/default ESP32 Dev Module scheme

No custom partitions.csv

External library: Adafruit NeoPixel

Important Hardware Note

The current WROOM v1.1 is:

WS2812B / NeoPixel only.

It does not use an OLED display.

NeoPixel data pin:

GPIO 4

Portable Phone-Powered Design

The WROOM is also designed for daily portable use behind a compatible smartphone.

Practical design estimate:

Approximately 20% additional phone battery use over an 8-hour day.

Actual consumption varies between hardware and phones.

WROOM LED Guidance

The WROOM uses the same basic portable status approach.

LED

Meaning

Recommended Response

Blue

Boot/startup

Wait for startup

Green

Normal private scanning

Continue normally

Orange

Possible relevant device profile

Be aware; a matching device may be nearby

Red

Strong/high-confidence match

Increase awareness; a strongly matching device is likely nearby

Purple

Scanner privacy-address issue

Check privacy status

LED alerts provide radio-awareness information.

They do not prove recording, ownership, intent, illegality, or exact physical distance.

ESP32-WROOM Data Flow

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

CYD Hardware Procurement Note

The two development CYDs obtained from the referenced vendor arrived with SparkleIoT ESP32 radio modules that included onboard U.FL / IPEX connectors.

This appears to be variable hardware stock. Seller photographs may show a standard ESP32 module, so another buyer is not guaranteed to receive the SparkleIoT/IPEX-equipped version.

For an external-antenna build:

Confirm the delivered board physically has the U.FL / IPEX connector.

Do not assume the seller will supply the same radio-module revision.

Even if the connector is present, the 0-ohm RF selector resistor must still be moved / resoldered to route RF to U.FL / IPEX on the development configuration described in this project.

Required U.FL / IPEX Resistor Change Procedure

This resistor move is required for the external-antenna CYD configuration. It is not an optional range modification. Disconnect all power before soldering.

Disconnect USB and every other power source from the CYD.

Locate the ESP32 radio module, the onboard PCB antenna, and the U.FL / IPEX socket.

Locate the tiny 0-ohm RF antenna-selector resistor in the antenna-routing area.

Identify the resistor position that currently routes the ESP32 RF feed to the onboard PCB antenna.

Using a fine-tip soldering iron, tweezers, and magnification, remove the 0-ohm resistor from the PCB-antenna position.

Move/resolder the same 0-ohm resistor into the selector position that routes the ESP32 RF feed to the U.FL / IPEX connector.

On a three-pad selector layout, the resistor must bridge the common RF-feed pad to the U.FL / IPEX path only. Do not bridge both antenna paths.

Inspect the area carefully for solder bridges, lifted pads, or other damage.

Connect the external antenna to the U.FL / IPEX socket.

Restore power only after the selector resistor has been moved and the external antenna has been connected.

If the 0-ohm resistor is still in the PCB-antenna position, plugging an antenna into the U.FL / IPEX socket does not select the external antenna.

CYD/module revisions can differ. Identify the RF selector and the correct antenna-routing pads on the exact board received before moving the resistor.

CYD DEV Lite

STATUS: WORKING / HARDWARE VALIDATED

BEST FOR: FACILITIES AND FIXED LOCATIONS

CYD DEV Lite is the stable advanced standalone detector.

It adds a graphical display, SD storage, detailed controls, manual data capture, and Sentry Mode.

Suitable authorised deployments include:

Childcare facilities

After-school care

Schools

School administration

School technology departments

Offices

Reception areas

Front desks

Healthcare facilities

Aged-care facilities

Secure facilities

Correctional administration areas

Facility entry points

Long-term BLE research

Database development

Parts

1 × ESP32-2432S028R CYD

1 × CYD case

1 × 3 dBi 2.4 GHz U.FL / IPEX antenna

1 × SD card

⚠️ CRITICAL CYD EXTERNAL-ANTENNA MODIFICATION

THE 0-OHM RF ANTENNA-SELECTOR RESISTOR MUST BE MOVED / RESOLDERED BEFORE THE EXTERNAL U.FL / IPEX ANTENNA WILL BE THE ACTIVE RF PATH.

On the SparkleIoT/IPEX-equipped CYD boards used during development, the presence of the U.FL / IPEX socket does not mean the external antenna is automatically connected to the ESP32 radio.

The tiny 0-ohm RF selector resistor must be physically moved from the PCB-antenna routing position to the U.FL / IPEX routing position.

If this resistor is not changed, plugging an external antenna into the IPEX socket does not select that antenna. The radio remains routed to the onboard PCB antenna.

This resistor change is therefore a required hardware modification for the external-antenna CYD configuration.

Disconnect power before soldering. Fine SMD soldering skills and magnification are strongly recommended. Component placement may vary between CYD revisions, so identify the selector on the exact board received before moving it.

Power

A suitable USB-C wall adapter or fixed USB power source is recommended.

Programming Requirements

Computer

Arduino IDE

Arduino-ESP32 3.3.11

USB data cable

Target: ESP32-2432S028R CYD

Serial Monitor: 115200 baud

OTA updating: not used

Use the partition configuration supplied with the final CYD package

Hardware

Display:

ILI9341

320 × 240

Landscape

Rotation 3

TFT:

MISO  -> GPIO 12
MOSI  -> GPIO 13
SCLK  -> GPIO 14
CS    -> GPIO 15
DC    -> GPIO 2
RST   -> -1
BL    -> GPIO 21

RGB LED:

GPIO 4
GPIO 16
GPIO 17

BOOT:

GPIO 0

Main Features

CYD DEV Lite includes:

2.8-inch graphical display

BLE smart-glasses detection

LOW / POSSIBLE / HIGH rule scoring

CAM AND AUDIO classification

Suspicious/development-device review

Known-device annotation

SD logging

Manual BLE context capture

SYS STAT

Self test

Sentry Mode

Rotating private BLE scanner address

Session-scoped pseudonymous device hashes

CYD DEV Lite Confidence Levels

LOW — 1 to 59

Low-confidence evidence is primarily a clue, annotation, or reference signal.

LOW does not by itself produce a smart-glasses warning.

POSSIBLE — 60 to 96

Orange warning.

The advertisement matched a configured smart-glasses-related rule strong enough to warrant user attention.

HIGH — 97 to 100

Red warning.

The advertisement matched a configured rule reaching the firmware's HIGH presentation threshold.

A score is a rule-match score.

It should not be interpreted as:

“There is a 97% probability this is smart glasses.”

CYD Sentry Mode

Sentry Mode is primarily designed for long-term unattended or lightly attended deployment.

It is particularly useful for:

Environmental BLE data collection

Long-duration facility monitoring

Schools and institutional environments

Field research

Baseline building

Database development

Signature research

False-positive refinement

Understanding BLE activity over time

Sentry is not ESP32 deep sleep.

The detector remains operational while the screen can remain dark and unobtrusive.

The standard configuration waits approximately:

30 minutes

between ordinary measurements.

Each measurement lasts approximately:

10 seconds.

What Sentry Measures

Sentry counts unique BLE device identities within the measurement window.

It does not simply count every advertisement.

RSSI and estimated distance do not define the Sentry activity metric.

Baseline Building

The activity threshold is based on the greater of:

150% of the existing baseline

Baseline + 5 devices

A sample must be strictly above the threshold to trigger active learning.

Active Learning

When significant activity is detected, Sentry enters a fixed:

60-minute ACTIVE learning period

During that period, repeated 10-second BLE windows are collected.

The 60-minute timer is fixed.

Additional activity does not continually restart or extend it.

Classification Continues

During Sentry scan periods, normal classification can continue using:

Smart-glasses rules

Camera/audio rules

Known-device rules

Suspicious/development-device rules

False-positive handling

Relevant observation logging can also continue.

Sentry Privacy

Sentry does not create permanent identities for nearby devices.

Current configuration supports up to:

100 pseudonymous identities for Sentry session/dashboard statistics

400 unique identities within a measurement window

A Sentry trigger means:

The BLE environment changed significantly.

It does not mean:

A camera, microphone, smart-glasses device, or “spy” entered the area.

CYD DEV Lite Data Flow

                     NEARBY BLE DEVICES
                            |
                            v
                +-------------------------+
                | BLE ADVERTISEMENTS      |
                +------------+------------+
                             |
                             v
                +-------------------------+
                | CYD BLE SCANNER         |
                |                         |
                | PRIVATE NRPA            |
                | random rotation         |
                | ~12-18 minutes          |
                +------------+------------+
                             |
                             v
                +-------------------------+
                | LOCAL DETECTION ENGINE  |
                |                         |
                | HIGH / MEDIUM / LOW     |
                | camera / audio          |
                | known devices           |
                | suspicious devices      |
                | false positives         |
                +------------+------------+
                             |
       +---------------------+----------------------+
       |                     |                      |
       v                     v                      v
+---------------+    +------------------+    +--------------------+
| TFT / RGB     |    | PRIVACY PROCESS  |    | SENTRY MODE        |
|               |    |                  |    |                    |
| LOW           |    | raw MAC          |    | long-term          |
| POSSIBLE      |    | transient only   |    | data collection    |
| HIGH          |    |       |          |    |       |            |
| CAM / AUDIO   |    |       v          |    |       v            |
+---------------+    | session hash     |    | baseline building  |
                     +--------+---------+    +---------+----------+
                              |                        |
                              +------------+-----------+
                                           |
                                           v
                                +----------------------+
                                | SD OBSERVATION LOGS  |
                                |                      |
                                | observations         |
                                | detections           |
                                | manual captures      |
                                | sessions             |
                                | session pseudonyms   |
                                |                      |
                                | no raw BLE MAC       |
                                | no persistent device |
                                | identity             |
                                +----------------------+

Manual CYD BLE Context Capture

CYD DEV Lite can capture:

5 observations before

5 observations after

for a total of:

10 observations

This provides context around an event of interest for later database analysis.

The same privacy rules apply.

Cameras, Microphones, and Consumer “Bugs”

All detector families are intended to notify the user when BLE advertisements resemble known or suspected:

Cameras

Hidden-camera products

Microphones

Wireless microphones

Recording devices

Consumer surveillance devices

Consumer “bug”-type devices

The database can use combinations of:

Bluetooth Company IDs

Advertised names

Manufacturer data

Service UUIDs

Product-specific patterns

Manufacturer-specific signatures

Keywords

Documented manufacturer information

Field-derived observations

The project has a particular interest in consumer recording and surveillance products commonly available in Australia.

Products such as RØDE microphones may be added where a specific BLE signature has actually been captured and verified.

A specific brand or product should not be described as supported by a firmware release unless its corresponding rule is actually present in that firmware.

A camera/audio alert means:

A nearby BLE advertisement resembles a known or suspected camera, microphone, or recording-device profile.

It does not prove that the device is hidden, active, or recording.

CYD Expanded DEV

STATUS: WORKING / EXPERIMENTAL / MOSTLY UNTESTED

CAPABILITY: HIGHEST

INTENDED USE: STATIONARY / DEVELOPMENT

CYD Expanded DEV is the highest-capability version of the current project.

It combines:

ESP32-2432S028R CYD

External 8 dBi U.FL / IPEX antenna

Required 0-ohm RF antenna-selector resistor modification to route the CYD radio to U.FL / IPEX

SD storage

Separate ESP32 database coprocessor

Local CYD rules

Expanded database architecture

Asynchronous UART database lookups

Future/planned Wi-Fi development

Potential authorised environments include:

Schools

School IT departments

Healthcare facilities

Aged-care facilities

Front offices

Reception areas

Secure facilities

Correctional facilities

Controlled-access locations

Development laboratories

It is intended to provide the greatest practical BLE reception capability of the current builds.

Actual range depends heavily on:

Target-device transmit power

Antenna installation

Walls

Obstructions

RF interference

Building construction

Antenna orientation

Local RF conditions

No fixed detection distance should be guaranteed without direct measurement.

Parts

1 × ESP32-2432S028R CYD with onboard U.FL / IPEX-capable radio hardware

1 × CYD case

1 × 8 dBi U.FL / IPEX antenna

1 × additional ESP32 database coprocessor

1 × SD card

Approximately 30 cm wire

Solder

Flux

⚠️ MANDATORY BEFORE USING THE 8 dBi ANTENNA

THE CYD'S 0-OHM RF ANTENNA-SELECTOR RESISTOR MUST BE CHANGED.

The resistor must be moved / resoldered from the PCB-antenna position to the U.FL / IPEX position.

The external 8 dBi antenna will not become the selected RF path simply because it is plugged into the U.FL / IPEX socket.

If the 0-ohm selector resistor is left in the PCB-antenna position, the ESP32 radio remains routed to the onboard PCB antenna.

For the CYD Expanded DEV hardware described here, moving this resistor is a required assembly step, not an optional range modification.

Disconnect power before soldering, identify the selector on the exact board revision received, move the 0-ohm resistor to the external-antenna path, inspect for solder bridges, and only then connect/use the 8 dBi antenna.

Power

A suitable fixed USB/wall power source is required.

CYD Expanded External-Antenna Hardware Check

Before flashing or commissioning CYD Expanded DEV, verify all of the following:

The CYD actually has an onboard U.FL / IPEX connector.

The delivered radio-module revision is suitable for external-antenna routing.

The 0-ohm RF selector resistor has been physically moved / resoldered from the PCB-antenna route to the U.FL / IPEX route.

The solder work has been inspected for bridges or damage.

The 8 dBi antenna is connected only after the RF selector has been changed.

If Step 3 has not been completed, the external antenna is not the selected RF path.

Important: CYD Expanded Uses Two Firmware Packages

CYD Expanded DEV — 1 of 2

Flash to:

ESP32-2432S028R CYD

This is the main:

BLE scanner

Local detection engine

Display

User interface

Privacy engine

Logging system

CYD Expanded DEV — 2 of 2

Flash to:

Separate ESP32 database coprocessor

This is the expanded database-processing half.

1 OF 2  ----------------->  ESP32-2432S028R CYD

2 OF 2  ----------------->  SEPARATE ESP32
                             DATABASE COPROCESSOR

Do not reverse them.

CYD Expanded Programming Requirements

Computer

Arduino IDE

Arduino-ESP32 3.3.11

USB data cable for each ESP32

OTA updating: not used

CYD Serial Monitor: 115200 baud

CYD-to-database UART: 460800 baud, 8N1

Database ESP: classic ESP32 Dev Module

Database flash target: approximately 4 MB

No PSRAM required

Use the build-specific partition configuration supplied with the final firmware

CYD Expanded Architecture

                         NEARBY BLE DEVICE
                                |
                                v
                      +---------------------+
                      | BLE ADVERTISEMENT   |
                      +----------+----------+
                                 |
                                 v
             +------------------------------------+
             | CYD EXPANDED DEV — 1 OF 2         |
             | ESP32-2432S028R                    |
             |                                    |
             | BLE scanning                       |
             | privacy handling                   |
             | local rules                        |
             | UI                                 |
             | alerts                             |
             | session hashing                    |
             | SD observation logging             |
             +----------------+-------------------+
                              |
                 LOCAL RULES REMAIN
                    AUTHORITATIVE
                              |
                +-------------+-------------+
                |                           |
                v                           v
     +----------------------+    +-------------------------+
     | LOCAL CYD DETECTION  |    | UART DATABASE LOOKUP    |
     |                      |    |                         |
     | smart glasses        |    | framed binary protocol  |
     | camera/audio         |    | CRC integrity check     |
     | known devices        |    | 460800 baud / 8N1       |
     | false positives      |    +------------+------------+
     +----------+-----------+                 |
                |                             v
                |              +----------------------------+
                |              | CYD EXPANDED DEV — 2 OF 2 |
                |              | ESP32 DATABASE COPROCESSOR |
                |              |                            |
                |              | expanded Company IDs       |
                |              | extended rule tables       |
                |              | reference information      |
                |              | classification lookup      |
                |              +-------------+--------------+
                |                            |
                |                      DATABASE RESULT
                |                            |
                +---------------+------------+
                                |
                                v
                    +-------------------------+
                    | CYD FINAL DECISION      |
                    |                         |
                    | local CYD rules remain  |
                    | authoritative           |
                    | database supplements    |
                    +------------+------------+
                                 |
                    +------------+-------------+
                    |                          |
                    v                          v
          +------------------+       +-----------------------+
          | TFT / LED ALERT  |       | SESSION / SD          |
          |                  |       | OBSERVATIONS          |
          +------------------+       |                       |
                                     | pseudonymous          |
                                     | no raw BLE MAC        |
                                     | no persistent device  |
                                     | identity              |
                                     +-----------------------+

The external database is supplementary.

If the database processor becomes unavailable, the CYD continues local BLE scanning and local rule evaluation.

CYD Expanded UART

Baud: 460800

Format: 8N1

CYD TX: GPIO 22

CYD RX: GPIO 27

Database ESP RX: GPIO 16

Database ESP TX: GPIO 17

CYD GPIO22 TX  ---------->  Database ESP GPIO16 RX

CYD GPIO27 RX  <----------  Database ESP GPIO17 TX

CYD GND        -----------  Database ESP GND

The protocol uses framed binary messages and CRC16 integrity checking.

Raw observed BLE addresses may transiently pass through the lookup protocol where required for matching.

They must not be persistently stored as observed-device identities.

CYD Expanded Busy-Environment Mode

The Expanded build includes a database-servicing strategy for busy BLE environments.

Current design:

Rolling 60-second activity window

Busy mode enters when unique devices are strictly greater than 20 per minute

Busy mode exits when activity is strictly below 15

Five-advertisement median RSSI

Stronger/nearer requests are serviced before weaker/farther requests

Weaker/farther observations are not discarded

Local CYD rules continue normally

Busy mode changes database service order only

Busy mode does not change local confidence scoring

Privacy by Design

Privacy is a core engineering requirement.

The detector does not need access to:

Contacts

Photos

Messages

User accounts

Phone location history

Phone camera

Phone microphone

The detector itself does not need, do or use:

A camera

A microphone

A companion app

Wi-Fi for BLE operation

Cloud processing

No Persistent Observed-Device Identity

This is a fundamental project privacy rule.

No observed BLE device is ever intentionally assigned a persistent identity across sessions.

An observed raw BLE MAC address may exist transiently in RAM while an advertisement is processed.

It may be used transiently for:

Exact-rule matching

OUI extraction

Deduplication

Session correlation

Producing a session-scoped pseudonym

Raw observed BLE MAC addresses are not intentionally written into persistent operational logs for legal reasons (such as complying with the NSW Device Surveillance Act 2007/ case law and OIAC reccomendations)

Where correlation is useful, a session-scoped value similar to:

MAC-HASH-7A3F91C2D4E5F607

may be used.

The session salt:

Is randomly generated

Exists only for that session

Is not intentionally persisted

Changes between sessions

The same BLE address should therefore normally receive a different pseudonym in a later session.

Persistent files may contain historical:

Observations

Classifications

Counters

Session-scoped pseudonyms

That does not make the observed-device identity persistent.

Pseudonymisation Is Not Anonymity

A session-scoped pseudonym is a privacy measure, not a guarantee of complete anonymity.

Other BLE characteristics can sometimes distinguish a device, including:

Advertised names

Manufacturer payloads

Stable payload identifiers

Service UUID combinations

This limitation should be considered when handling exported data.

Protecting the Detector's Own BLE Identity

Where supported, the firmware uses a private scanner-originating BLE address rather than continuously exposing one fixed public address during active scanning.

A fresh private address is generated at a random interval of approximately:

12–18 minutes

Rotation occurs while scanning is idle.

This reduces exposure of a stable scanner identity and makes straightforward tracking using one fixed BLE address more difficult.

It does not make the detector invisible or undetectable.

What Happens if Private BLE Setup Fails?

CYD DEV Lite — Fail Closed

If the private NRPA cannot be established and verified:

Active BLE scanning remains blocked

Firmware retries private-address setup

It does not silently continue normal active scanning using its public identity

This is deliberate fail-closed behaviour.

ESP32-S3 / ESP32-WROOM

Portable builds make privacy problems visible.

Flashing purple: private setup failed and retrying.

Solid purple: clearly indicated public-address fallback where supported.

Detection Database Development

The database combines different forms of evidence.

Public documentation uses categories such as:

CONFIRMED

DOCUMENTED

FIELD DERIVED

REFERENCE ONLY

EXPERIMENTAL

Confirmed / Documented

Can include:

Official advertised names

Confirmed manufacturer fingerprints

Documented manufacturer information

Documented service information

Strong multi-signal combinations

Physically confirmed observations

Field Derived

Part of the database was manually developed and refined through approximately two weeks of BLE signature collection and real-world field testing in Sydney, Australia.

That work has been used to:

Identify ordinary BLE products

Reduce false positives

Identify headphones and speakers

Identify commercial equipment

Investigate smart-glasses advertisements

Examine camera/audio candidates

Refine known-device annotation

Test behaviour in busy BLE environments

Reference Only

Examples include:

Bluetooth Company Identifiers

Service UUID assignments

Manufacturer reference data

A Company ID identifies an assigned organisation.

It does not prove the physical product type.

Experimental

Some rules remain exploratory and may be based on:

Advertised names

Product/model names

Incomplete public information

User-supplied information

Unverified BLE characteristics

Experimental rules should not be presented as equivalent to confirmed device fingerprints.

Known Kmart / Anko Camera Glasses Example

One documented product-specific example is:

Item: 43700141

Model: JLR-82067

Documented Bluetooth name: Anko43700141

The refined rule uses the exact advertised name as a strong product-specific clue.

An advertised name can potentially be imitated, so it should not be described as cryptographic proof of identity.

Known Limitations and Blind Spots

A device may be missed if:

It does not use BLE

BLE is disabled

It is not advertising

It uses Wi-Fi only

Its BLE signature is unknown

Its advertisements change

Its firmware changes

It exposes insufficient information

Its address and payload are heavily randomised

RF interference prevents reception

It is outside practical BLE range

It deliberately imitates another device

Its signature has not been added

Legitimate equipment such as:

Cameras

Microphones

Dashcams

Headphones

Development boards

Smart-home equipment

may also advertise BLE characteristics overlapping with camera/audio or development-device categories.

This project is an additional privacy-awareness tool.

It is not a guarantee that an area is free from:

Cameras

Microphones

Recording equipment

Consumer bugs

Smart glasses

Advanced covert surveillance equipment

Flashing Overview

Install Arduino IDE.

Install the correct Espressif ESP32 board package.

Place the .ino and required project files together.

Connect using a USB data cable.

Select the correct board.

Select the correct serial port.

Confirm the required partition configuration.

Confirm the required upload speed.

Select Verify.

Resolve compile errors.

Select Upload.

Allow flashing to complete.

Restart if required.

Confirm expected LED/display behaviour.

S3 Flashing Summary

Arduino-ESP32: 3.3.11

Serial Monitor: 115200

Upload: 921600

Fallback Upload: 460800

OTA: not used

Partition: supplied partitions.csv

Expected startup:

BLUE
 |
 v
GREEN

WROOM Flashing Summary

Board: ESP32 Dev Module

Arduino-ESP32: 3.3.11

BLE: Bluedroid

Upload: 115200

Serial: 115200

Flash mode: DIO where applicable

Partition: normal/default

OTA: not used

External library: Adafruit NeoPixel

No OLED libraries required

Expected startup:

BLUE
 |
 v
GREEN

CYD DEV Lite Flashing Summary

Target:

ESP32-2432S028R CYD

Arduino-ESP32: 3.3.11

Serial: 115200

OTA: not used

SD card recommended/required for normal logging functionality

Use the released CYD partition configuration

After flashing:

TFT initialises.

Startup/legal interface appears.

Private BLE setup completes.

Scanning becomes available.

Main interface appears.

Check SD status.

Use SYS STAT to verify operation.

CYD Expanded Flashing Warning

DO NOT FLASH BOTH FIRMWARE HALVES TO THE SAME BOARD.

EXTERNAL ANTENNA REQUIREMENT: the CYD's 0-ohm RF antenna-selector resistor must be moved / resoldered to the U.FL / IPEX position before the 8 dBi external antenna is used. If the resistor remains in the PCB-antenna position, connecting the 8 dBi antenna does not route the radio through it.

CYD EXPANDED DEV — 1 OF 2
        |
        +------> ESP32-2432S028R CYD

CYD EXPANDED DEV — 2 OF 2
        |
        +------> SEPARATE ESP32
                 DATABASE COPROCESSOR

Updating the Bluetooth Company ID Database

CYD Expanded DEV 2 of 2 includes:

generate_company_db.py

The script generates:

company_ids_generated.h

macOS

python3 --version
cd /path/to/ESP32_BLE_DATABASE_COPROCESSOR_v1
python3 generate_company_db.py

Using local JSON:

python3 generate_company_db.py --input company_ids.json

Windows

py --version
cd "C:\path\to\ESP32_BLE_DATABASE_COPROCESSOR_v1"
py generate_company_db.py

Linux

python3 --version
cd /path/to/ESP32_BLE_DATABASE_COPROCESSOR_v1
python3 generate_company_db.py

After regenerating company_ids_generated.h:

Recompile CYD Expanded DEV 2 of 2

Reflash the database ESP

The CYD does not need to be reflashed solely because this generated header changed. After WiFi capabilities are added this process might change to auto update the database on know networks.

Troubleshooting Decision Tree

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

Contributing a New BLE Signature

Suggested submission format:

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

Please do not submit another person's captured raw BLE MAC address.

The strongest submissions are:

Captured from equipment you own

Captured from equipment you can positively identify

Repeated observations

Physically confirmed

Supported by documentation where possible

Version Maturity

WORKING

Reported operational on physical hardware.

HARDWARE VALIDATED

Compiled, flashed, and tested against the intended hardware and major functionality.

EXPERIMENTAL

Development firmware that operates but still requires broader validation.

PLANNED

A feature or design direction that is not currently part of the released firmware.

Build

Maturity

ESP32-S3 Lite v1.1

WORKING

ESP32-WROOM v1.1

WORKING

CYD DEV Lite

WORKING / HARDWARE VALIDATED

CYD Expanded DEV — 1 of 2

WORKING / EXPERIMENTAL / MOSTLY UNTESTED

CYD Expanded DEV — 2 of 2

WORKING / EXPERIMENTAL / MOSTLY UNTESTED

CYD Expanded Wi-Fi scanning

PLANNED / EXPERIMENTAL

Real-World Development and Testing

The project has been refined through real-world BLE field testing rather than only simulated data.

Part of the database was developed through approximately two weeks of BLE collection and field testing in Sydney, Australia.

Testing has included:

Public transport

Shopping environments

Offices

Commercial areas

High-density BLE environments

Future Development Roadmap

Planned development includes:

Additional verified smart-glasses signatures

Additional camera/audio signatures

Australian consumer surveillance-device research

Additional verified microphone signatures

Continued field testing

False-positive reduction

Improved database provenance

Expanded Company ID/reference information

Continued CYD Expanded validation

Database-coprocessor improvements

Community-submitted signatures

Improved installation documentation

Improved troubleshooting documentation

Future Wi-Fi scanning

Migration of proven CYD features into S3/WROOM

Wi-Fi Roadmap

Wi-Fi scanning is planned/experimental for CYD Expanded DEV.

The intended direction is for Wi-Fi observations to supplement BLE evidence rather than replace BLE detection.

Current BLE-only releases should not be described as already providing completed Wi-Fi detection.

Legal and Responsible Use

Bluetooth and radio-monitoring laws vary between countries and jurisdictions.

Whether a particular use is lawful may depend on:

What information is collected

How long it is retained

Where the detector is deployed

Who operates it

Purpose of deployment

Privacy obligations

Workplace policies

School policies

Healthcare policies

Correctional-facility rules

Surveillance legislation

Users are responsible for ensuring deployment is lawful and authorised.

CYD External-Antenna Safety Reminder

For CYD DEV Lite and CYD Expanded DEV builds using the external antenna configuration:

THE 0-OHM RF ANTENNA-SELECTOR RESISTOR MUST BE CHANGED.

The resistor must be moved / resoldered from the onboard PCB-antenna routing position to the U.FL / IPEX routing position.

Connecting an external antenna without changing this resistor does not select the external antenna.

Always disconnect power before soldering and verify the exact board revision and selector position before modification.

Final Project Principle

This project is not intended to create fear around Bluetooth devices, cameras, microphones, or smart glasses.

Its purpose is to give people additional information about the radio environment around them.

The detector cannot determine another person's intentions.

It cannot determine whether recording is occurring.

It cannot prove that a camera is active.

It cannot prove that a microphone is active.

It cannot establish that somebody is acting unlawfully.

It can observe BLE radio characteristics and report when those characteristics resemble known or suspected device profiles.

This project detects radio characteristics, not people. Its purpose is privacy awareness, not surveillance.
It cannot tell if a camera or microphone is currently recording. Treat alerts as information requiring context, not proof of wrongdoing.
