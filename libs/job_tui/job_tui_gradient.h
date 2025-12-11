#pragma once

#include <memory>
#include <vector>
#include <algorithm>
#include <rgb_color.h>

namespace job::tui {

enum class GradientOrientation {
    HORIZONTAL,
    VERTICAL
};

struct GradientStop {
    int                     position; // 0–100 (percent)
    ansi::utils::RGBColor   color;

    GradientStop(int pos, ansi::utils::RGBColor col) :
        position(std::clamp(pos, 0, 100)),
        color(col)
    {
    }
};

class Gradient {
public:
    using Ptr = std::shared_ptr<Gradient>;
    Gradient(GradientOrientation orientation = GradientOrientation::HORIZONTAL);

    void addStop(int percent, ansi::utils::RGBColor color);

    ansi::utils::RGBColor colorAt(int percent) const;

    GradientOrientation orientation() const;
    void setOrientation(GradientOrientation ori);

private:
    std::vector<GradientStop>   m_stops;
    GradientOrientation         m_orientation;

    static ansi::utils::RGBColor interpolate(const ansi::utils::RGBColor &a, const ansi::utils::RGBColor &b, float t);
};

}
