#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <esp_system.h>
#include <stdint.h>
#include <string.h>

#define LITE11_FIRMWARE_NAME "ESP32 Wroom NeoPixel"
#define LITE11_LOG_FORMAT_VERSION 1U
#define LITE11_MAX_SESSION_RECORDS 256U
#define LITE11_RECORD_CHECKPOINT_MS (5UL * 60UL * 1000UL)
#define LITE11_MIN_TARGET_FS_BYTES (1536UL * 1024UL)
#define LITE11_COMPANY_ID_NONE 0xFFFFU

enum Lite11PrivacyMode:uint8_t {
    LITE11_PRIVACY_STARTING=0,LITE11_PRIVACY_PRIVATE_NRPA=1,LITE11_PRIVACY_PUBLIC_FALLBACK=2
};
enum Lite11RecordSource:uint8_t {
    LITE11_SOURCE_GLASSES=1,LITE11_SOURCE_CAMERA_AUDIO=2
};
enum Lite11ClassFlags:uint8_t {
    LITE11_CLASS_GLASSES=0x01,LITE11_CLASS_CAMERA_AUDIO=0x02,LITE11_CLASS_FALSE_POSITIVE_SUPPRESSED=0x04
};
enum Lite11AlertFlags:uint8_t {
    LITE11_ALERT_NONE=0,LITE11_ALERT_POSSIBLE_GLASSES=0x01,LITE11_ALERT_HIGH_GLASSES=0x02,LITE11_ALERT_CAMERA_AUDIO=0x04
};
struct Lite11ObservationInput {
    uint64_t deviceHash;
    uint32_t uptimeMs;
    int8_t rssi;
    uint16_t companyId;
    uint16_t relevantUuid16;
    uint8_t confidence;
    uint8_t tier;
    uint8_t source;
    uint8_t classFlags;
    uint8_t alertFlags;
    bool falsePositiveSuppressed;
    const char* classification;
    const char* signature;
};
struct Lite11SessionHeader {
    uint32_t magic,sessionId,bootCount,resetReason,recordCount,alertEpisodes,droppedCandidates,privateFailuresThisSession;
    uint8_t privacyMode;
    uint8_t reserved[15];
};
struct Lite11CandidateRecord {
    uint32_t magic,sessionId;
    uint64_t deviceHash;
    uint32_t firstSeenUptimeMs,lastSeenUptimeMs,observationCount;
    int8_t strongestRssi,weakestRssi;
    uint16_t companyId,relevantUuid16;
    uint8_t currentConfidence,highestConfidence,tier,source,classFlags,alertFlags,falsePositiveSuppressed,reserved;
    char classification[32];
    char signature[24];
};
struct Lite11ResetRecord {
    uint32_t magic,bootCount,newSessionId,previousSessionId,resetReason;
    uint8_t expectedRestart,faultLike;
    uint8_t reserved[2];
};
static constexpr uint32_t LITE11_SESSION_MAGIC=0x4C533131UL,LITE11_RECORD_MAGIC=0x52433131UL,LITE11_RESET_MAGIC=0x52533131UL;
static Preferences lite11Prefs;
static bool lite11PrefsReady=false,lite11FsReady=false,lite11Dirty=false,lite11Urgent=false;
static uint32_t lite11SessionIdValue=0,lite11BootCountValue=0,lite11LastFlushMs=0;
static uint64_t lite11Salt=0;
static Lite11SessionHeader lite11Header= {
};
static Lite11CandidateRecord lite11Records[LITE11_MAX_SESSION_RECORDS];
static uint16_t lite11RecordCountRam=0;
static portMUX_TYPE lite11LogMux=portMUX_INITIALIZER_UNLOCKED;
static inline void lite11Copy(char*d,size_t n,const char*s) {
    if(!d||n==0)return;
    if(!s)s="";
    strncpy(d,s,n-1);
    d[n-1]='\0';
}
static inline bool lite11FaultLike(esp_reset_reason_t r) {
    return r==ESP_RST_PANIC||r==ESP_RST_INT_WDT||r==ESP_RST_TASK_WDT||r==ESP_RST_WDT||r==ESP_RST_BROWNOUT;
}
static inline String lite11SessionPath(uint32_t sid) {
    char p[24];
    snprintf(p,sizeof(p),"/s%06lu.bin",(unsigned long)sid);
    return String(p);
}
static inline uint64_t lite11DeviceHashNative(const uint8_t* data,size_t len) {
    if(!data||len==0)return 0;
    uint64_t h=1469598103934665603ULL^lite11Salt;
    for(size_t i=0;i<len;i++) {
        h^=data[i];
        h*=1099511628211ULL;
    }
    h^=(uint64_t)lite11SessionIdValue*0x9E3779B97F4A7C15ULL;
    if(h==0)h=1;
    return h;
}
static inline bool lite11WriteCurrent() {
    if(!lite11FsReady)return false;
    String path=lite11SessionPath(lite11SessionIdValue);
    File f=LittleFS.open(path,"w");
    if(!f)return false;
    Lite11SessionHeader h;
    portENTER_CRITICAL(&lite11LogMux);
    h=lite11Header;
    h.recordCount=lite11RecordCountRam;
    portEXIT_CRITICAL(&lite11LogMux);
    bool ok=f.write((uint8_t*)&h,sizeof(h))==sizeof(h);
    for(uint16_t i=0;ok&&i<lite11RecordCountRam;i++)ok=f.write((uint8_t*)&lite11Records[i],sizeof(Lite11CandidateRecord))==sizeof(Lite11CandidateRecord);
    f.close();
    return ok;
}
static inline void lite11AppendReset(uint32_t prev,bool expected,esp_reset_reason_t reason) {
    if(!lite11FsReady)return;
    File f=LittleFS.open("/resets.bin",FILE_APPEND);
    if(!f)return;
    Lite11ResetRecord r= {
        LITE11_RESET_MAGIC,lite11BootCountValue,lite11SessionIdValue,prev,(uint32_t)reason,(uint8_t)(expected?1:0),(uint8_t)(lite11FaultLike(reason)?1:0), {
            0,0
        }
    };
    f.write((uint8_t*)&r,sizeof(r));
    f.close();
}
static inline bool lite11PersistentBegin() {
    lite11PrefsReady=lite11Prefs.begin("wrmlite11",false);
    uint32_t prev=0;
    bool expected=false;
    if(lite11PrefsReady) {
        prev=lite11Prefs.getUInt("sid",0);
        lite11BootCountValue=lite11Prefs.getUInt("boot",0)+1;
        lite11SessionIdValue=prev+1;
        expected=lite11Prefs.getBool("expected",false);
        lite11Prefs.putUInt("sid",lite11SessionIdValue);
        lite11Prefs.putUInt("boot",lite11BootCountValue);
        lite11Prefs.putBool("expected",false);
    }
    else {
        lite11BootCountValue=1;
        lite11SessionIdValue=1;
    }
    lite11Salt=((uint64_t)esp_random()<<32)|esp_random();
    if(lite11Salt==0)lite11Salt=0xA5A55A5A12345678ULL;
    lite11FsReady=LittleFS.begin(true);
    memset(&lite11Header,0,sizeof(lite11Header));
    lite11Header.magic=LITE11_SESSION_MAGIC;
    lite11Header.sessionId=lite11SessionIdValue;
    lite11Header.bootCount=lite11BootCountValue;
    lite11Header.resetReason=(uint32_t)esp_reset_reason();
    lite11Header.privacyMode=LITE11_PRIVACY_STARTING;
    lite11RecordCountRam=0;
    lite11Dirty=true;
    lite11Urgent=true;
    if(lite11FsReady) {
        lite11WriteCurrent();
        lite11AppendReset(prev,expected,esp_reset_reason());
    }
    return lite11FsReady;
}
static inline uint32_t lite11CurrentSessionId() {
    return lite11SessionIdValue;
}
static inline uint32_t lite11LifetimeBootCount() {
    return lite11BootCountValue;
}
static inline bool lite11StorageReady() {
    return lite11FsReady;
}
static inline void lite11PrepareExpectedRestart() {
    if(lite11PrefsReady)lite11Prefs.putBool("expected",true);
}
static inline void lite11NotePrivateStartSuccess() {
    if(lite11PrefsReady) {
        uint32_t v=lite11Prefs.getUInt("privOK",0);
        if(v!=UINT32_MAX)++v;
        lite11Prefs.putUInt("privOK",v);
    }
    portENTER_CRITICAL(&lite11LogMux);
    lite11Header.privacyMode=LITE11_PRIVACY_PRIVATE_NRPA;
    lite11Dirty=lite11Urgent=true;
    portEXIT_CRITICAL(&lite11LogMux);
}
static inline void lite11NotePrivateFailure() {
    if(lite11PrefsReady) {
        uint32_t v=lite11Prefs.getUInt("privFail",0);
        if(v!=UINT32_MAX)++v;
        lite11Prefs.putUInt("privFail",v);
    }
    portENTER_CRITICAL(&lite11LogMux);
    if(lite11Header.privateFailuresThisSession!=UINT32_MAX)++lite11Header.privateFailuresThisSession;
    lite11Dirty=lite11Urgent=true;
    portEXIT_CRITICAL(&lite11LogMux);
}
static inline void lite11NotePublicFallback() {
    if(lite11PrefsReady) {
        uint32_t v=lite11Prefs.getUInt("public",0);
        if(v!=UINT32_MAX)++v;
        lite11Prefs.putUInt("public",v);
    }
    portENTER_CRITICAL(&lite11LogMux);
    lite11Header.privacyMode=LITE11_PRIVACY_PUBLIC_FALLBACK;
    lite11Dirty=lite11Urgent=true;
    portEXIT_CRITICAL(&lite11LogMux);
}
static inline void lite11NoteAlertEpisode() {
    portENTER_CRITICAL(&lite11LogMux);
    if(lite11Header.alertEpisodes!=UINT32_MAX)++lite11Header.alertEpisodes;
    lite11Dirty=lite11Urgent=true;
    portEXIT_CRITICAL(&lite11LogMux);
}
static inline int16_t lite11FindRecordIndex(uint64_t h) {
    for(uint16_t i=0;i<lite11RecordCountRam;i++)if(lite11Records[i].deviceHash==h)return (int16_t)i;
    return -1;
}
static inline bool lite11ObserveCandidate(const Lite11ObservationInput& in) {
    if(in.deviceHash==0||in.confidence<40)return false;
    portENTER_CRITICAL(&lite11LogMux);
    int16_t found=lite11FindRecordIndex(in.deviceHash);
    bool significant=false;
    if(found<0) {
        if(lite11RecordCountRam>=LITE11_MAX_SESSION_RECORDS) {
            if(lite11Header.droppedCandidates!=UINT32_MAX)++lite11Header.droppedCandidates;
            lite11Dirty=true;
            portEXIT_CRITICAL(&lite11LogMux);
            return false;
        }
        uint16_t idx=lite11RecordCountRam++;
        Lite11CandidateRecord&r=lite11Records[idx];
        memset(&r,0,sizeof(r));
        r.magic=LITE11_RECORD_MAGIC;
        r.sessionId=lite11SessionIdValue;
        r.deviceHash=in.deviceHash;
        r.firstSeenUptimeMs=r.lastSeenUptimeMs=in.uptimeMs;
        r.observationCount=1;
        r.strongestRssi=r.weakestRssi=in.rssi;
        r.companyId=in.companyId;
        r.relevantUuid16=in.relevantUuid16;
        r.currentConfidence=r.highestConfidence=in.confidence;
        r.tier=in.tier;
        r.source=in.source;
        r.classFlags=in.classFlags;
        r.alertFlags=in.alertFlags;
        r.falsePositiveSuppressed=in.falsePositiveSuppressed?1:0;
        lite11Copy(r.classification,sizeof(r.classification),in.classification);
        lite11Copy(r.signature,sizeof(r.signature),in.signature);
        significant=true;
    }
    else {
        Lite11CandidateRecord&r=lite11Records[found];
        r.lastSeenUptimeMs=in.uptimeMs;
        if(r.observationCount!=UINT32_MAX)++r.observationCount;
        if(in.rssi>r.strongestRssi)r.strongestRssi=in.rssi;
        if(in.rssi<r.weakestRssi)r.weakestRssi=in.rssi;
        r.currentConfidence=in.confidence;
        if(in.confidence>r.highestConfidence) {
            r.highestConfidence=in.confidence;
            significant=true;
        }
        r.companyId=in.companyId;
        r.relevantUuid16=in.relevantUuid16;
        r.tier=in.tier;
        r.source=in.source;
        r.classFlags|=in.classFlags;
        if((in.alertFlags&~r.alertFlags)!=0)significant=true;
        r.alertFlags|=in.alertFlags;
        r.falsePositiveSuppressed|=in.falsePositiveSuppressed?1:0;
        lite11Copy(r.classification,sizeof(r.classification),in.classification);
        lite11Copy(r.signature,sizeof(r.signature),in.signature);
    }
    lite11Dirty=true;
    if(significant||in.alertFlags!=0)lite11Urgent=true;
    portEXIT_CRITICAL(&lite11LogMux);
    return true;
}
static inline bool lite11FlushPending(bool force) {
    if(!lite11FsReady||!lite11Dirty)return lite11FsReady;
    uint32_t now=millis();
    if(!force&&!lite11Urgent&&(uint32_t)(now-lite11LastFlushMs)<LITE11_RECORD_CHECKPOINT_MS)return true;
    bool ok=lite11WriteCurrent();
    if(ok) {
        portENTER_CRITICAL(&lite11LogMux);
        lite11Dirty=false;
        lite11Urgent=false;
        portEXIT_CRITICAL(&lite11LogMux);
        lite11LastFlushMs=now;
    }
    return ok;
}
static inline uint32_t lite11ResetRecordCount() {
    if(!lite11FsReady)return 0;
    File f=LittleFS.open("/resets.bin","r");
    if(!f)return 0;
    uint32_t n=f.size()/sizeof(Lite11ResetRecord);
    f.close();
    return n;
}
static inline void lite11DumpResetHistory() {
    Serial.println("RESET / BOOT HISTORY");
    if(!lite11FsReady) {
        Serial.println("UNAVAILABLE");
        return;
    }
    File f=LittleFS.open("/resets.bin","r");
    if(!f) {
        Serial.println("NONE");
        return;
    }
    Lite11ResetRecord r;
    while(f.read((uint8_t*)&r,sizeof(r))==sizeof(r)) {
        Serial.printf("BOOT %lu | SESSION %lu | PREV %lu | RESET_REASON %lu | EXPECTED %u | FAULTLIKE %u\n",(unsigned long)r.bootCount,(unsigned long)r.newSessionId,(unsigned long)r.previousSessionId,(unsigned long)r.resetReason,(unsigned)r.expectedRestart,(unsigned)r.faultLike);
    }
    f.close();
}
static inline uint32_t lite11DumpSession(uint32_t sid) {
    String p=lite11SessionPath(sid);
    File f=LittleFS.open(p,"r");
    if(!f)return 0;
    Lite11SessionHeader h;
    if(f.read((uint8_t*)&h,sizeof(h))!=sizeof(h)) {
        f.close();
        return 0;
    }
    Serial.printf("SESSION %lu | BOOT %lu | RECORDS %lu | ALERT_EPISODES %lu | PRIVACY %u | RESET %lu\n",(unsigned long)h.sessionId,(unsigned long)h.bootCount,(unsigned long)h.recordCount,(unsigned long)h.alertEpisodes,(unsigned)h.privacyMode,(unsigned long)h.resetReason);
    uint32_t n=0;
    Lite11CandidateRecord r;
    while(f.read((uint8_t*)&r,sizeof(r))==sizeof(r)) {
        n++;
        Serial.printf("  RECORD %lu | MAC-HASH-%016llX | CONF %u | HIGHEST %u | OBS %lu | RSSI %d..%d | CID 0x%04X | UUID 0x%04X | ALERT 0x%02X | FP %u | %s | %s\n",(unsigned long)n,(unsigned long long)r.deviceHash,(unsigned)r.currentConfidence,(unsigned)r.highestConfidence,(unsigned long)r.observationCount,(int)r.weakestRssi,(int)r.strongestRssi,(unsigned)r.companyId,(unsigned)r.relevantUuid16,(unsigned)r.alertFlags,(unsigned)r.falsePositiveSuppressed,r.classification,r.signature);
    }
    f.close();
    return n;
}
static inline void lite11DumpAll(const char* currentModeText) {
    lite11FlushPending(true);
    uint32_t privOK=lite11PrefsReady?lite11Prefs.getUInt("privOK",0):0,privFail=lite11PrefsReady?lite11Prefs.getUInt("privFail",0):0,pub=lite11PrefsReady?lite11Prefs.getUInt("public",0):0;
    Serial.println();
    Serial.println("============================================================");
    Serial.println("ESP32 WROOM — LOG DUMP");
    Serial.printf("FIRMWARE: %s\n",LITE11_FIRMWARE_NAME);
    Serial.printf("CURRENT_SESSION_ID: %lu\n",(unsigned long)lite11SessionIdValue);
    Serial.printf("LIFETIME_BOOT_COUNT: %lu\n",(unsigned long)lite11BootCountValue);
    Serial.printf("CURRENT_PRIVACY_MODE: %s\n",currentModeText?currentModeText:"UNKNOWN");
    Serial.printf("PRIVATE_START_SUCCESSES: %lu\n",(unsigned long)privOK);
    Serial.printf("PRIVATE_START_FAILURES: %lu\n",(unsigned long)privFail);
    Serial.printf("PUBLIC_FALLBACK_STARTS: %lu\n",(unsigned long)pub);
    Serial.printf("TOTAL_RESET_CRASH_RECORDS: %lu\n",(unsigned long)lite11ResetRecordCount());
    Serial.printf("LITTLEFS_STATUS: %s\n",lite11FsReady?"READY":"UNAVAILABLE");
    if(lite11FsReady) {
        Serial.printf("LITTLEFS_USED_BYTES: %lu\n",(unsigned long)LittleFS.usedBytes());
        Serial.printf("LITTLEFS_TOTAL_BYTES: %lu\n",(unsigned long)LittleFS.totalBytes());
    }
    Serial.println();
    lite11DumpResetHistory();
    Serial.println();
    Serial.println("SESSION-BY-SESSION >=40% DEDUPLICATED HISTORY");
    uint32_t total=0;
    if(lite11FsReady)for(uint32_t sid=1;sid<=lite11SessionIdValue;sid++)total+=lite11DumpSession(sid);
    Serial.printf("TOTAL_STORED_GE40_ENTRIES: %lu\n",(unsigned long)total);
    Serial.println("END LOG DUMP");
    Serial.println("============================================================\n");
}
