#pragma once

#include <string_view>

#include <io_base.h>

#include <rgb_color.h>
#include <job_ansi_attributes.h>

#include "job_tui_rect.h"

namespace job::tui {
class DrawContext {
public:
    using Ptr = std::shared_ptr<DrawContext>;
    explicit DrawContext(job::core::IODevice::Ptr device);
    enum class BorderStyle {
        SINGLE,
        DOUBLE,
        LINE,
        DOT,
        BLOCK,
        DarkShade,
        MediumShade,
        LightShade
    };
    bool ok() const;
    void write(std::string_view data);

    void setColor(const ansi::utils::RGBColor &fg, const ansi::utils::RGBColor &bg);
    void setForeground(const ansi::utils::RGBColor &fg);
    void setBackground(const ansi::utils::RGBColor &bg);

    void moveCursor(int x, int y);
    void resetColors();

    void apply(const ansi::Attributes &attr);

    void drawBox(int width, int height);
    void drawBox(Rect rect, BorderStyle style = BorderStyle::BLOCK);
    void drawBox(int x, int y, int width, int height, BorderStyle style = BorderStyle::BLOCK);

private:
    void applyColor(const ansi::utils::RGBColor &color, bool isForeground);

    core::IODevice::Ptr     m_device;
    ansi::utils::RGBColor   m_fg;
    ansi::utils::RGBColor   m_bg;
    bool                    m_writeOk = true;
};

}
