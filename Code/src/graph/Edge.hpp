#pragma once

#include <cstdint>

struct Edge {
    uint64_t id = 0;
    uint64_t from = 0;
    uint64_t to   = 0;
    uint32_t color = 0x005A6E8C; // packed 0x00RRGGBB

    static uint32_t packRGB(uint8_t r, uint8_t g, uint8_t b) {
        return (static_cast<uint32_t>(r) << 16) |
               (static_cast<uint32_t>(g) << 8)  |
                static_cast<uint32_t>(b);
    }
    uint8_t r() const { return (color >> 16) & 0xFF; }
    uint8_t g() const { return (color >>  8) & 0xFF; }
    uint8_t b() const { return  color        & 0xFF; }
};
