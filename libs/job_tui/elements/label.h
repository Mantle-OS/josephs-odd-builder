#pragma once

#include <string>
#include <memory>

#include "job_tui_item.h"

namespace job::tui::gui {

class Label : public JobTuiItem {
public:
    using Ptr = std::shared_ptr<Label>;

    enum class TextAlignment {
        AlignLeft,
        AlignRight,
        AlignBottom,
        AlignTop,
        AlignVCenter,
        AlignHCenter,
        AlignCenter = AlignVCenter | AlignHCenter
    };

    enum class ElideMode {
        ElideRight,
        ElideLeft,
        ElideMiddle,
        NoElide
    };

    enum class WrapMode {
        NoWrap,
        WordWrap,
        CharWrap
    };

    static Label::Ptr create(JobTuiItem *parent = nullptr) {
        auto ptr = std::make_shared<Label>();
        if (parent)
            parent->addChild(ptr);
        return ptr;
    }

    static Label::Ptr create(Rect rect, JobTuiItem *parent = nullptr)
    {
        auto ptr = std::make_shared<Label>();
        ptr->setGeometry(rect);
        if (parent)
            parent->addChild(ptr);
        return ptr;
    }

    static Label::Ptr create(Rect rect, const std::string &txt,
                                         JobTuiItem *parent = nullptr)
    {
        auto ptr = std::make_shared<Label>();
        ptr->setGeometry(rect);
        ptr->setText(txt);
        if (parent)
            parent->addChild(ptr);
        return ptr;
    }

    static Label::Ptr create(Rect rect,
                                         const std::string &txt,
                                         Label::WrapMode wrapMode = Label::WrapMode::NoWrap,
                                         Label::TextAlignment hAlign = Label::TextAlignment::AlignLeft,
                                         Label::TextAlignment vAlign = Label::TextAlignment::AlignVCenter,
                                         JobTuiItem *parent = nullptr)
    {
        auto ptr = std::make_shared<Label>();
        ptr->setGeometry(rect);
        ptr->setWrapMode(wrapMode);
        ptr->setText(txt);
        ptr->setHorizontalAlignment(hAlign);
        ptr->setVerticalAlignment(vAlign);
        if (parent)
            parent->addChild(ptr);
        return ptr;
    }

    explicit Label(JobTuiItem *parent = nullptr);

    void setText(const std::string &text);
    std::string text() const;

    void setHorizontalAlignment(TextAlignment a);
    TextAlignment horizontalAlignment() const;

    void setVerticalAlignment(TextAlignment a);
    TextAlignment verticalAlignment() const;

    void setElideMode(ElideMode mode);
    ElideMode elideMode() const;

    void setWrapMode(WrapMode mode);
    WrapMode wrapMode() const;

    void setForeground(const ansi::RGBColor &color);
    void setBackground(const ansi::RGBColor &color);
    void setUnderline(ansi::utils::enums::UnderlineStyle style);
    void setBold(bool on);
    void setItalic(bool italic);
    void setStrikethrough(bool on);

    void paintSelf(DrawContext &ctx) override;

private:
    std::string         m_text;
    TextAlignment       m_horizontalAlignment = TextAlignment::AlignLeft;
    TextAlignment       m_verticalAlignment   = TextAlignment::AlignTop;
    ElideMode           m_elideMode = ElideMode::NoElide;
    WrapMode            m_wrapMode = WrapMode::NoWrap;
};

} 
