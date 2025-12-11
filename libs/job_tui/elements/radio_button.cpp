#include "radio_button.h"

#include <algorithm>
#include <unordered_map>
#include <cassert>

#include "job_ansi_attributes.h"


namespace {
using ParentScope = job::tui::JobTuiItem*;
using RadioPtr    = job::tui::gui::RadioButton*;
struct Registry {
    std::unordered_map<ParentScope,
                       std::unordered_map<std::string, std::vector<RadioPtr>>> groups;

    static Registry &inst()
    {
        static Registry r;
        return r;
    }
};
}

namespace job::tui::gui {

RadioButton::Ptr RadioButton::create(JobTuiItem *parent)
{
    assert(parent && "RadioButton::create requires a non-null parent");
    auto ptr = std::shared_ptr<RadioButton>(new RadioButton(parent));
    parent->addChild(ptr);
    return ptr;
}

RadioButton::Ptr RadioButton::create(Rect rect,
                                     const std::string &text,
                                     JobTuiItem *parent)
{
    assert(parent && "RadioButton::create(rect, text, parent) requires non-null parent");
    auto ptr = RadioButton::Ptr(new RadioButton(parent));
    ptr->setGeometry(rect);
    ptr->setText(text);
    parent->addChild(ptr);
    return ptr;
}

RadioButton::RadioButton(JobTuiItem *parent) :
    JobTuiItem{parent},
    m_parentScope(parent)
{
    assert(m_parentScope && "RadioButton requires a non-null parent");
    setItemHasContents(true);
    setFocusable(true);
    setAcceptsMouseEvents(true);
    registerInGroup();
}

RadioButton::~RadioButton()
{
    unregisterFromGroup();
}

void RadioButton::setText(const std::string &text)
{
    m_text = text;
}

const std::string &RadioButton::text() const
{
    return m_text;
}

void RadioButton::setGroupName(const std::string &name)
{
    if (name == m_groupName)
        return;

    unregisterFromGroup();
    m_groupName = std::move(name);
    registerInGroup();
}

const std::string &RadioButton::groupName() const
{
    return m_groupName;
}

void RadioButton::setChecked(bool on)
{
    if (m_checked == on)
        return;

    if (on) {
        // Uncheck peers under SAME (parentScope, groupName)
        // This is fucked
        auto &rg = Registry::inst().groups;
        auto itScope = rg.find(m_parentScope);
        if (itScope != rg.end()) {
            auto itGroup = itScope->second.find(m_groupName);
            if (itGroup != itScope->second.end()) {
                for (auto *rb : itGroup->second) {
                    if (rb != this && rb->m_checked) {
                        rb->m_checked = false;
                        if (rb->onToggled) rb->onToggled(false);
                    }
                }
            }
        }
    }

    m_checked = on;
    if (onToggled) onToggled(m_checked);
}

bool RadioButton::isChecked() const
{
    return m_checked;
}


void RadioButton::registerInGroup()
{
    auto &vec = Registry::inst().groups[m_parentScope][m_groupName];
    if (std::find(vec.begin(), vec.end(), this) == vec.end())
        vec.push_back(this);

    int checkedCount = 0;
    RadioPtr firstChecked = nullptr;
    for (auto *rb : vec) {
        if (rb->m_checked) {
            ++checkedCount;
            if (!firstChecked)
                firstChecked = rb;
        }
    }
    if (checkedCount > 1) {
        for (auto *rb : vec) {
            if (rb != firstChecked && rb->m_checked) {
                rb->m_checked = false;
                if (rb->onToggled)
                    rb->onToggled(false);
            }
        }
    }
}

void RadioButton::unregisterFromGroup()
{
    auto &rg = Registry::inst().groups;
    auto itScope = rg.find(m_parentScope);
    if (itScope == rg.end())
        return;

    auto &gm = itScope->second;
    auto itGroup = gm.find(m_groupName);
    if (itGroup == gm.end())
        return;

    auto &vec = itGroup->second;
    vec.erase(std::remove(vec.begin(), vec.end(), this), vec.end());

    if (vec.empty())
        gm.erase(itGroup);

    if (gm.empty())
        rg.erase(itScope);
}


bool RadioButton::mouseEvent(const Event &event)
{
    if (!isVisible())
        return false;

    const int ex = event.globalX();
    const int ey = event.globalY();

    const int x0 = x();
    const int y0 = y();

    const int w  = width();
    const int h  = height();

    if (ex >= x0 && ex < x0 + w && ey >= y0 && ey < y0 + h) {
        if (event.type() == Event::EventType::MousePress) {
            setChecked(true);
            if (onClicked)
                onClicked();

            return true;
        }
        return true;
    }
    return false;
}

bool RadioButton::focusEvent(const Event &e)
{
    if (!isVisible() || !isFocusable())
        return false;

    if (e.type() == Event::EventType::KeyPress) {
        const std::string k = e.key();
        if (k == " " || k == "Enter" || k == "\r") {
            setChecked(true);
            return true;
        }

        if (k == "Left" || k == "Up" || k == "Right" || k == "Down") {
            auto &rg = Registry::inst().groups;
            auto itScope = rg.find(m_parentScope);
            if (itScope != rg.end()) {
                auto itGroup = itScope->second.find(m_groupName);
                if (itGroup != itScope->second.end()) {
                    auto &vec = itGroup->second;
                    if (vec.size() > 1) {
                        auto it = std::find(vec.begin(), vec.end(), this);
                        if (it != vec.end()) {
                            int idx = int(std::distance(vec.begin(), it));
                            int dir = (k == "Left" || k == "Up") ? -1 : +1;
                            idx = (idx + dir + int(vec.size())) % int(vec.size());
                            vec[size_t(idx)]->setFocus(true);
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}


void RadioButton::paintSelf(DrawContext &ctx)
{
    if (!isVisible())
        return;

    paintGlyphAndLabel(ctx);
}

void RadioButton::paintGlyphAndLabel(DrawContext &ctx)
{
    const bool f  = focus();
    const bool on = m_checked;

    const int gx = x();              // local (parent already adjusted origin)
    const int gy = y();

    const int labelX = gx + 4;       // "( ) " => 4 cells

    ansi::Attributes baseAttr;
    if (auto attr = attributes())
        baseAttr = *attr;

    // Focus cue: invert around the glyph only (cheap and readable)
    ansi::Attributes glyphAttr = baseAttr;
    if (f)
        glyphAttr.inverse = true;

    // Glyph
    ctx.moveCursor(gx, gy);
    ctx.resetColors();
    ctx.apply(glyphAttr);
    ctx.write(on ? "(•)" : "( )");

    // Label
    ctx.resetColors();
    ctx.apply(baseAttr);

    int maxW = std::max(0, width() - (labelX - gx));
    std::string shown = m_text;
    if (maxW > 0 && int(shown.size()) > maxW)
        shown = shown.substr(0, std::max(0, maxW - 1)) + "…";

    ctx.moveCursor(labelX, gy);
    if (maxW > 0)
        ctx.write(shown);
}


} 
