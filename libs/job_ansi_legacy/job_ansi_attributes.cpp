#include "job_ansi_attributes.h"
#include <shared_mutex>
#include <atomic>
namespace job::ansi {

Attributes::Attributes()
{
    reset();
}

bool Attributes::operator==(const Attributes &other) const noexcept
{
    if (flags != other.flags) return false;

    // Only compare color values if the flag says they exist
    if (hasFg && m_fg != other.m_fg) return false;
    if (hasBg && m_bg != other.m_bg) return false;

    return true;
}

bool Attributes::operator!=(const Attributes &other) const noexcept
{
    return !(*this == other);
}

const RGBColor *Attributes::getForeground() const noexcept
{
    return hasFg ? &m_fg : nullptr;
}

void Attributes::setForeground(const RGBColor &color) noexcept
{
    m_fg = color;
    hasFg = 1;
}
void Attributes::resetForeground() noexcept
{
    hasFg = 0;
}

const RGBColor *Attributes::getBackground() const noexcept
{
    return hasBg ? &m_bg : nullptr;
}

void Attributes::setBackground(const RGBColor &color) noexcept
{
    m_bg = color;
    hasBg = 1;
}

void Attributes::resetBackground() noexcept
{
    hasBg = 0;
}
void Attributes::resetColors() noexcept
{
    hasFg = 0;
    hasBg = 0;
}

void Attributes::setUnderline(UnderlineStyle style) noexcept
{
    underlineVal = static_cast<uint16_t>(style);
}


std::string_view Attributes::underlineToString(UnderlineStyle style) noexcept
{
    std::string_view ret = "unknown";
    switch (style) {
    case UnderlineStyle::None:
        ret = "none";
        break;
    case UnderlineStyle::Single:
        ret = "single";
        break;
    case UnderlineStyle::Double:
        ret =  "double";
        break;
    case UnderlineStyle::Curly:
        ret =  "curly";
        break;
    case UnderlineStyle::Dotted:
        ret =  "dotted";
        break;
    case UnderlineStyle::Dashed:
        ret =  "dashed";
        break;
    }
    return ret;
}


void Attributes::setBlink(uint8_t mode) noexcept
{
    blink = mode & 0x3;
}

UnderlineStyle Attributes::getUnderline() const noexcept
{
    return static_cast<UnderlineStyle>(underlineVal);
}

std::string Attributes::toString() const
{
    std::string str = "flags=" + std::to_string(flags);
    str += " ul=";
    str += underlineToString(getUnderline());

    if (hasFg) str += " fg=" + m_fg.toHexString();
    if (hasBg) str += " bg=" + m_bg.toHexString();

    return str;
}

void Attributes::reset()
{
    flags = 0;
}



Attributes::Ptr Attributes::intern(const Attributes &value)
{

    static std::unordered_map<Attributes, std::weak_ptr<Attributes>, Hash> cache;
    static std::shared_mutex mutex;
    static std::atomic<size_t> insertCount{0};
    constexpr size_t kGcThreshold = 1024; // Sweep every 1024 distinct attributes

    {
        std::shared_lock lock(mutex);
        auto it = cache.find(value);
        if (it != cache.end())
            if (auto shared = it->second.lock())
                return shared;
    }

    std::unique_lock lock(mutex);

    auto it = cache.find(value);
    if (it != cache.end()) {
        if (auto shared = it->second.lock())
            return shared;
        else
            cache.erase(it);
    }

    // GC
    if (insertCount.fetch_add(1, std::memory_order_relaxed) > kGcThreshold) {
        insertCount.store(0, std::memory_order_relaxed);
        for (auto i = cache.begin(); i != cache.end(); ) {
            if (i->second.expired())
                i = cache.erase(i);
            else
                ++i;
        }
    }

    auto shared = std::make_shared<Attributes>(value);
    cache.emplace(value, shared);
    return shared;
}

}
