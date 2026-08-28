#pragma once

/*
  =============================================================================
  USER-EDITABLE CYD DEV EXPANDED 1 SELF TEST
  =============================================================================

  IMPORTANT MAINTENANCE RULE
  --------------------------
  THIS SELF TEST MIRRORS THE SCANNER'S REAL OPERATIONAL FUNCTIONS.

  If you change an operational LED state, POSSIBLE banner, CAM AND AUDIO
  warning, suspicious/review warning, HIGH alert tile, confidence threshold/
  presentation, alert layout, or related UI behaviour in
  the main sketch, YOU MUST ALSO UPDATE THIS FILE so the diagnostic continues
  to test the same behaviour the scanner actually uses.

  Do not add legacy/demo effects here unless they also exist operationally.
  In particular, v6.2 has no operational purple, slow-blink, fast-blink, rapid,
  or strobe alert modes, so this self test must not test those effects.

  The test deliberately uses the real operational helpers supplied by the main
  sketch (rgbBlue/rgbGreen/rgbOrange/rgbRed, approximateDistanceMetres,
  warningGrid, drawWarningTile, etc.) rather than maintaining duplicate copies.

  Fake self-test devices are display-only. They are not logged, counted,
  deduplicated, tracked, or placed into the real detection cooldown.
  =============================================================================
*/

// -----------------------------------------------------------------------------
// USER-EDITABLE TIMING
// -----------------------------------------------------------------------------
#define SELF_TEST_LED_STEP_MS      2000UL
#define SELF_TEST_BANNER_STEP_MS   3000UL
#define SELF_TEST_REVIEW_STEP_MS   3500UL
#define SELF_TEST_WARNING_STEP_MS  1800UL
#define SELF_TEST_RESULT_MS        2000UL
#define SELF_TEST_CLOSE_MS          700UL
#define SELF_TEST_INTRO_MS         1200UL

// -----------------------------------------------------------------------------
// USER-EDITABLE EXIT WARNING
// -----------------------------------------------------------------------------
// This warning is intentionally drawn on EVERY self-test screen.
// If the BOOT/exit behaviour changes operationally, update this text and the
// corresponding self-test control logic here at the same time.
#define SELF_TEST_EXIT_TEXT        "PRESS BOOT TO EXIT SELF TEST"
#define SELF_TEST_EXIT_BAR_Y       218
#define SELF_TEST_EXIT_BAR_H       22

// -----------------------------------------------------------------------------
// SELF-TEST STATE
// -----------------------------------------------------------------------------
bool selfTestActive = false;
bool selfTestClosing = false;
uint8_t selfTestStep = 0;
uint32_t selfTestStepStartedMs = 0;

// -----------------------------------------------------------------------------
// SELF-TEST DRAWING HELPERS
// -----------------------------------------------------------------------------
void drawSelfTestExitWarning() {
    // Persistent, high-visibility instruction on every diagnostic screen.
    tft.fillRect(0, SELF_TEST_EXIT_BAR_Y, 320, SELF_TEST_EXIT_BAR_H, TFT_YELLOW);
    tft.setTextColor(TFT_BLACK, TFT_YELLOW);
    tft.setTextSize(1);
    tft.setCursor(72, SELF_TEST_EXIT_BAR_Y + 7);
    tft.print(SELF_TEST_EXIT_TEXT);
}

void drawSelfTestMessage(const char* title, const char* subtitle = nullptr) {
    if (!takeUi(100)) return;

    tft.fillScreen(TFT_BLACK);
    tft.setTextWrap(false);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(12, 65);
    tft.print(title);

    if (subtitle && subtitle[0]) {
        tft.setTextSize(1);
        tft.setCursor(12, 105);
        tft.print(subtitle);
    }

    if (selfTestActive) {
        drawSelfTestExitWarning();
    }

    giveUi();
}

void fillSelfTestWarningDevice(WarningDevice& d, uint8_t index) {
    d.deviceMacHash = "TEST";
    d.name = "TEST DEVICE " + String(index + 1);
    d.product = "SELF TEST";
    d.cameraStatus = "TEST";
    d.rssi = -55 - (int)index;
    d.confidence = HIGH_ALERT_CONFIDENCE;
    d.expiresAt = millis() + 60000UL;
}

void drawSelfTestPossibleBanner() {
    if (!takeUi(100)) return;

    tft.fillScreen(TFT_BLACK);
    tft.setTextWrap(false);

    // Mirrors the operational v6.2 60..96 POSSIBLE presentation.
    tft.fillRect(0, 0, 320, 54, TFT_ORANGE);
    tft.setTextColor(TFT_BLACK, TFT_ORANGE);
    tft.setTextSize(2);
    tft.setCursor(6, 4);
    tft.print("POSSIBLE GLASSES 85%");

    tft.setTextSize(1);
    tft.setCursor(6, 27);
    tft.print("SELF TEST POSSIBLE MATCH");
    tft.setCursor(6, 41);
    tft.printf("RSSI -72 dBm   ~%.1f m", approximateDistanceMetres(-72));

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(12, 92);
    tft.print("ORANGE LED + POSSIBLE BANNER");

    drawSelfTestExitWarning();

    giveUi();
}

void showSelfTestHighWarnings(uint8_t count) {
    if (count > MAX_WARNING_DEVICES) count = MAX_WARNING_DEVICES;

    warningDeviceCount = count;
    for (uint8_t i = 0; i < count; i++) {
        fillSelfTestWarningDevice(warningDevices[i], i);
    }

    // Mirrors the operational 97..100 red HIGH warning grid renderer.
    // warningScreenActive remains false so test devices never enter real alert
    // lifetime/logging behaviour.
    if (!takeUi(100)) return;

    tft.fillScreen(TFT_BLACK);
    tft.setTextWrap(false);

    tft.fillRect(0, 0, 320, 43, TFT_RED);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.setTextSize(2);
    tft.setCursor(7, 5);
    tft.print("SELF TEST HIGH 97%");

    tft.setTextSize(1);
    tft.setCursor(8, 29);
    tft.printf("%u TEST DEVICE%s", (unsigned)count, count == 1 ? "" : "S");

    const int left = 4;
    const int right = 316;
    const int top = 47;
    const int bottom = 214;
    const int gap = 3;

    uint8_t cols, rows;
    warningGrid(count, cols, rows);

    int areaW = right - left;
    int areaH = bottom - top;
    int tileW = (areaW - gap * (cols - 1)) / cols;
    int tileH = (areaH - gap * (rows - 1)) / rows;

    for (uint8_t i = 0; i < count; i++) {
        int col = i % cols;
        int row = i / cols;
        if (row >= rows) break;
        int x = left + col * (tileW + gap);
        int y = top + row * (tileH + gap);
        drawWarningTile(warningDevices[i], i + 1, x, y, tileW, tileH);
    }

    drawSelfTestExitWarning();

    giveUi();
}

void clearSelfTestWarnings() {
    for (uint8_t i = 0; i < MAX_WARNING_DEVICES; i++) {
        clearWarningDevice(warningDevices[i]);
    }
    warningDeviceCount = 0;
}

// -----------------------------------------------------------------------------
// SELF-TEST CONTROL
// -----------------------------------------------------------------------------
void startSelfTest() {
    if (selfTestActive || warningScreenActive || micCamAlert.active || suspiciousAlert.active) return;
    if (currentPage != PAGE_STATUS) return;

    selfTestActive = true;
    selfTestClosing = false;
    selfTestStep = 0;
    selfTestStepStartedMs = millis();
    clearSelfTestWarnings();

    drawSelfTestMessage("SELF TEST", "Mirrors CYD Dev Expanded 1 operational functions");
}

void finishSelfTest() {
    clearSelfTestWarnings();
    selfTestActive = false;
    selfTestClosing = false;
    selfTestStep = 0;
    rgbGreen();
    currentPage = PAGE_STATUS;
    showStatusPage();
}

// Called by a REAL qualifying warning before the real alert is drawn.
void abortSelfTestForRealWarning() {
    if (!selfTestActive) return;

    clearSelfTestWarnings();
    selfTestClosing = true;
    drawSelfTestMessage("CLOSING TEST", "REAL WARNING DETECTED");

    uint32_t started = millis();
    while ((uint32_t)(millis() - started) < SELF_TEST_CLOSE_MS) {
        delay(5);
    }

    selfTestActive = false;
    selfTestClosing = false;
    selfTestStep = 0;
    rgbGreen();
}

uint32_t selfTestStepDuration(uint8_t step) {
    // BLUE and GREEN operational LED states.
    if (step == 1 || step == 2) return SELF_TEST_LED_STEP_MS;

    // Operational 60..96 orange POSSIBLE banner + LED.
    if (step == 3) return SELF_TEST_BANNER_STEP_MS;

    // Separate orange CAM AND AUDIO warning, then generic suspicious review.
    if (step == 4 || step == 5) return SELF_TEST_REVIEW_STEP_MS;

    // Operational 97..100 red HIGH tiled layouts: 1, 2, 4, 9, 20 devices.
    if (step >= 6 && step <= 10) return SELF_TEST_WARNING_STEP_MS;

    if (step == 11) return SELF_TEST_RESULT_MS;

    // Intro screen before step 1.
    return SELF_TEST_INTRO_MS;
}

void enterSelfTestStep(uint8_t step) {
    selfTestStep = step;
    selfTestStepStartedMs = millis();

    switch (selfTestStep) {
        case 1:
            clearSelfTestWarnings();
            ledLogicalState = LED_STATE_TEST_BLUE;
            rgbBlue();
            drawSelfTestMessage("BLUE LED", "Boot / setup indicator");
            break;

        case 2:
            ledLogicalState = LED_STATE_TEST_GREEN;
            rgbGreen();
            drawSelfTestMessage("GREEN LED", "Clear / normal scanning indicator");
            break;

        case 3:
            ledLogicalState = LED_STATE_TEST_ORANGE;
            rgbOrange();
            drawSelfTestPossibleBanner();
            break;

        case 4:
            ledLogicalState = LED_STATE_TEST_ORANGE;
            rgbOrange();
            drawMicCamAlertScreen("CAMERA / AUDIO DEVICE", "CAMERA / AUDIO");
            if (takeUi(100)) {
                drawSelfTestExitWarning();
                giveUi();
            }
            break;

        case 5:
            ledLogicalState = LED_STATE_TEST_ORANGE;
            rgbOrange();
            drawSuspiciousAlertScreen("POSSIBLE ESP32 DEV BOARD", "DIY / DEV");
            if (takeUi(100)) {
                drawSelfTestExitWarning();
                giveUi();
            }
            break;

        case 6:
            ledLogicalState = LED_STATE_TEST_RED;
            rgbRed();
            showSelfTestHighWarnings(1);
            break;

        case 7:
            rgbRed();
            showSelfTestHighWarnings(2);
            break;

        case 8:
            rgbRed();
            showSelfTestHighWarnings(4);
            break;

        case 9:
            rgbRed();
            showSelfTestHighWarnings(9);
            break;

        case 10:
            rgbRed();
            showSelfTestHighWarnings(20);
            break;

        case 11:
            clearSelfTestWarnings();
            rgbGreen();
            drawSelfTestMessage("SELF TEST PASSED", "CYD Dev Expanded 1 UI + LED test complete");
            break;

        default:
            finishSelfTest();
            break;
    }
}

void updateSelfTest() {
    if (!selfTestActive || selfTestClosing) return;

    uint32_t now = millis();
    uint32_t duration = selfTestStepDuration(selfTestStep);
    if ((uint32_t)(now - selfTestStepStartedMs) < duration) return;

    enterSelfTestStep(selfTestStep + 1);
}
