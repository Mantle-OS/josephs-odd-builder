#include "job_ansi_cell.h"

#include <wchar.h>

namespace job::ansi {

Cell::Cell() :
    m_char(U' '), 
    m_charWidth(1)
{}

Cell::Cell(char32_t ch)
{
    setChar(ch);
}

Cell::Cell(char32_t ch, Attributes::Ptr attr) :
    m_attr(attr),
    m_char(U' '),
    m_charWidth(1)
{
    if(!m_attr)
        m_attr = defaultAttributes();

    setChar(ch);
}

Cell::Cell(const Cell &other)
{
    *this = other;
}

Cell &Cell::operator=(const Cell &other)
{
    m_dirty = other.m_dirty;
    m_char = other.m_char;
    m_marks = other.m_marks;
    m_attr = other.m_attr;
    m_charWidth = other.m_charWidth;
    m_trailing = other.m_trailing;
    m_lineMode = other.m_lineMode;
    m_lineWidth = other.m_lineWidth;
    m_lineHeight = other.m_lineHeight;
    m_isTopHalf = other.m_isTopHalf;
    m_protectWrite = other.m_protectWrite;
    m_protectErase = other.m_protectErase;
    m_dirty = true;  // always mark new cells as dirty for repaint

    return *this;
}

int Cell::width() const
{
    return m_charWidth;
}

bool Cell::isTrailing() const
{
    return m_trailing;
}

void Cell::setTrailing(bool trailing)
{
    m_trailing = trailing;
}

std::u32string Cell::marks() const
{
    return m_marks;
}

void Cell::clearMarks()
{
    if (!m_marks.empty()) {
        m_marks.clear();
        m_dirty = true;
    }
}

void Cell::appendCombiningChar(char32_t ch)
{
    m_marks.push_back(ch);
    m_dirty = true;
}

bool Cell::dirty() const
{
    return m_dirty;
}

void Cell::setDirty(bool dirty) {
    if(m_dirty != dirty)
        m_dirty = dirty;
}

std::u32string Cell::fullCharSequence() const
{
    std::u32string result;
    result += m_char;
    result += m_marks;
    return result;
}

void Cell::setChar(char32_t ch)
{
    if (m_char != ch) {
        m_char = ch;
        m_marks.clear();
        m_charWidth = std::max(1, wcwidth(ch));  // fallback to 1 if wcwidth() fails
        m_trailing = false;
        m_dirty = true;
    }
}

const Cell &Cell::blank()
{
    static Cell blankCell(U' ', defaultAttributes());
    return blankCell;
}

void Cell::setAttributes(const Attributes &attr)
{
    if (!m_attr || *m_attr != attr) {
        m_attr = Attributes::intern(attr);
        m_dirty = true;
    }
}

const Attributes *Cell::attributes() const
{
    return m_attr ? m_attr.get() : defaultAttributes().get();
}


void Cell::reset()
{
    setChar(U' ');
    resetAttributes();
    m_lineMode = LineDisplayMode::SingleWidth;
    m_lineWidth = 1;
    m_lineHeight = 1;
    m_isTopHalf = false;
    m_dirty = true;
    m_protectErase = false;
    m_protectWrite = false;
}

std::shared_ptr<Attributes> Cell::defaultAttributes()
{
    static auto shared = std::make_shared<Attributes>();
    return shared;
}

char32_t Cell::getChar() const
{
    return m_char;
}

void Cell::resetAttributes()
{
    m_attr = defaultAttributes();
    m_dirty = true;
}

void Cell::setForeground(const RGBColor &color)
{
    Attributes modified = *attributes();
    modified.setForeground(color);
    m_attr = Attributes::intern(modified);
    m_dirty = true;
}

void Cell::setBackground(const RGBColor &color)
{
    Attributes modified = *attributes();
    modified.setBackground(color);
    m_attr = Attributes::intern(modified);
    m_dirty = true;
}

void Cell::resetColors()
{
    if (m_attr) {
        Attributes modified = *m_attr;
        modified.resetColors();
        m_attr = Attributes::intern(modified);
        m_dirty = true;
    }
}

bool Cell::isRenderable() const
{
    return !m_trailing && m_char != U' ';
}

// Empty means it contains only a space and no combining marks
bool Cell::isEmpty() const
{
    return m_char == U' ' && m_marks.empty();
}

// Guaranteed non-null attributes (fly-weight default if none set)
const Attributes &Cell::attributesOrDefault() const
{
    return m_attr ? *m_attr : *defaultAttributes();
}

// Line-mode helpers (used by Screen::setLineDisplayMode etc.)
void Cell::setLineDisplayMode(LineDisplayMode mode)
{
    m_lineMode = mode;
}
LineDisplayMode Cell::getLineDisplayMode() const
{
    return m_lineMode;
}

void Cell::setLineWidth(int width)
{
    m_lineWidth  = std::max(1, width);
}
int  Cell::getLineWidth()  const
{
    return m_lineWidth;
}

void Cell::setLineHeight(int height)
{
    m_lineHeight = std::max(1, height);
}
int  Cell::getLineHeight() const
{
    return m_lineHeight;
}

void Cell::setLineHeightPosition(bool isTop)
{
    m_isTopHalf = isTop;
}
bool Cell::isTopHalf() const
{
    return m_isTopHalf;
}

void Cell::setProtection(bool erase, bool write)
{
    m_protectErase = erase;
    m_protectWrite = write;
}

bool Cell::isProtectedForErase() const
{
    return m_protectErase;
}
bool Cell::isProtectedForWrite() const
{
    return m_protectWrite;
}

}
