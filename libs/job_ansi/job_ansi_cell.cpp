#include "job_ansi_cell.h"

#include <wchar.h>

namespace job::ansi {

Cell::Cell() :
    m_char(U' ')
{}

Cell::Cell(char32_t ch) :
    m_char(ch)
{
}

Cell::Cell(char32_t ch, Attributes::Ptr attr, int width) noexcept :
    m_attr(std::move(attr)),
    m_char(ch)
{
    m_charWidth = width;
    if (!m_attr) {
        m_attr = defaultAttributes();
    }
}

Cell::Cell(const Cell &other) :
    m_attr(other.m_attr),
    m_char(other.m_char)
{
    // Copy Bitfields
    m_charWidth    = other.m_charWidth;
    m_lineWidth    = other.m_lineWidth;
    m_lineHeight   = other.m_lineHeight;
    m_lineMode     = other.m_lineMode;
    m_dirty        = true; // New cell is dirty
    m_trailing     = other.m_trailing;
    m_isTopHalf    = other.m_isTopHalf;
    m_protectErase = other.m_protectErase;
    m_protectWrite = other.m_protectWrite;

    // Deep copy marks if they exist
    if (other.m_marks) {
        m_marks = std::make_unique<std::u32string>(*other.m_marks);
    }
}

Cell &Cell::operator=(const Cell &other)
{
    m_dirty = other.m_dirty;
    m_char = other.m_char;
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

    if (other.m_marks)
        m_marks = std::make_unique<std::u32string>(*other.m_marks);

    return *this;
}


Cell::Cell(Cell &&other) noexcept = default;

bool Cell::hasMarks() const noexcept
{
    return m_marks != nullptr;
}

int Cell::width() const noexcept
{
    return m_charWidth;
}

bool Cell::isTrailing() const noexcept
{
    return m_trailing;
}

void Cell::setTrailing(bool trailing)
{
    m_trailing = trailing;
}

std::u32string Cell::marks() const
{
    return m_marks ? *m_marks : std::u32string{};
}

void Cell::clearMarks()
{
    if (m_marks) {
        m_marks.reset();
        m_dirty = true;
    }
}

void Cell::appendCombiningChar(char32_t ch)
{
    if (!m_marks)
        m_marks = std::make_unique<std::u32string>();

    m_marks->push_back(ch);
    m_dirty = true;
}


std::u32string Cell::fullCharSequence() const
{
    std::u32string result;
    result += m_char;
    if (m_marks) {
        result += *m_marks;
    }
    return result;
}


const Cell &Cell::blank()
{
    static Cell blankCell(U' ', defaultAttributes());
    return blankCell;
}


const Attributes *Cell::attributes() const noexcept
{
    return m_attr ? m_attr.get() : defaultAttributes().get();
}

std::shared_ptr<Attributes> Cell::defaultAttributes()
{
    static auto shared = std::make_shared<Attributes>();
    return shared;
}
// Guaranteed non-null attributes (fly-weight default if none set)
const Attributes &Cell::attributesOrDefault() const noexcept
{
    return m_attr ? *m_attr : *defaultAttributes();
}
void Cell::setAttributes(const Attributes &attr)
{
    if (!m_attr || *m_attr != attr) {
        m_attr = Attributes::intern(attr);
        m_dirty = true;
    }
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




void Cell::reset() noexcept
{
    m_char = U' ';
    m_charWidth = 1;
    resetAttributes();
    m_marks.reset();

    m_lineMode = 0; // SingleWidth (assuming enum 0)
    m_lineWidth = 1;
    m_lineHeight = 1;
    m_isTopHalf = false;
    m_dirty = true;
    m_protectErase = false;
    m_protectWrite = false;
    m_trailing = false;
}

void Cell::resetAttributes() noexcept
{
    m_attr = defaultAttributes();
    m_dirty = true;
}


bool Cell::isRenderable() const noexcept
{
    return !m_trailing && m_char != U' ';
}

// Empty means it contains only a space and no combining marks
bool Cell::isEmpty() const
{
    return m_char == U' ' && !m_marks;
}

bool Cell::dirty() const noexcept
{
    return m_dirty;
}

void Cell::setDirty(bool dirty)
{
    m_dirty = dirty;
}


char32_t Cell::getChar() const noexcept
{
    return m_char;
}

void Cell::setChar(char32_t ch, int width)
{
    if (m_char != ch) {
        m_char = ch;
        // Optimization: Removing libc dependency. Width is passed in.
        m_charWidth = width;

        m_marks.reset(); // Clear combining marks on new base char
        m_trailing = false;
        m_dirty = true;
    }
}

void Cell::setLineDisplayMode(LineDisplayMode mode) noexcept
{
    m_lineMode = static_cast<uint32_t>(mode);
}

LineDisplayMode Cell::getLineDisplayMode() const noexcept
{
    return static_cast<LineDisplayMode>(m_lineMode);
}

void Cell::setLineWidth(int width) noexcept
{
    m_lineWidth  = std::max(1, width);
}

int  Cell::getLineWidth()  const noexcept
{
    return m_lineWidth;
}

void Cell::setLineHeight(int height) noexcept
{
    m_lineHeight = std::max(1, height);
}

int Cell::getLineHeight() const noexcept
{
    return m_lineHeight;
}

void Cell::setLineHeightPosition(bool isTop) noexcept
{
    m_isTopHalf = isTop;
}

bool Cell::isTopHalf() const noexcept
{
    return m_isTopHalf;
}

void Cell::setProtection(bool erase, bool write) noexcept
{
    m_protectErase = erase;
    m_protectWrite = write;
}

bool Cell::isProtectedForErase() const noexcept
{
    return m_protectErase;
}
bool Cell::isProtectedForWrite() const noexcept
{
    return m_protectWrite;
}

}
