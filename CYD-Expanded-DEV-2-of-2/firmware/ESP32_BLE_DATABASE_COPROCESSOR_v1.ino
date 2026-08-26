/*
 * ESP32 BLE DATABASE COPROCESSOR v1
 * ---------------------------------
 * Target: classic ESP32 Dev Module, 4 MB flash, no PSRAM required.
 *
 * Role:
 *   - NO BLE scanning on this ESP in v1.
 *   - Receive BLE observation lookups from the CYD over UART.
 *   - Search flash-resident smart-glasses / known-device / reference tables.
 *   - Return evidence/annotation to the CYD.
 *
 * Privacy hard rule:
 *   Observed raw BLE MAC addresses may exist transiently in RAM and transit
 *   UART for matching/OUI extraction, but MUST NEVER be persisted or printed.
 *   This sketch does not mount LittleFS/SPIFFS/SD and does not use Preferences.
 */

#include <Arduino.h>
#include <HardwareSerial.h>
#include "ble_db_protocol.h"
#include "database_engine.h"

// ---------------- USER-EDITABLE UART SETTINGS ----------------
#define DB_UART_BAUD          460800UL
#define DB_UART_RX_PIN             16   // Connect from CYD GPIO22 TX
#define DB_UART_TX_PIN             17   // Connect to   CYD GPIO27 RX
#define DB_UART_PORT                2

#define DB_WORK_QUEUE_LEN          24
#define DB_RESULT_QUEUE_LEN        16
#define DB_WORKER_STACK_BYTES    6144
#define DB_WORKER_PRIORITY          1
#define DB_HELLO_INTERVAL_MS     2000UL
// -------------------------------------------------------------

HardwareSerial DbUart(DB_UART_PORT);
BleDbFrameReceiver DbReceiver;

struct DbWorkItem {
    uint32_t sequence;
    BleDbLookupPayload lookup;
};

struct DbResultItem {
    uint32_t sequence;
    BleDbResultPayload result;
};

QueueHandle_t DbWorkQueue = nullptr;
QueueHandle_t DbResultQueue = nullptr;
TaskHandle_t DbWorkerTaskHandle = nullptr;

volatile uint32_t DbRxLookups = 0;
volatile uint32_t DbTxResults = 0;
volatile uint32_t DbQueueDrops = 0;
volatile uint32_t DbInvalidFrames = 0;
volatile uint32_t DbMatches = 0;
uint32_t LastHelloMs = 0;

static const uint32_t DB_BUILD_ID = 0x00010001UL;

void sendHello() {
    BleDbHelloPayload hello = {};
    hello.protocolVersion = BLE_DB_PROTOCOL_VERSION;
    hello.companyReferenceCount = GENERATED_COMPANY_ID_COUNT;
    hello.glassesRuleCount = dbGlassesRuleCount();
    hello.knownDeviceRuleCount = dbKnownRuleCount();
    hello.micCamRuleCount = dbMicCamRuleCount();
    hello.buildId = DB_BUILD_ID;
    strlcpy(hello.nodeName, "BLE-DB-ESP32-v1", sizeof(hello.nodeName));
    bleDbSendFrame(DbUart, BLE_DB_MSG_HELLO, 0, &hello, sizeof(hello));
}

void dbWorkerTask(void*) {
    DbWorkItem work = {};
    for (;;) {
        if (xQueueReceive(DbWorkQueue, &work, portMAX_DELAY) == pdTRUE) {
            DbResultItem output = {};
            output.sequence = work.sequence;
            dbEvaluateLookup(work.lookup, output.result);
            if (output.result.matched) DbMatches++;

            if (xQueueSend(DbResultQueue, &output, pdMS_TO_TICKS(10)) != pdTRUE) {
                DbQueueDrops++;
            }

            // Erase the RAM copy containing the transient raw BLE MAC as soon as
            // this lookup is complete. Nothing from work.lookup is persisted.
            memset(&work, 0, sizeof(work));
        }
    }
}

void handleFrame(const BleDbReceivedFrame& frame) {
    if (frame.type == BLE_DB_MSG_PING) {
        bleDbSendFrame(DbUart, BLE_DB_MSG_PONG, frame.sequence, nullptr, 0);
        return;
    }

    if (frame.type != BLE_DB_MSG_LOOKUP ||
        frame.payloadLength != sizeof(BleDbLookupPayload)) {
        return;
    }

    DbWorkItem work = {};
    work.sequence = frame.sequence;
    memcpy(&work.lookup, frame.payload, sizeof(work.lookup));

    // Zero-wait enqueue. If overloaded, drop the external lookup rather than
    // block UART servicing or create a growing RAM backlog.
    if (xQueueSend(DbWorkQueue, &work, 0) == pdTRUE) {
        DbRxLookups++;
    } else {
        DbQueueDrops++;
    }

    // Clear the local stack copy that contained the transient raw MAC.
    memset(&work, 0, sizeof(work));
}

void setup() {
    Serial.begin(115200);
    delay(250);

    Serial.println();
    Serial.println("ESP32 BLE DATABASE COPROCESSOR v1");
    Serial.println("BLE-only database service; Wi-Fi disabled.");
    Serial.println("Raw observed BLE MAC persistence: DISABLED.");
    Serial.printf("Glasses rules: %u\n", (unsigned)dbGlassesRuleCount());
    Serial.printf("Known-device rules: %u\n", (unsigned)dbKnownRuleCount());
    Serial.printf("CAM/AUDIO rules: %u\n", (unsigned)dbMicCamRuleCount());
    Serial.printf("Company references: %u\n", (unsigned)GENERATED_COMPANY_ID_COUNT);

    DbWorkQueue = xQueueCreate(DB_WORK_QUEUE_LEN, sizeof(DbWorkItem));
    DbResultQueue = xQueueCreate(DB_RESULT_QUEUE_LEN, sizeof(DbResultItem));
    if (DbWorkQueue == nullptr || DbResultQueue == nullptr) {
        Serial.println("FATAL: RAM queue allocation failed.");
        while (true) delay(1000);
    }

    DbUart.begin(DB_UART_BAUD, SERIAL_8N1, DB_UART_RX_PIN, DB_UART_TX_PIN);

    BaseType_t taskRc = xTaskCreatePinnedToCore(
        dbWorkerTask,
        "ble-db-worker",
        DB_WORKER_STACK_BYTES,
        nullptr,
        DB_WORKER_PRIORITY,
        &DbWorkerTaskHandle,
        1
    );
    if (taskRc != pdPASS) {
        Serial.println("FATAL: database worker task allocation failed.");
        while (true) delay(1000);
    }

    sendHello();
    LastHelloMs = millis();
}

void loop() {
    // UART RX parser. No database search is performed here; valid lookup frames
    // are pushed into the bounded RAM worker queue.
    while (DbUart.available() > 0) {
        uint8_t value = (uint8_t)DbUart.read();
        BleDbReceivedFrame frame = {};
        if (DbReceiver.feed(value, frame)) {
            handleFrame(frame);
        }
    }

    // Drain completed lookup results and transmit them to the CYD.
    DbResultItem output = {};
    uint8_t sentThisLoop = 0;
    while (sentThisLoop < 4 && xQueueReceive(DbResultQueue, &output, 0) == pdTRUE) {
        if (bleDbSendFrame(DbUart,
                           BLE_DB_MSG_RESULT,
                           output.sequence,
                           &output.result,
                           sizeof(output.result))) {
            DbTxResults++;
        }
        memset(&output, 0, sizeof(output));
        sentThisLoop++;
    }

    uint32_t now = millis();
    if ((uint32_t)(now - LastHelloMs) >= DB_HELLO_INTERVAL_MS) {
        sendHello();
        LastHelloMs = now;
    }

    // Human-readable local health only. Never print observed raw BLE MACs.
    static uint32_t lastStats = 0;
    if ((uint32_t)(now - lastStats) >= 10000UL) {
        Serial.printf("DB STAT | rx=%lu tx=%lu matches=%lu drops=%lu free_heap=%u\n",
                      (unsigned long)DbRxLookups,
                      (unsigned long)DbTxResults,
                      (unsigned long)DbMatches,
                      (unsigned long)DbQueueDrops,
                      ESP.getFreeHeap());
        lastStats = now;
    }

    delay(1);
}
