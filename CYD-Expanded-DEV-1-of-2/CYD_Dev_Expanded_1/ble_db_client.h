#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>
#include "ble_db_protocol.h"

// ============================================================================
// CYD BLE DATABASE COPROCESSOR CLIENT — USER-EDITABLE SETTINGS
// ============================================================================
// CN1 on the ESP32-2432S028R exposes GPIO22 and GPIO27. This client maps:
//   CYD GPIO22 (TX) -> database ESP RX
//   CYD GPIO27 (RX) <- database ESP TX
//   GND             -> database ESP GND
// Power the second ESP from an adequate supply; common ground is mandatory.
#define BLE_DB_UART_PORT              1
#define BLE_DB_UART_BAUD         460800UL
#define BLE_DB_UART_RX_PIN            27
#define BLE_DB_UART_TX_PIN            22

#define BLE_DB_REQUEST_QUEUE_LEN      24
#define BLE_DB_RESULT_QUEUE_LEN       12
#define BLE_DB_RECENT_CACHE_LEN       96
#define BLE_DB_REQUERY_MS          15000UL
#define BLE_DB_REQUEST_TIMEOUT_MS    350UL
#define BLE_DB_PING_INTERVAL_MS     1000UL
#define BLE_DB_OFFLINE_MS           4000UL
#define BLE_DB_STARTUP_GRACE_MS     6000UL
#define BLE_DB_TASK_STACK_BYTES     6144
#define BLE_DB_TASK_PRIORITY           1

// CYD Dev Expanded 1 busy-environment prioritisation. Busy mode is based on unique device
// identities seen in a rolling 60-second window. It changes ONLY the order in
// which external database lookups are serviced; it does not change local
// confidence, alerts, logging, or whether a device is considered a match.
#define BLE_DB_BUSY_ENTER_UNIQUE_PER_MIN  20
#define BLE_DB_BUSY_EXIT_UNIQUE_PER_MIN   15
#define BLE_DB_BUSY_WINDOW_MS          60000UL
#define BLE_DB_BUSY_RSSI_SAMPLES           5
#define BLE_DB_PRIORITY_BACKLOG_LEN BLE_DB_REQUEST_QUEUE_LEN
// ============================================================================

struct BleDbQueuedLookup {
    uint32_t localDeviceHash; // session-scoped CYD hash; never persisted here
    uint32_t queuedAtMs;      // RAM-only FIFO/tie-break timestamp
    int16_t  priorityRssi;    // smoothed RSSI used only for busy-mode ordering
    BleDbLookupPayload payload;
};

struct BleDbQueuedResult {
    uint32_t sequence;
    BleDbResultPayload payload;
};

struct BleDbRecentEntry {
    uint32_t localDeviceHash;
    uint32_t lastQueuedMs;
    uint32_t lastSeenMs;
    int8_t   rssiSamples[BLE_DB_BUSY_RSSI_SAMPLES];
    uint8_t  rssiSampleCount;
    uint8_t  rssiSampleNext;
    int16_t  smoothedRssi;
};

enum BleDbClientState : uint8_t {
    BLE_DB_CLIENT_STARTING = 0,
    BLE_DB_CLIENT_ONLINE,
    BLE_DB_CLIENT_FAILED
};

static HardwareSerial bleDbSerial(BLE_DB_UART_PORT);
static QueueHandle_t bleDbRequestQueue = nullptr;
static QueueHandle_t bleDbResultQueue = nullptr;
static TaskHandle_t bleDbTaskHandle = nullptr;
static BleDbRecentEntry bleDbRecent[BLE_DB_RECENT_CACHE_LEN] = {};
static BleDbFrameReceiver bleDbReceiver;
static BleDbQueuedLookup bleDbPriorityBacklog[BLE_DB_PRIORITY_BACKLOG_LEN] = {};
static uint8_t bleDbPriorityBacklogCount = 0;

static volatile bool bleDbBusyModeActive = false;
static volatile uint16_t bleDbBusyUniquePerMinute = 0;
static volatile uint32_t bleDbBusyLastObservationMs = 0;


static volatile uint32_t bleDbClientStartedMs = 0;
static volatile uint32_t bleDbLastValidRxMs = 0;
static volatile uint32_t bleDbQueriesQueued = 0;
static volatile uint32_t bleDbQueriesSent = 0;
static volatile uint32_t bleDbResultsReceived = 0;
static volatile uint32_t bleDbMatchedResults = 0;
static volatile uint32_t bleDbQueueDrops = 0;
static volatile uint32_t bleDbTimeouts = 0;
static volatile uint32_t bleDbCrcOrFrameRejects = 0;
static volatile bool bleDbStatusChangedFlag = false;

static char bleDbLastVendor[BLE_DB_VENDOR_LEN] = "";
static char bleDbLastProduct[BLE_DB_PRODUCT_LEN] = "";
static char bleDbLastDeviceType[BLE_DB_TYPE_LEN] = "";
static uint8_t bleDbLastClassification = BLE_DB_CLASS_UNKNOWN;
static uint8_t bleDbLastConfidenceSuggestion = 0;

inline void bleDbCopyText(char* destination, size_t destinationSize, const String& source) {
    if (destination == nullptr || destinationSize == 0) return;
    size_t copyLength = source.length();
    if (copyLength >= destinationSize) copyLength = destinationSize - 1;
    memcpy(destination, source.c_str(), copyLength);
    destination[copyLength] = '\0';
}

inline BleDbClientState bleDbClientState() {
    uint32_t now = millis();
    uint32_t started = bleDbClientStartedMs;
    uint32_t lastRx = bleDbLastValidRxMs;

    if (lastRx != 0 && (uint32_t)(now - lastRx) <= BLE_DB_OFFLINE_MS) {
        return BLE_DB_CLIENT_ONLINE;
    }
    if ((uint32_t)(now - started) < BLE_DB_STARTUP_GRACE_MS) {
        return BLE_DB_CLIENT_STARTING;
    }
    return BLE_DB_CLIENT_FAILED;
}

inline bool bleDbIsOnline() {
    return bleDbClientState() == BLE_DB_CLIENT_ONLINE;
}

inline const char* bleDbClientStateText() {
    switch (bleDbClientState()) {
        case BLE_DB_CLIENT_ONLINE: return "ONLINE";
        case BLE_DB_CLIENT_FAILED: return "FAIL";
        default: return "STARTING";
    }
}

inline bool bleDbConsumeStatusChanged() {
    bool changed = bleDbStatusChangedFlag;
    bleDbStatusChangedFlag = false;
    return changed;
}

inline int16_t bleDbMedianRssi(const int8_t* samples, uint8_t count) {
    if (samples == nullptr || count == 0) return -127;
    if (count > BLE_DB_BUSY_RSSI_SAMPLES) count = BLE_DB_BUSY_RSSI_SAMPLES;

    int8_t values[BLE_DB_BUSY_RSSI_SAMPLES];
    for (uint8_t i = 0; i < count; ++i) values[i] = samples[i];

    for (uint8_t i = 1; i < count; ++i) {
        int8_t key = values[i];
        int j = (int)i - 1;
        while (j >= 0 && values[j] > key) {
            values[j + 1] = values[j];
            --j;
        }
        values[j + 1] = key;
    }

    if (count & 1U) return values[count / 2U];
    return ((int16_t)values[(count / 2U) - 1U] +
            (int16_t)values[count / 2U]) / 2;
}

inline int bleDbFindRecentSlot(uint32_t localDeviceHash) {
    if (localDeviceHash == 0) return -1;
    for (int i = 0; i < BLE_DB_RECENT_CACHE_LEN; ++i) {
        if (bleDbRecent[i].localDeviceHash == localDeviceHash) return i;
    }
    return -1;
}

inline int bleDbAllocateRecentSlot(uint32_t localDeviceHash) {
    int emptySlot = -1;
    int oldestSlot = 0;

    for (int i = 0; i < BLE_DB_RECENT_CACHE_LEN; ++i) {
        if (bleDbRecent[i].localDeviceHash == 0 && emptySlot < 0) emptySlot = i;
        if (bleDbRecent[i].lastSeenMs < bleDbRecent[oldestSlot].lastSeenMs) oldestSlot = i;
    }

    int slot = (emptySlot >= 0) ? emptySlot : oldestSlot;
    bleDbRecent[slot] = {};
    bleDbRecent[slot].localDeviceHash = localDeviceHash;
    bleDbRecent[slot].smoothedRssi = -127;
    return slot;
}

inline void bleDbUpdateBusyMode(uint32_t nowMs) {
    uint16_t uniqueCount = 0;

    for (int i = 0; i < BLE_DB_RECENT_CACHE_LEN; ++i) {
        if (bleDbRecent[i].localDeviceHash == 0) continue;
        if ((uint32_t)(nowMs - bleDbRecent[i].lastSeenMs) <= BLE_DB_BUSY_WINDOW_MS) {
            ++uniqueCount;
        }
    }

    bleDbBusyUniquePerMinute = uniqueCount;

    // Hysteresis prevents flapping near the threshold.
    // Enter only ABOVE 20 unique devices/min; leave only BELOW 15/min.
    if (!bleDbBusyModeActive && uniqueCount > BLE_DB_BUSY_ENTER_UNIQUE_PER_MIN) {
        bleDbBusyModeActive = true;
    } else if (bleDbBusyModeActive && uniqueCount < BLE_DB_BUSY_EXIT_UNIQUE_PER_MIN) {
        bleDbBusyModeActive = false;
    }
}

inline int16_t bleDbTrackObservation(uint32_t localDeviceHash, int rssi, uint32_t nowMs) {
    int slot = bleDbFindRecentSlot(localDeviceHash);
    if (slot < 0) slot = bleDbAllocateRecentSlot(localDeviceHash);
    if (slot < 0) return (int16_t)rssi;

    BleDbRecentEntry& entry = bleDbRecent[slot];
    bleDbBusyLastObservationMs = nowMs;
    entry.lastSeenMs = nowMs;
    entry.rssiSamples[entry.rssiSampleNext] = (int8_t)constrain(rssi, -127, 20);
    entry.rssiSampleNext = (entry.rssiSampleNext + 1U) % BLE_DB_BUSY_RSSI_SAMPLES;
    if (entry.rssiSampleCount < BLE_DB_BUSY_RSSI_SAMPLES) ++entry.rssiSampleCount;
    entry.smoothedRssi = bleDbMedianRssi(entry.rssiSamples, entry.rssiSampleCount);

    bleDbUpdateBusyMode(nowMs);
    return entry.smoothedRssi;
}

inline bool bleDbBusyModeIsActive() {
    return bleDbBusyModeActive;
}

inline uint16_t bleDbBusyUniqueDevicesPerMinute() {
    return bleDbBusyUniquePerMinute;
}

inline bool bleDbRequestComesBefore(const BleDbQueuedLookup& a,
                                    const BleDbQueuedLookup& b,
                                    bool proximityPriority) {
    if (proximityPriority && a.priorityRssi != b.priorityRssi) {
        // Less-negative RSSI is stronger/closer and therefore comes first.
        return a.priorityRssi > b.priorityRssi;
    }
    return (int32_t)(a.queuedAtMs - b.queuedAtMs) < 0;
}

inline void bleDbFillPriorityBacklog() {
    if (bleDbRequestQueue == nullptr) return;

    BleDbQueuedLookup incoming = {};
    while (bleDbPriorityBacklogCount < BLE_DB_PRIORITY_BACKLOG_LEN &&
           xQueueReceive(bleDbRequestQueue, &incoming, 0) == pdTRUE) {
        bleDbPriorityBacklog[bleDbPriorityBacklogCount++] = incoming;
    }
}

inline bool bleDbTakeNextLookup(BleDbQueuedLookup& out) {
    if (bleDbRequestQueue == nullptr) return false;

    // During BUSY mode, gather the current bounded FIFO queue into a task-local
    // RAM backlog, then pick the strongest smoothed RSSI first. New arrivals
    // remain queued and are merged before every subsequent send. Far devices
    // are retained and processed later rather than discarded because of RSSI.
    if (bleDbBusyModeActive || bleDbPriorityBacklogCount > 0) {
        bleDbFillPriorityBacklog();
        if (bleDbPriorityBacklogCount == 0) return false;

        uint8_t best = 0;
        const bool proximityPriority = bleDbBusyModeActive;
        for (uint8_t i = 1; i < bleDbPriorityBacklogCount; ++i) {
            if (bleDbRequestComesBefore(bleDbPriorityBacklog[i],
                                        bleDbPriorityBacklog[best],
                                        proximityPriority)) {
                best = i;
            }
        }

        out = bleDbPriorityBacklog[best];
        for (uint8_t i = best + 1; i < bleDbPriorityBacklogCount; ++i) {
            bleDbPriorityBacklog[i - 1] = bleDbPriorityBacklog[i];
        }
        --bleDbPriorityBacklogCount;
        return true;
    }

    return xQueueReceive(bleDbRequestQueue, &out, 0) == pdTRUE;
}

inline bool bleDbShouldQueue(uint32_t localDeviceHash, uint32_t nowMs) {
    int slot = bleDbFindRecentSlot(localDeviceHash);
    if (slot < 0) slot = bleDbAllocateRecentSlot(localDeviceHash);
    if (slot < 0) return false;

    BleDbRecentEntry& entry = bleDbRecent[slot];
    if (entry.lastQueuedMs != 0 &&
        (uint32_t)(nowMs - entry.lastQueuedMs) < BLE_DB_REQUERY_MS) {
        return false;
    }

    entry.lastQueuedMs = nowMs;
    return true;
}

inline bool queueBleDatabaseObservation(uint32_t localDeviceHash,
                                        const String& rawMac,
                                        uint8_t addressType,
                                        uint16_t companyId,
                                        int rssi,
                                        const String& name,
                                        const String& manufacturerHex,
                                        const String& serviceUuids) {
    if (bleDbRequestQueue == nullptr) return false;

    uint32_t now = millis();

    // Track every observed advertisement before the 15-second DB re-query
    // suppression so BUSY mode reflects live unique-device density and RSSI
    // smoothing rather than only the subset sent to the coprocessor.
    int16_t smoothedRssi = bleDbTrackObservation(localDeviceHash, rssi, now);
    if (!bleDbShouldQueue(localDeviceHash, now)) return true;

    BleDbQueuedLookup request = {};
    request.localDeviceHash = localDeviceHash;
    request.queuedAtMs = now;
    request.priorityRssi = smoothedRssi;
    bleDbCopyText(request.payload.mac, sizeof(request.payload.mac), rawMac);
    request.payload.addressType = addressType;
    request.payload.rssi = (int16_t)rssi;
    request.payload.companyId = companyId;
    bleDbCopyText(request.payload.name, sizeof(request.payload.name), name);
    bleDbCopyText(request.payload.manufacturerHex, sizeof(request.payload.manufacturerHex), manufacturerHex);
    bleDbCopyText(request.payload.serviceUuids, sizeof(request.payload.serviceUuids), serviceUuids);

    // Zero wait: BLE callback must never block on the external database.
    if (xQueueSend(bleDbRequestQueue, &request, 0) != pdTRUE) {
        bleDbQueueDrops++;
        return false;
    }

    bleDbQueriesQueued++;
    return true;
}

inline void bleDbHandleReceivedFrame(const BleDbReceivedFrame& frame) {
    BleDbClientState before = bleDbClientState();
    bleDbLastValidRxMs = millis();
    BleDbClientState after = bleDbClientState();
    if (before != after) bleDbStatusChangedFlag = true;

    if (frame.type == BLE_DB_MSG_RESULT &&
        frame.payloadLength == sizeof(BleDbResultPayload)) {
        BleDbQueuedResult result = {};
        result.sequence = frame.sequence;
        memcpy(&result.payload, frame.payload, sizeof(result.payload));
        if (xQueueSend(bleDbResultQueue, &result, 0) == pdTRUE) {
            bleDbResultsReceived++;
            if (result.payload.matched) bleDbMatchedResults++;
        } else {
            bleDbQueueDrops++;
        }
    }
}

inline void bleDbClientTask(void*) {
    BleDbQueuedLookup pending = {};
    bool requestOutstanding = false;
    uint32_t pendingSequence = 0;
    uint32_t pendingSentMs = 0;
    uint32_t nextSequence = 1;
    uint32_t lastPingMs = 0;
    BleDbClientState previousState = BLE_DB_CLIENT_STARTING;

    for (;;) {
        while (bleDbSerial.available() > 0) {
            uint8_t value = (uint8_t)bleDbSerial.read();
            BleDbReceivedFrame frame = {};
            if (bleDbReceiver.feed(value, frame)) {
                bleDbHandleReceivedFrame(frame);
                if (frame.type == BLE_DB_MSG_RESULT && frame.sequence == pendingSequence) {
                    requestOutstanding = false;
                }
            }
        }

        uint32_t now = millis();

        BleDbClientState currentState = bleDbClientState();
        if (currentState != previousState) {
            previousState = currentState;
            bleDbStatusChangedFlag = true;
        }

        if ((uint32_t)(now - lastPingMs) >= BLE_DB_PING_INTERVAL_MS) {
            bleDbSendFrame(bleDbSerial, BLE_DB_MSG_PING, 0, nullptr, 0);
            lastPingMs = now;
        }

        if (requestOutstanding &&
            (uint32_t)(now - pendingSentMs) >= BLE_DB_REQUEST_TIMEOUT_MS) {
            requestOutstanding = false;
            bleDbTimeouts++;
        }

        // If RF activity stops completely, expire BUSY mode without touching
        // the observation cache from this UART task. Normal sparse traffic
        // recalculates the rolling count in bleDbTrackObservation().
        uint32_t lastObservation = bleDbBusyLastObservationMs;
        if (bleDbBusyModeActive && lastObservation != 0 &&
            (uint32_t)(now - lastObservation) > BLE_DB_BUSY_WINDOW_MS) {
            bleDbBusyUniquePerMinute = 0;
            bleDbBusyModeActive = false;
        }

        if (!requestOutstanding && bleDbRequestQueue != nullptr) {
            if (bleDbTakeNextLookup(pending)) {
                pendingSequence = nextSequence++;
                if (nextSequence == 0) nextSequence = 1;
                if (bleDbSendFrame(bleDbSerial,
                                   BLE_DB_MSG_LOOKUP,
                                   pendingSequence,
                                   &pending.payload,
                                   sizeof(pending.payload))) {
                    requestOutstanding = true;
                    pendingSentMs = now;
                    bleDbQueriesSent++;
                } else {
                    bleDbQueueDrops++;
                }
                // pending.raw MAC remains only in this task's RAM and is overwritten
                // by the next dequeued request. It is never persisted or printed.
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

inline bool initBleDatabaseClient() {
    bleDbRequestQueue = xQueueCreate(BLE_DB_REQUEST_QUEUE_LEN, sizeof(BleDbQueuedLookup));
    bleDbResultQueue = xQueueCreate(BLE_DB_RESULT_QUEUE_LEN, sizeof(BleDbQueuedResult));
    if (bleDbRequestQueue == nullptr || bleDbResultQueue == nullptr) {
        bleDbClientStartedMs = millis();
        bleDbStatusChangedFlag = true;
        return false;
    }

    bleDbSerial.begin(BLE_DB_UART_BAUD,
                      SERIAL_8N1,
                      BLE_DB_UART_RX_PIN,
                      BLE_DB_UART_TX_PIN);
    bleDbClientStartedMs = millis();
    bleDbLastValidRxMs = 0;

    BaseType_t rc = xTaskCreatePinnedToCore(
        bleDbClientTask,
        "ble-db-uart",
        BLE_DB_TASK_STACK_BYTES,
        nullptr,
        BLE_DB_TASK_PRIORITY,
        &bleDbTaskHandle,
        1
    );

    if (rc != pdPASS) {
        bleDbTaskHandle = nullptr;
        bleDbStatusChangedFlag = true;
        return false;
    }

    return true;
}

inline void processBleDatabaseResults() {
    if (bleDbResultQueue == nullptr) return;

    BleDbQueuedResult result = {};
    uint8_t processed = 0;
    while (processed < 4 && xQueueReceive(bleDbResultQueue, &result, 0) == pdTRUE) {
        processed++;
        bleDbLastClassification = result.payload.classification;
        bleDbLastConfidenceSuggestion = result.payload.confidenceSuggestion;
        strlcpy(bleDbLastVendor, result.payload.vendor, sizeof(bleDbLastVendor));
        strlcpy(bleDbLastProduct, result.payload.product, sizeof(bleDbLastProduct));
        strlcpy(bleDbLastDeviceType, result.payload.deviceType, sizeof(bleDbLastDeviceType));

        // Deliberately no raw MAC in this output.
        Serial.printf(
            "DB RESULT | seq=%lu | match=%u | class=%u | vendor=%s | product=%s | type=%s | suggestion=%u | reason=%s\n",
            (unsigned long)result.sequence,
            (unsigned)result.payload.matched,
            (unsigned)result.payload.classification,
            result.payload.vendor,
            result.payload.product,
            result.payload.deviceType,
            (unsigned)result.payload.confidenceSuggestion,
            result.payload.reason
        );
    }
}
