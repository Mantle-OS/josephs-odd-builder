#include "job_tui_item.h"

namespace job::tui {

JobTuiItem::JobTuiItem(JobTuiObject *parent) :
    JobTuiObject{parent},
    m_attributes{std::make_shared<ansi::Attributes>()}
{

}

JobTuiItem::JobTuiItem(Rect rec, ansi::Attributes::Ptr attr, JobTuiObject *parent) :
    JobTuiObject{parent},
    m_x(rec.x),
    m_y(rec.y),
    m_width(rec.width),
    m_height(rec.height),
    m_attributes(std::move(attr))
{
}

void JobTuiItem::layout(int x, int y, int width, int height)
{
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;
}

void JobTuiItem::setGeometry(Rect rec)
{
    setGeometry(rec.x, rec.y, rec.width, rec.height);
}

void JobTuiItem::setGeometry(int x, int y, int w, int h)
{
    m_x = x;
    m_y = y;
    m_width = w;
    m_height = h;
}

void JobTuiItem::getGeometry(int &x, int &y, int &w, int &h) const
{
    x = m_x;
    y = m_y;
    w = m_width;
    h = m_height;
}

Rect JobTuiItem::boundingRect() const
{
    return Rect{m_x, m_y, m_width, m_height};
}

void JobTuiItem::setAttributes(const ansi::Attributes::Ptr &attr)
{
    m_attributes = attr;
}

void JobTuiItem::setVisible(bool visible)
{
    m_visible = visible;
}

bool JobTuiItem::isVisible() const
{
    return m_visible;
}

void JobTuiItem::setZ(int z)
{
    if (z < 0)
        z = 0;

    if (z > 100)
        z = 100;

    m_z = z;
}

int JobTuiItem::z() const
{
    return m_z;
}

void JobTuiItem::setY(int y)
{
    m_y = y;
}

int JobTuiItem::y() const
{
    return m_y;
}

void JobTuiItem::setX(int x)
{
    m_x = x;
}

int JobTuiItem::x() const
{
    return m_x;
}

void JobTuiItem::setWidth(int width)
{
    m_width = width;
}

int JobTuiItem::width() const
{
    return m_width;
}

void JobTuiItem::setHeight(int height)
{
    m_height = height;
}

int JobTuiItem::height() const
{
    return m_height;
}

bool JobTuiItem::isFocusable() const
{
    return m_focusable;
}

void JobTuiItem::setFocusable(bool focusable)
{
    m_focusable = focusable;
}

JobTuiAnchors &JobTuiItem::anchors()
{
    return m_anchors;
}

const JobTuiAnchors &JobTuiItem::anchors() const
{
    return m_anchors;
}

void JobTuiItem::setAnchors(JobTuiAnchors anchors)
{
    m_anchors = anchors;
}

void JobTuiItem::resolveAnchors()
{
    if (!m_anchors.isAnchored())
        return;

    // centerIn overrides horizontal/verticalCenter
    if (m_anchors.centerIn) {
        m_anchors.horizontalCenter = m_anchors.centerIn;
        m_anchors.verticalCenter = m_anchors.centerIn;
    }

    // fill takes full control (early return)
    if (m_anchors.fill) {
        JobTuiItem *target = m_anchors.fill;
        if (target) {
            setX(target->x() + m_anchors.leftMargin);
            setY(target->y() + m_anchors.topMargin);
            setWidth(target->width() - m_anchors.leftMargin - m_anchors.rightMargin);
            setHeight(target->height() - m_anchors.topMargin - m_anchors.bottomMargin);
        }
        return;
    }

    // Positional anchors
    if (m_anchors.left)
        setX(m_anchors.left->x() + m_anchors.leftMargin);
    else if (m_anchors.right)
        setX(m_anchors.right->x() + m_anchors.right->width() - width() - m_anchors.rightMargin);


    if (m_anchors.top)
        setY(m_anchors.top->y() + m_anchors.topMargin);
    else if (m_anchors.bottom)
        setY(m_anchors.bottom->y() + m_anchors.bottom->height() - height() - m_anchors.bottomMargin);


    // 4: Center alignment overrides left/right or top/bottom
    if (m_anchors.horizontalCenter) {
        JobTuiItem *target = m_anchors.horizontalCenter;
        int targetMid = target->x() + target->width() / 2;
        setX(targetMid - width() / 2);
    }

    if (m_anchors.verticalCenter) {
        JobTuiItem *target = m_anchors.verticalCenter;
        int targetMid = target->y() + target->height() / 2;
        setY(targetMid - height() / 2);
    }
}

void JobTuiItem::setLayout(LayoutEngine::Ptr engine)
{
    m_layoutEngine = std::move(engine);
}

LayoutEngine *JobTuiItem::layout() const
{
    return m_layoutEngine.get();
}

void JobTuiItem::applyLayout()
{
    if (m_layoutEngine.get())
        m_layoutEngine.get()->applyLayout();
}

bool JobTuiItem::mouseEvent([[maybe_unused]] const Event &event)
{
    return false;
}

void JobTuiItem::setAcceptsMouseEvents(bool enabled)
{
    m_acceptsMouseEvents = enabled;
}

bool JobTuiItem::acceptsMouseEvents() const
{
    return m_acceptsMouseEvents;
}

bool JobTuiItem::itemHasContents() const
{
    return m_itemHasContents;
}

void JobTuiItem::setItemHasContents(bool itemHasContents)
{
    m_itemHasContents = itemHasContents;
}

void JobTuiItem::setFocus(bool focus)
{
    // std::cout << " Set Focus " << isFocusable() << m_focus << focus << "\n";

    if(isFocusable() && m_focus != focus){
        m_focus = focus;
        if(onFocusChanged)
            onFocusChanged(m_focus);
    }
}

bool JobTuiItem::focus() const
{
    return m_focus;
}

bool JobTuiItem::focusEvent(const Event &event)
{
    if (!isVisible() || !isFocusable())
        return false;

    if (m_focus && event.type() == Event::EventType::KeyPress)
        if (event.key() == "\t")
            return true;

    return false;
}

ansi::Attributes::Ptr JobTuiItem::attributes() const
{
    return m_attributes;
}

void JobTuiItem::paint(DrawContext &ctx, int parentX, int parentY)
{
    if (!m_visible || !m_itemHasContents)
        return;

    int absX = parentX + m_x;
    int absY = parentY + m_y;

    ctx.moveCursor(absX, absY);
    paintSelf(ctx);

    auto childrenList = childrenAs<JobTuiItem>();
    std::sort(childrenList.begin(), childrenList.end(), [](JobTuiItem *a, JobTuiItem *b) {
        return a->z() < b->z(); // Lower z
    });

    for (auto *child : childrenList)
        if (child && child->isVisible())
            child->paint(ctx, absX, absY);

}

void JobTuiItem::paint(DrawContext &ctx)
{
    paint(ctx, 0, 0);
}

void JobTuiItem::paintSelf(DrawContext &ctx)
{
    if (!m_attributes || !m_itemHasContents)
        return;

    if (m_attributes->foreground)
        ctx.setForeground(m_attributes->foreground.value());

    if (m_attributes->background)
        ctx.setBackground(m_attributes->background.value());

    for (int dy = 0; dy < m_height; ++dy) {
        ctx.moveCursor(m_x, m_y + dy);
        ctx.write(std::string(m_width, ' '));
    }

    ctx.resetColors();
}

}
