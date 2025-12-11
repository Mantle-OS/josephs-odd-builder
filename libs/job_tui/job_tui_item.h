#pragma once

#include <memory>
#include <job_ansi_attributes.h>

#include "job_tui_object.h"
#include "job_tui_draw_context.h"
#include "job_tui_rect.h"
#include "job_tui_anchors.h"
#include "job_tui_layout_engine.h"

namespace job::tui {
class JobTuiItem : public JobTuiObject {
public:
    using Ptr = std::shared_ptr<JobTuiItem>;
    explicit JobTuiItem(JobTuiObject *parent = nullptr);

    JobTuiItem(Rect rec,
               ansi::Attributes::Ptr attr = nullptr,
               JobTuiObject *parent = nullptr);

    virtual ~JobTuiItem() = default;

    static JobTuiItem::Ptr create(JobTuiItem *parent = nullptr)
    {
        auto ptr = std::make_shared<JobTuiItem>(parent);
        if (parent)
            parent->addChild(ptr);

        return ptr;
    }

    static JobTuiItem::Ptr create(JobTuiAnchors::Ptr anchors, JobTuiItem *parent = nullptr)
    {
        auto ptr = std::make_shared<JobTuiItem>(parent);
        if(anchors)
            ptr->setAnchors(*anchors.get());

        if (parent)
            parent->addChild(ptr);

        return ptr;
    }

    static JobTuiItem::Ptr create(Rect rect, JobTuiItem *parent = nullptr)
    {
        auto ptr = std::make_shared<JobTuiItem>(parent);
        ptr->setGeometry(rect);
        if (parent)
            parent->addChild(ptr);

        return ptr;
    }

    static JobTuiItem::Ptr create(Rect rect, ansi::Attributes::Ptr attr, JobTuiItem *parent = nullptr)
    {
        auto ptr = std::make_shared<JobTuiItem>(parent);
        ptr->setGeometry(rect);
        ptr->setAttributes(attr);
        ptr->setItemHasContents(true);
        if (parent)
            parent->addChild(ptr);

        return ptr;
    }

    virtual void paint(DrawContext &ctx, int parentX, int parentY);
    virtual void paint(DrawContext &ctx);
    virtual void paintSelf(DrawContext &ctx);
    virtual void layout(int x, int y, int width, int height);

    void setGeometry(Rect rec);
    void setGeometry(int x, int y, int w, int h);
    void getGeometry(int &x, int &y, int &w, int &h) const;
    Rect boundingRect() const;
    void setAttributes(const ansi::Attributes::Ptr &attr);
    ansi::Attributes::Ptr attributes() const;

    virtual void setVisible(bool visible);;
    bool isVisible() const;

    void setFocus(bool focus);
    bool focus() const;
    virtual bool focusEvent(const Event &event);
    std::function<void(bool event)> onFocusChanged;

    void handleEvent(const Event &event) override
    {
        // Handle mouse events if enabled
        if ((event.type() == Event::EventType::MousePress || event.type() == Event::EventType::MouseRelease) && acceptsMouseEvents())
            if (mouseEvent(event))
                return;
        // Call generic event handler and propagate to children
        JobTuiObject::handleEvent(event);
    }
    template<typename T>
    static std::shared_ptr<T> createChild(JobTuiItem *parent, bool fill = false)
    {
        auto ptr = std::make_shared<T>(parent);
        if (parent) {
            parent->addChild(ptr);
            if (fill)
                ptr->setGeometry(parent->x(), parent->y(), parent->width(), parent->height());
        }
        return ptr;
    }
    void setZ(int z);
    int z() const;

    void setY(int y);
    int y() const;

    void setX(int x);
    int x() const;

    void setWidth(int width);
    int width() const;

    void setHeight(int height);
    int height() const;

    bool isFocusable() const;
    void setFocusable(bool focusable);

    JobTuiAnchors &anchors();
    const JobTuiAnchors &anchors() const;
    void setAnchors(JobTuiAnchors anchors);
    void resolveAnchors();
\
    LayoutEngine *layout() const;
    void setLayout(LayoutEngine::Ptr engine);
    void applyLayout();

    virtual bool mouseEvent([[maybe_unused]] const Event &event);
    void setAcceptsMouseEvents(bool enabled);
    bool acceptsMouseEvents() const;

    bool itemHasContents() const;
    void setItemHasContents(bool itemHasContents);

protected:
    int                     m_x                     = 0;
    int                     m_y                     = 0;
    int                     m_z                     = 0;
    int                     m_width                 = 0;
    int                     m_height                = 0;
    bool                    m_focus                 = false;
    bool                    m_focusable             = true;
    bool                    m_visible               = true;
    ansi::Attributes::Ptr   m_attributes;
    bool                    m_acceptsMouseEvents    = false;
    bool                    m_itemHasContents       = false;

private:
    JobTuiAnchors           m_anchors;
    LayoutEngine::Ptr       m_layoutEngine;

};

}
