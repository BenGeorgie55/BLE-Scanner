#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <stdint.h>
#define LITE_COUNTER_WINDOW_BOOTS 5U
#define LITE_COUNTER_NVS_VERSION 1U
#define LITE_COUNTER_CHECKPOINT_MS (5UL * 60UL * 1000UL)
enum LiteCounterKind : uint8_t {
    LITE_COUNT_BLUE_BOOT=0, LITE_COUNT_GREEN_PRIVATE_READY, LITE_COUNT_ORANGE_POSSIBLE, LITE_COUNT_RED_HIGH_GLASSES, LITE_COUNT_RED_CAMERA_AUDIO, LITE_COUNT_PURPLE_PRIVATE_FAIL, LITE_COUNT_PURPLE_PUBLIC_MODE
};
struct LiteCounterSnapshot {
    uint8_t bootWindow;
    uint32_t blueBoot,greenPrivateReady,orangePossible,redHighGlasses,redCameraAudio,purplePrivateFail,purplePublicMode;
};
static Preferences liteCounterPrefs;
static bool liteCounterPrefsReady=false, liteCounterDirty=false;
static uint32_t liteCounterLastCheckpointMs=0;
static portMUX_TYPE liteCounterMux=portMUX_INITIALIZER_UNLOCKED;
static LiteCounterSnapshot liteCounterState= {
};
static inline void liteCounterSaturatingIncrement(uint32_t& v) {
    if(v!=UINT32_MAX)++v;
}
static inline LiteCounterSnapshot liteCountersSnapshot() {
    LiteCounterSnapshot c;
    portENTER_CRITICAL(&liteCounterMux);
    c=liteCounterState;
    portEXIT_CRITICAL(&liteCounterMux);
    return c;
}
static inline bool liteCountersAreDirty() {
    bool d;
    portENTER_CRITICAL(&liteCounterMux);
    d=liteCounterDirty;
    portEXIT_CRITICAL(&liteCounterMux);
    return d;
}
static inline void liteCountersMarkDirty() {
    portENTER_CRITICAL(&liteCounterMux);
    liteCounterDirty=true;
    portEXIT_CRITICAL(&liteCounterMux);
}
static inline bool liteCountersPersistNow() {
    if(!liteCounterPrefsReady)return false;
    LiteCounterSnapshot s;
    portENTER_CRITICAL(&liteCounterMux);
    s=liteCounterState;
    liteCounterDirty=false;
    portEXIT_CRITICAL(&liteCounterMux);
    bool ok=true;
    ok&=(liteCounterPrefs.putUChar("ver",LITE_COUNTER_NVS_VERSION)==1);
    ok&=(liteCounterPrefs.putUChar("boots",s.bootWindow)==1);
    ok&=(liteCounterPrefs.putUInt("blue",s.blueBoot)==sizeof(uint32_t));
    ok&=(liteCounterPrefs.putUInt("green",s.greenPrivateReady)==sizeof(uint32_t));
    ok&=(liteCounterPrefs.putUInt("orange",s.orangePossible)==sizeof(uint32_t));
    ok&=(liteCounterPrefs.putUInt("rhigh",s.redHighGlasses)==sizeof(uint32_t));
    ok&=(liteCounterPrefs.putUInt("rcam",s.redCameraAudio)==sizeof(uint32_t));
    ok&=(liteCounterPrefs.putUInt("pfail",s.purplePrivateFail)==sizeof(uint32_t));
    ok&=(liteCounterPrefs.putUInt("pub",s.purplePublicMode)==sizeof(uint32_t));
    liteCounterLastCheckpointMs=millis();
    if(!ok)liteCountersMarkDirty();
    return ok;
}
static inline bool liteCountersBegin() {
    liteCounterPrefsReady=liteCounterPrefs.begin("wrmlitecnt",false);
    LiteCounterSnapshot s= {
    };
    if(liteCounterPrefsReady) {
        uint8_t ver=liteCounterPrefs.getUChar("ver",0);
        if(ver==LITE_COUNTER_NVS_VERSION) {
            s.bootWindow=liteCounterPrefs.getUChar("boots",0);
            s.blueBoot=liteCounterPrefs.getUInt("blue",0);
            s.greenPrivateReady=liteCounterPrefs.getUInt("green",0);
            s.orangePossible=liteCounterPrefs.getUInt("orange",0);
            s.redHighGlasses=liteCounterPrefs.getUInt("rhigh",0);
            s.redCameraAudio=liteCounterPrefs.getUInt("rcam",0);
            s.purplePrivateFail=liteCounterPrefs.getUInt("pfail",0);
            s.purplePublicMode=liteCounterPrefs.getUInt("pub",0);
        }
        else {
            liteCounterPrefs.clear();
        }
    }
    if(s.bootWindow>=LITE_COUNTER_WINDOW_BOOTS)s= {
    };
    s.bootWindow++;
    portENTER_CRITICAL(&liteCounterMux);
    liteCounterState=s;
    liteCounterDirty=true;
    portEXIT_CRITICAL(&liteCounterMux);
    if(liteCounterPrefsReady)liteCountersPersistNow();
    return liteCounterPrefsReady;
}
static inline void liteCountersIncrement(LiteCounterKind k) {
    portENTER_CRITICAL(&liteCounterMux);
    switch(k) {
        case LITE_COUNT_BLUE_BOOT:liteCounterSaturatingIncrement(liteCounterState.blueBoot);
        break;
        case LITE_COUNT_GREEN_PRIVATE_READY:liteCounterSaturatingIncrement(liteCounterState.greenPrivateReady);
        break;
        case LITE_COUNT_ORANGE_POSSIBLE:liteCounterSaturatingIncrement(liteCounterState.orangePossible);
        break;
        case LITE_COUNT_RED_HIGH_GLASSES:liteCounterSaturatingIncrement(liteCounterState.redHighGlasses);
        break;
        case LITE_COUNT_RED_CAMERA_AUDIO:liteCounterSaturatingIncrement(liteCounterState.redCameraAudio);
        break;
        case LITE_COUNT_PURPLE_PRIVATE_FAIL:liteCounterSaturatingIncrement(liteCounterState.purplePrivateFail);
        break;
        case LITE_COUNT_PURPLE_PUBLIC_MODE:liteCounterSaturatingIncrement(liteCounterState.purplePublicMode);
        break;
    }
    liteCounterDirty=true;
    portEXIT_CRITICAL(&liteCounterMux);
}
static inline void liteCountersCheckpointIfDue(uint32_t now) {
    if(!liteCounterPrefsReady||!liteCountersAreDirty())return;
    if((uint32_t)(now-liteCounterLastCheckpointMs)<LITE_COUNTER_CHECKPOINT_MS)return;
    liteCountersPersistNow();
}
static inline uint32_t liteCountersTotalAlerts(const LiteCounterSnapshot&s) {
    uint64_t t=(uint64_t)s.orangePossible+s.redHighGlasses+s.redCameraAudio;
    return t>UINT32_MAX?UINT32_MAX:(uint32_t)t;
}
static inline void liteCountersPrintSerial(const char* mode) {
    LiteCounterSnapshot s=liteCountersSnapshot();
    Serial.println("ESP32 WROOM");
    Serial.printf("MODE: %s\n",mode?mode:"STARTING");
    Serial.printf("COUNTER WINDOW: %u / %u BOOTS\n",(unsigned)s.bootWindow,(unsigned)LITE_COUNTER_WINDOW_BOOTS);
    Serial.printf("BLUE_BOOT: %lu\n",(unsigned long)s.blueBoot);
    Serial.printf("GREEN_PRIVATE_READY: %lu\n",(unsigned long)s.greenPrivateReady);
    Serial.printf("ORANGE_POSSIBLE: %lu\n",(unsigned long)s.orangePossible);
    Serial.printf("RED_HIGH_GLASSES: %lu\n",(unsigned long)s.redHighGlasses);
    Serial.printf("RED_CAMERA_AUDIO: %lu\n",(unsigned long)s.redCameraAudio);
    Serial.printf("PURPLE_PRIVATE_FAIL: %lu\n",(unsigned long)s.purplePrivateFail);
    Serial.printf("PURPLE_PUBLIC_MODE: %lu\n",(unsigned long)s.purplePublicMode);
    Serial.printf("TOTAL_ALERT_TRIGGERS: %lu\n\n",(unsigned long)liteCountersTotalAlerts(s));
}
