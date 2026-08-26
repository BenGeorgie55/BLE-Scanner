#pragma once
#include <stdint.h>
#ifndef GLASSES_CONFIDENCE_RULE_TYPE_DEFINED
#define GLASSES_CONFIDENCE_RULE_TYPE_DEFINED
struct GlassesConfidenceRule {
    const char* company; const char* product; uint8_t confidence; int32_t companyId;
    const char* macExact; const char* manufacturerHex; uint16_t serviceUuid16;
    const char* namePattern; bool exactName; uint32_t oui24; bool hasCamera;
    bool cameraKnown; const char* reason;
};
#endif
static const GlassesConfidenceRule HIGH_CONFIDENCE_RULES[] = {
    { "Google", "Google Glass", 90, -1, "40:a3:b2:6c:24:46", nullptr, 0, nullptr, false, 0, true, true, "Exact confirmed Google Glass MAC" },
    { "Google", "Google Glass", 90, 0x00E0, nullptr, "E0000090CA64E064", 0, nullptr, false, 0, true, true, "Confirmed Google Glass manufacturer fingerprint" },
    { "Meta Platforms Technologies", "Meta Ray-Ban", 90, 0x058E, nullptr, "4D45544152425F474C415353", 0, nullptr, false, 0, true, true, "Confirmed META_RB_GLASS manufacturer fingerprint" },
    { "Luxottica Group", "Ray-Ban Meta", 90, 0x0D53, nullptr, nullptr, 0, "ray-ban", false, 0, true, true, "Luxottica Company ID + Ray-Ban name" },
    { "Luxottica Group", "Ray-Ban Meta", 90, 0x0D53, nullptr, nullptr, 0, "ray ban", false, 0, true, true, "Luxottica Company ID + Ray Ban name" },
    { "Luxottica Group", "Oakley Meta", 90, 0x0D53, nullptr, nullptr, 0, "oakley", false, 0, true, true, "Luxottica Company ID + Oakley name" },
    { "Meta Platforms", "Meta smart-glasses candidate", 90, 0x01AB, nullptr, nullptr, 0xFD5F, nullptr, false, 0, true, true, "Meta Company ID + Meta service UUID" },
    { "Sony Corporation", "Sony SmartEyeglass", 90, 0x012D, nullptr, nullptr, 0, "SmartEyeglass", false, 0, true, true, "Sony Company ID + SmartEyeglass name" },
    { "Seiko Epson", "Epson Moverio", 90, 0x0040, nullptr, nullptr, 0, "Moverio", false, 0, true, true, "Epson Company ID + Moverio name" },
    { "Snapchat Inc", "Snap Spectacles", 90, 0x03C2, nullptr, nullptr, 0, "Spectacles", false, 0, true, true, "Snap Company ID + Spectacles name" },
    { "Vuzix Corporation", "Vuzix Smart Glasses", 90, 0x060C, nullptr, nullptr, 0, "Vuzix", false, 0, true, true, "Vuzix Company ID + Vuzix name" },
    { nullptr, nullptr, 0, -1, nullptr, nullptr, 0, nullptr, false, 0, false, false, nullptr }
};
