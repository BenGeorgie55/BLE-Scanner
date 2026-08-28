#pragma once
struct FalsePositiveRule {
    const char* namePattern; const char* assumedDevice; const char* assumedType; const char* reason;
};
static const FalsePositiveRule FALSE_POSITIVE_RULES[] = {
    { "LE_WH-1000XM5", "Sony WH-1000XM5", "Headphones", "Known Sony headphones" },
    { "LE_WH-1000XM6", "Sony WH-1000XM6", "Headphones", "Known Sony headphones" },
    { "LE_WF-1000XM5", "Sony WF-1000XM5", "Earbuds", "Known Sony earbuds" },
    { "LE_WF-C510", "Sony WF-C510", "Earbuds", "Known Sony earbuds" },
    { "LE_WF-C710N", "Sony WF-C710N", "Earbuds", "Known Sony earbuds" },
    { "LE_ULT WEAR", "Sony ULT Wear", "Headphones", "Known Sony ULT headphones" },
    { "WH-", "WH-series audio device", "Headphones", "WH-series headphones" },
    { "Crusher", "Skullcandy Crusher", "Headphones", "Skullcandy Crusher headphones" },
    { "Skull Crusher", "Skullcandy Crusher", "Headphones", "Skullcandy headphones" },
    { "Govee", "Govee device", "Smart light", "Govee lighting device" },
    { "Square Reader", "Square Reader", "POS system", "Square commercial POS reader" },
    { "Square", "Square POS device", "POS system", "Square commercial POS device" },
    { "Jabra Evolve", "Jabra Evolve", "Headset", "Jabra Evolve headset/headphones" },
    { "BYD", "BYD vehicle", "Car", "BYD vehicle" },
    { nullptr, nullptr, nullptr, nullptr }
};
