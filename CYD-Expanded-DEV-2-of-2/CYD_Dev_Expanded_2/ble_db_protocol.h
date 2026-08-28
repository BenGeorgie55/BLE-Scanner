#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <string.h>

// Shared CYD <-> BLE database ESP32 wire protocol.
// Raw observed BLE MAC addresses MAY transit this protocol and exist in RAM,
// but MUST NEVER be written to SD, LittleFS, NVS/Preferences, files, or logs.

#define BLE_DB_PROTOCOL_VERSION 1
#define BLE_DB_MAGIC_0 0xDB
#define BLE_DB_MAGIC_1 0xB1
#define BLE_DB_MAX_PAYLOAD 384

#define BLE_DB_NAME_LEN       56
#define BLE_DB_MFG_HEX_LEN   128
#define BLE_DB_UUIDS_LEN     128
#define BLE_DB_VENDOR_LEN     48
#define BLE_DB_PRODUCT_LEN    72
#define BLE_DB_TYPE_LEN       44
#define BLE_DB_REASON_LEN     96

enum BleDbMessageType : uint8_t {
    BLE_DB_MSG_HELLO  = 1,
    BLE_DB_MSG_PING   = 2,
    BLE_DB_MSG_PONG   = 3,
    BLE_DB_MSG_LOOKUP = 4,
    BLE_DB_MSG_RESULT = 5
};

enum BleDbClassification : uint8_t {
    BLE_DB_CLASS_UNKNOWN = 0,
    BLE_DB_CLASS_REFERENCE,
    BLE_DB_CLASS_KNOWN_DEVICE,
    BLE_DB_CLASS_FALSE_POSITIVE,
    BLE_DB_CLASS_SMART_GLASSES,
    BLE_DB_CLASS_CAMERA_AUDIO
};

enum BleDbEvidence : uint8_t {
    BLE_DB_EVIDENCE_NONE = 0,
    BLE_DB_EVIDENCE_REFERENCE,
    BLE_DB_EVIDENCE_KNOWN,
    BLE_DB_EVIDENCE_STRONG
};

#pragma pack(push, 1)
struct BleDbFrameHeader {
    uint8_t  magic0;
    uint8_t  magic1;
    uint8_t  version;
    uint8_t  type;
    uint32_t sequence;
    uint16_t payloadLength;
};

struct BleDbHelloPayload {
    uint16_t protocolVersion;
    uint16_t companyReferenceCount;
    uint16_t glassesRuleCount;
    uint16_t knownDeviceRuleCount;
    uint16_t micCamRuleCount;
    uint32_t buildId;
    char     nodeName[24];
};

struct BleDbLookupPayload {
    // Transient only. Never persist this field on either ESP32.
    char     mac[18];
    uint8_t  addressType;
    int16_t  rssi;
    uint16_t companyId;       // 0xFFFF = unavailable
    char     name[BLE_DB_NAME_LEN];
    char     manufacturerHex[BLE_DB_MFG_HEX_LEN];
    char     serviceUuids[BLE_DB_UUIDS_LEN];
};

struct BleDbResultPayload {
    uint8_t  matched;
    uint8_t  classification;
    uint8_t  evidence;
    uint8_t  tier;
    uint8_t  confidenceSuggestion; // Informational in v1; CYD local rules remain authoritative.
    uint8_t  hasCamera;
    uint8_t  cameraKnown;
    uint8_t  reserved;
    uint16_t companyId;
    char     vendor[BLE_DB_VENDOR_LEN];
    char     product[BLE_DB_PRODUCT_LEN];
    char     deviceType[BLE_DB_TYPE_LEN];
    char     reason[BLE_DB_REASON_LEN];
};
#pragma pack(pop)

static_assert(sizeof(BleDbFrameHeader) == 10, "Unexpected BLE DB frame header size");
static_assert(sizeof(BleDbLookupPayload) <= BLE_DB_MAX_PAYLOAD, "BLE DB lookup payload too large");
static_assert(sizeof(BleDbResultPayload) <= BLE_DB_MAX_PAYLOAD, "BLE DB result payload too large");

inline uint16_t bleDbCrc16Update(uint16_t crc, const uint8_t* data, size_t length) {
    while (length--) {
        crc ^= (uint16_t)(*data++) << 8;
        for (uint8_t i = 0; i < 8; ++i) {
            crc = (crc & 0x8000U) ? (uint16_t)((crc << 1) ^ 0x1021U)
                                  : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

inline uint16_t bleDbFrameCrc(const BleDbFrameHeader& header,
                              const uint8_t* payload,
                              uint16_t payloadLength) {
    uint16_t crc = 0xFFFFU;
    crc = bleDbCrc16Update(crc, reinterpret_cast<const uint8_t*>(&header), sizeof(header));
    if (payload != nullptr && payloadLength > 0) {
        crc = bleDbCrc16Update(crc, payload, payloadLength);
    }
    return crc;
}

inline bool bleDbSendFrame(HardwareSerial& serial,
                           uint8_t type,
                           uint32_t sequence,
                           const void* payload,
                           uint16_t payloadLength) {
    if (payloadLength > BLE_DB_MAX_PAYLOAD) return false;

    BleDbFrameHeader header = {
        BLE_DB_MAGIC_0,
        BLE_DB_MAGIC_1,
        BLE_DB_PROTOCOL_VERSION,
        type,
        sequence,
        payloadLength
    };

    const uint8_t* payloadBytes = reinterpret_cast<const uint8_t*>(payload);
    uint16_t crc = bleDbFrameCrc(header, payloadBytes, payloadLength);

    if (serial.write(reinterpret_cast<const uint8_t*>(&header), sizeof(header)) != sizeof(header)) return false;
    if (payloadLength > 0 &&
        serial.write(payloadBytes, payloadLength) != payloadLength) return false;

    uint8_t crcBytes[2] = {
        (uint8_t)(crc & 0xFFU),
        (uint8_t)((crc >> 8) & 0xFFU)
    };
    return serial.write(crcBytes, sizeof(crcBytes)) == sizeof(crcBytes);
}

struct BleDbReceivedFrame {
    uint8_t  type;
    uint32_t sequence;
    uint16_t payloadLength;
    uint8_t  payload[BLE_DB_MAX_PAYLOAD];
};

class BleDbFrameReceiver {
public:
    BleDbFrameReceiver() { reset(); }

    void reset() {
        index_ = 0;
        expectedLength_ = 0;
    }

    bool feed(uint8_t byteValue, BleDbReceivedFrame& out) {
        if (index_ == 0) {
            if (byteValue != BLE_DB_MAGIC_0) return false;
            buffer_[index_++] = byteValue;
            return false;
        }

        if (index_ == 1) {
            if (byteValue != BLE_DB_MAGIC_1) {
                index_ = (byteValue == BLE_DB_MAGIC_0) ? 1 : 0;
                if (index_ == 1) buffer_[0] = BLE_DB_MAGIC_0;
                return false;
            }
            buffer_[index_++] = byteValue;
            return false;
        }

        if (index_ >= sizeof(buffer_)) {
            reset();
            return false;
        }

        buffer_[index_++] = byteValue;

        if (index_ == sizeof(BleDbFrameHeader)) {
            BleDbFrameHeader header;
            memcpy(&header, buffer_, sizeof(header));
            if (header.magic0 != BLE_DB_MAGIC_0 ||
                header.magic1 != BLE_DB_MAGIC_1 ||
                header.version != BLE_DB_PROTOCOL_VERSION ||
                header.payloadLength > BLE_DB_MAX_PAYLOAD) {
                reset();
                return false;
            }
            expectedLength_ = sizeof(BleDbFrameHeader) + header.payloadLength + 2U;
        }

        if (expectedLength_ == 0 || index_ < expectedLength_) return false;
        if (index_ != expectedLength_) {
            reset();
            return false;
        }

        BleDbFrameHeader header;
        memcpy(&header, buffer_, sizeof(header));
        const uint8_t* payload = buffer_ + sizeof(BleDbFrameHeader);
        const uint8_t* crcPtr = payload + header.payloadLength;
        uint16_t receivedCrc = (uint16_t)crcPtr[0] | ((uint16_t)crcPtr[1] << 8);
        uint16_t expectedCrc = bleDbFrameCrc(header, payload, header.payloadLength);

        if (receivedCrc != expectedCrc) {
            reset();
            return false;
        }

        out.type = header.type;
        out.sequence = header.sequence;
        out.payloadLength = header.payloadLength;
        if (header.payloadLength > 0) {
            memcpy(out.payload, payload, header.payloadLength);
        }
        reset();
        return true;
    }

private:
    uint8_t buffer_[sizeof(BleDbFrameHeader) + BLE_DB_MAX_PAYLOAD + 2U];
    size_t index_;
    size_t expectedLength_;
};
