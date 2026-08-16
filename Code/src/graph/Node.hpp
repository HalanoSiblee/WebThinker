#pragma once

#include <string>
#include <cstdint>

// Color is packed as 0x00RRGGBB (24-bit RGB, high byte unused/reserved)
struct Node {
    uint64_t id = 0;
    double x = 0.0;
    double y = 0.0;
    std::string title;
    std::string text;               // core content
    uint32_t color = 0x00E0E0E0;    // packed 0x00RRGGBB

    // Runtime only
    bool selected = false;

    // Helpers for packed color
    static uint32_t packRGB(uint8_t r, uint8_t g, uint8_t b) {
        return (static_cast<uint32_t>(r) << 16) |
               (static_cast<uint32_t>(g) << 8)  |
                static_cast<uint32_t>(b);
    }
    uint8_t r() const { return (color >> 16) & 0xFF; }
    uint8_t g() const { return (color >>  8) & 0xFF; }
    uint8_t b() const { return  color        & 0xFF; }
    void setRGB(uint8_t r, uint8_t g, uint8_t b) { color = packRGB(r, g, b); }
};
