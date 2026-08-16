#pragma once

#include <string>
#include <cstdint>

// SVG vector object placed on the XY plane
struct VectorObject {
    uint64_t id = 0;
    double x = 0.0;       // center X
    double y = 0.0;       // center Y
    float  scale = 1.0f;  // size multiplier
    std::string title;
    std::string svg;      // raw SVG markup
    bool selected = false;

    // Approximate world-unit half-size for hit testing (before scale)
    static constexpr double kBaseHalf = 1.0;
};
