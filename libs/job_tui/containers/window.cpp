#include "window.h"
#include <job_ansi_suffix.h>
#include <algorithm>

namespace job::tui::gui {

Window::Window(JobTuiItem *parent):
    JobTuiItem{parent}
{
    setItemHasContents(true);
}

void Window::setTitle(const std::string &title)
{
    m_title = title;
}

std::string Window::title() const
{
    return m_title;
}

Window::TitleAlignment Window::align() const
{
    return m_align;
}

void Window::setAlign(TitleAlignment align)
{
    m_align = align;
}

void Window::paintTitle(DrawContext &ctx, int x, int y, int w)
{
    if (m_title.empty())
        return;

    std::string titleText = " " + m_title + " ";
    // truncate and indicate with ellipsis
    if (static_cast<int>(titleText.size()) > w - 4)
        titleText = " " + m_title.substr(0, w - 6) + "… ";

    int tx = x + 2; // default left alignment
    switch (m_align) {
    case Window::TitleAlignment::Center:
        tx = x + (w - titleText.size()) / 2;
        break;
    case Window::TitleAlignment::Right:
        tx = x + w - static_cast<int>(titleText.size()) - 2;
        break;
    case Window::TitleAlignment::Left:
    default:
        break;
    }

    // Draw title over horizontal line
    ctx.moveCursor(tx, y);
    ctx.write(titleText);
}

void Window::paintSelf(DrawContext &ctx)
{
    if (!isVisible())
        return;

    int x;
    int y;
    int w;
    int h;
    getGeometry(x, y, w, h);

    using namespace ansi::utils::suffix;

    ctx.write(RESET_SGR);

    // ─ Horizontal lines
    for (int i = 1; i < w - 1; ++i) {
        ctx.moveCursor(x + i, y);
        ctx.write("─");
        ctx.moveCursor(x + i, y + h - 1);
        ctx.write("─");
    }

    // │ Vertical lines
    for (int i = 1; i < h - 1; ++i) {
        ctx.moveCursor(x, y + i);
        ctx.write("│");
        ctx.moveCursor(x + w - 1, y + i);
        ctx.write("│");
    }

    // Corners
    ctx.moveCursor(x, y);
    ctx.write("┌");

    ctx.moveCursor(x + w - 1, y);
    ctx.write("┐");

    ctx.moveCursor(x, y + h - 1);
    ctx.write("└");

    ctx.moveCursor(x + w - 1, y + h - 1);
    ctx.write("┘");

    // Title
    paintTitle(ctx, x, y, w);

    // Paint self
    // paintSelf(ctx);

    // Paint children with margin offset (content region)
    int contentX = x + 1;
    int contentY = y + 1;
    for (const auto &child : childrenAs<JobTuiItem>())
        if (child->isVisible())
            child->paint(ctx, contentX, contentY);
}

}
