#include "job_tui_gradient.h"
namespace job::tui {

Gradient::Gradient(GradientOrientation orientation) :
    m_orientation(orientation)
{

}

void Gradient::addStop(int percent, ansi::utils::RGBColor color)
{
    m_stops.emplace_back(percent, color);
    std::sort(m_stops.begin(), m_stops.end(), [](const GradientStop &a, const GradientStop &b) {
        return a.position < b.position;
    });
}

ansi::utils::RGBColor Gradient::colorAt(int percent) const
{
    if (m_stops.empty())
        return ansi::utils::RGBColor{0, 0, 0};

    percent = std::clamp(percent, 0, 100);

    if (percent <= m_stops.front().position)
        return m_stops.front().color;

    if (percent >= m_stops.back().position)
        return m_stops.back().color;

    for (size_t i = 0; i < m_stops.size() - 1; ++i) {
        const auto &a = m_stops[i];
        const auto &b = m_stops[i + 1];
        if (percent >= a.position && percent <= b.position) {
            float t = float(percent - a.position) / float(b.position - a.position);
            return interpolate(a.color, b.color, t);
        }
    }

    return m_stops.back().color; // fallback
}

GradientOrientation Gradient::orientation() const
{
    return m_orientation;
}

void Gradient::setOrientation(GradientOrientation ori)
{
    m_orientation = ori;
}

ansi::utils::RGBColor Gradient::interpolate(const ansi::utils::RGBColor &a, const ansi::utils::RGBColor &b, float t)
{
    auto lerp = [](uint8_t x, uint8_t y, float f) -> uint8_t {
        return static_cast<uint8_t>(x + (y - x) * f);
    };
    return ansi::utils::RGBColor{
        lerp(a.red(),   b.red(),   t),
        lerp(a.green(), b.green(), t),
        lerp(a.blue(),  b.blue(),  t),
        lerp(a.alpha(), b.alpha(), t)
    };
}






}
