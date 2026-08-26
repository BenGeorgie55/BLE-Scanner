# CYD Expanded DEV — 1 of 2

**Status:** WORKING / EXPERIMENTAL / MOSTLY UNTESTED  
**Target:** ESP32-2432S028R CYD

> **1 OF 2 -> CYD**

The CYD performs BLE scanning, local authoritative matching, UI, alerting and logging. Supplementary lookups are sent to the database ESP32.

UART:
- 460800 baud, 8N1
- CYD GPIO22 TX -> DB ESP GPIO16 RX
- CYD GPIO27 RX <- DB ESP GPIO17 TX
- common GND

Local CYD rules remain authoritative.
