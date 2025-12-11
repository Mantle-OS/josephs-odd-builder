#pragma once

#include "job_tui_item.h"
#include "job_tui_core_application.h"
#include <string>

namespace job::tui::gui {

class Window : public JobTuiItem{
public:
    using Ptr = std::shared_ptr<Window>;
    enum class TitleAlignment {
        Left,
        Center,
        Right
    };
    static Window::Ptr create(JobTuiCoreApplication *parent = nullptr) {
        auto ptr = std::make_shared<Window>();
        if (parent)
            parent->setWindow(ptr);
        return ptr;
    }

    static Window::Ptr  create(Rect rect, const std::string &title = "",
                                          Window::TitleAlignment title_alignment = TitleAlignment::Center,
                                          JobTuiCoreApplication *parent = nullptr )
    {
        auto ptr = std::make_shared<Window>();
        if (parent)
            parent->setWindow(ptr);
        ptr->setTitle(title);
        ptr->setGeometry(rect);
        ptr->setAlign(title_alignment);
        return ptr;
    }

    static Window::Ptr  create(Rect rect, const std::string &title = "",
                                          JobTuiCoreApplication *parent = nullptr )
    {
        auto ptr = std::make_shared<Window>();
        if (parent)
            parent->setWindow(ptr);

        ptr->setTitle(title);
        ptr->setGeometry(rect);
        ptr->setAlign(TitleAlignment::Center);
        return ptr;
    }
    explicit Window(JobTuiItem *parent = nullptr);

    TitleAlignment align() const;
    void setAlign(TitleAlignment align);

    void setTitle(const std::string &title);
    std::string title() const;

    void paintTitle(DrawContext &ctx, int x, int y, int w);
    void paintSelf(DrawContext &ctx) override;

private:
    std::string     m_title;
    TitleAlignment  m_align = Window::TitleAlignment::Center;
};

}
