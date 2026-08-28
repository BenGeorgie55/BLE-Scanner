#pragma once
#include <stdint.h>

/*
 * ESP32 S3 Glasses Scanner Lite v1
 * CONSUMER CAMERA / AUDIO REVIEW RULES
 *
 * Every active rule below reports a 90% REVIEW match and drives the urgent
 * full-brightness RED STROBE in the Lite consumer UI.
 * 90% means confidence that the BLE advertisement matches this rule; it is NOT a
 * 90% probability that a device is hidden, recording, malicious, or surveilling.
 *
 * Deliberately avoids a generic "camera", "mic", or "audio" substring because
 * those would create excessive false positives in ordinary consumer electronics.
 */

enum CameraAudioNameMode : uint8_t {
    CAMERA_AUDIO_CONTAINS = 0,
    CAMERA_AUDIO_PREFIX   = 1,
    CAMERA_AUDIO_EXACT    = 2
};

enum CameraAudioCategory : uint8_t {
    CAMERA_AUDIO_CAMERA = 0,
    CAMERA_AUDIO_MICROPHONE,
    CAMERA_AUDIO_CAMERA_MIC,
    CAMERA_AUDIO_RECORDER
};

struct CameraAudioRule {
    const char* label;
    CameraAudioCategory category;
    const char* namePattern;
    CameraAudioNameMode nameMode;
    uint8_t confidence;
    const char* reason;
};

#define CAMERA_AUDIO_ALERT_CONFIDENCE 90
#define CAMERA_AUDIO_ALERT_MS         10000UL

static const CameraAudioRule CAMERA_AUDIO_RULES[] = {
    { "CAMERA / AUDIO DEVICE", CAMERA_AUDIO_CAMERA_MIC, "CAM-", CAMERA_AUDIO_PREFIX, 90,
      "CAM- provisioning-name clue used by camera/audio products" },
    { "ESP32-CAM DEVICE", CAMERA_AUDIO_CAMERA, "ESP32-CAM", CAMERA_AUDIO_CONTAINS, 90,
      "Explicit ESP32-CAM advertised-name clue" },
    { "M5 CAMERA DEVICE", CAMERA_AUDIO_CAMERA, "M5Camera", CAMERA_AUDIO_CONTAINS, 90,
      "Explicit M5Camera advertised-name clue" },

    { "POSSIBLE HIDDEN CAMERA", CAMERA_AUDIO_CAMERA, "Hidden Camera", CAMERA_AUDIO_CONTAINS, 90,
      "Advertised name explicitly states Hidden Camera" },
    { "POSSIBLE SPY CAMERA", CAMERA_AUDIO_CAMERA, "Spy Camera", CAMERA_AUDIO_CONTAINS, 90,
      "Advertised name explicitly states Spy Camera" },
    { "POSSIBLE MINI CAMERA", CAMERA_AUDIO_CAMERA, "Mini Camera", CAMERA_AUDIO_CONTAINS, 90,
      "Advertised name explicitly states Mini Camera" },
    { "POSSIBLE IP CAMERA", CAMERA_AUDIO_CAMERA, "IP Camera", CAMERA_AUDIO_CONTAINS, 90,
      "Advertised name explicitly states IP Camera" },
    { "POSSIBLE IP CAMERA", CAMERA_AUDIO_CAMERA, "IPCAM", CAMERA_AUDIO_CONTAINS, 90,
      "IPCAM advertised-name clue" },
    { "POSSIBLE BLUETOOTH CAMERA", CAMERA_AUDIO_CAMERA, "Bluetooth Camera", CAMERA_AUDIO_CONTAINS, 90,
      "Advertised name explicitly states Bluetooth Camera" },

    { "POSSIBLE VOICE RECORDER", CAMERA_AUDIO_RECORDER, "Voice Recorder", CAMERA_AUDIO_CONTAINS, 90,
      "Advertised name explicitly states Voice Recorder" },
    { "POSSIBLE AUDIO RECORDER", CAMERA_AUDIO_RECORDER, "Audio Recorder", CAMERA_AUDIO_CONTAINS, 90,
      "Advertised name explicitly states Audio Recorder" },
    { "POSSIBLE BLUETOOTH RECORDER", CAMERA_AUDIO_RECORDER, "Bluetooth Recorder", CAMERA_AUDIO_CONTAINS, 90,
      "Advertised name explicitly states Bluetooth Recorder" },
    { "POSSIBLE WIRELESS MIC", CAMERA_AUDIO_MICROPHONE, "Wireless Mic", CAMERA_AUDIO_CONTAINS, 90,
      "Advertised name explicitly states Wireless Mic" },
    { "POSSIBLE BLUETOOTH MIC", CAMERA_AUDIO_MICROPHONE, "Bluetooth Mic", CAMERA_AUDIO_CONTAINS, 90,
      "Advertised name explicitly states Bluetooth Mic" },
    { "POSSIBLE MICROPHONE", CAMERA_AUDIO_MICROPHONE, "MIC-", CAMERA_AUDIO_PREFIX, 90,
      "MIC- advertised-name prefix" },

    { nullptr, CAMERA_AUDIO_CAMERA, nullptr, CAMERA_AUDIO_CONTAINS, 0, nullptr }
};
