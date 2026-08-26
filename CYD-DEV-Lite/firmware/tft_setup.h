#pragma once


/*
 * ========================= USER-EDITABLE HARDWARE SETUP =================
 * Change these pin/driver values only if you move to a different CYD/display
 * revision. The current values are the proven ESP32-2432S028R ILI9341 setup.
 * Keep this file beside the sketch as the readable hardware reference.
 * =======================================================================
 */
// ESP32-2432S028R / CYD ILI9341 display setup.
// Sketch-local TFT_eSPI configuration.
//
// Display bus:
//   MISO 12
//   MOSI 13
//   SCLK 14
//   CS   15
//   DC    2
//   RST  -1
//   BL   21
//
// Display uses HSPI.
// SD card remains on VSPI separately.

#define USER_SETUP_LOADED
#define USER_SETUP_INFO "CYD ILI9341 240x320 HSPI"

#define ILI9341_DRIVER

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1

#define TFT_BL 21
#define TFT_BACKLIGHT_ON HIGH

#define LOAD_GLCD
#define LOAD_FONT2

#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  16000000

#define USE_HSPI_PORT
#define SUPPORT_TRANSACTIONS
