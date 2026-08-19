#pragma once

#include <string>
#include <cstdint>

// Axis-aligned region on the XY plane (center + size)
struct Square {
    uint64_t id = 0;
    double x = 0.0;   // center X
    double y = 0.0;   // center Y
    double w = 2.0;   // width  (world units)
    double h = 2.0;   // height (world units)
    std::string title;
    std::string text;   // core text
    int z = 0;            // stacking -10 back .. 10 front
    uint32_t color = 0x0044AAFF; // packed 0x00RRGGBB border

    bool selected = false;

    static uint32_t packRGB(uint8_t r, uint8_t g, uint8_t b) {
        return (static_cast<uint32_t>(r) << 16) |
               (static_cast<uint32_t>(g) << 8)  |
                static_cast<uint32_t>(b);
    }
    uint8_t r() const { return (color >> 16) & 0xFF; }
    uint8_t g() const { return (color >>  8) & 0xFF; }
    uint8_t b() const { return  color        & 0xFF; }
    void setRGB(uint8_t r, uint8_t g, uint8_t b) { color = packRGB(r, g, b); }

    // Dimmed fill (~25% brightness)
    void dimmedRGB(uint8_t& out_r, uint8_t& out_g, uint8_t& out_b) const {
        out_r = static_cast<uint8_t>(r() * 0.25);
        out_g = static_cast<uint8_t>(g() * 0.25);
        out_b = static_cast<uint8_t>(b() * 0.25);
    }
};
