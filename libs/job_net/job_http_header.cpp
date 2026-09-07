#include "job_http_header.h"

#include <algorithm>
#include <cctype>

#include <job_logger.h>

namespace job::net {

JobHttpHeader::JobHttpHeader() = default;

JobHttpHeader::JobHttpHeader(std::string_view name, std::string_view value)
{
    if (!append(name, value))
        JOB_LOG_ERROR("JobHttpHeader: rejected init field '{}'", std::string(name));
}

JobHttpHeader::JobHttpHeader(const JobHttpHeader &other) = default;
JobHttpHeader::JobHttpHeader(JobHttpHeader &&other) noexcept = default;
JobHttpHeader::~JobHttpHeader() = default;

JobHttpHeader &JobHttpHeader::operator=(const JobHttpHeader &other) = default;
JobHttpHeader &JobHttpHeader::operator=(JobHttpHeader &&other) noexcept = default;

std::string JobHttpHeader::normalizeKey(std::string_view input)
{
    std::string key(input);
    for (char &c : key)
        c = lowerAscii(c);
    return key;
}

bool JobHttpHeader::equalsIgnoreCase(std::string_view a, std::string_view b) noexcept
{
    if (a.size() != b.size())
        return false;

    for (std::size_t i = 0; i < a.size(); ++i) {
        if (lowerAscii(a[i]) != lowerAscii(b[i]))
            return false;
    }
    return true;
}

bool JobHttpHeader::isValidFieldName(std::string_view name) noexcept
{
    if (name.empty())
        return false;

    return std::all_of(name.begin(), name.end(),
                       [](char c) { return isTChar(static_cast<unsigned char>(c)); });
}

bool JobHttpHeader::isValidFieldValue(std::string_view value) noexcept
{
    return std::none_of(value.begin(), value.end(),
                        [](char c) { return c == '\0' || c == '\r' || c == '\n'; });
}

// Stupid Cookie .....
bool JobHttpHeader::isCombinable(std::string_view name) noexcept
{
    return !equalsIgnoreCase(name, "set-cookie");
}

std::string_view JobHttpHeader::trimOws(std::string_view v) noexcept
{
    const auto isOws = [](char c) {
        return c == ' ' || c == '\t';
    };

    while (!v.empty() && isOws(v.front()))
        v.remove_prefix(1);

    while (!v.empty() && isOws(v.back()))
        v.remove_suffix(1);

    return v;
}

bool JobHttpHeader::makeField(std::string_view name, std::string_view value, Field &out) const
{
    if (!isValidFieldName(name)) {
        JOB_LOG_WARN("JobHttpHeader: invalid field name");
        return false;
    }

    const std::string_view trimmed = trimOws(value);
    if (!isValidFieldValue(trimmed)) {
        JOB_LOG_WARN("JobHttpHeader: invalid field value (control character)");
        return false;
    }

    out.name = normalizeKey(name);
    out.displayName.assign(name);
    out.value.assign(trimmed);
    return true;
}


std::string JobHttpHeader::toString() const
{
    std::size_t needed = 2;
    for (const Field &f : m_fields)
        needed += f.displayName.size() + f.value.size() + 4;

    std::string out;
    out.reserve(needed);

    for (const Field &f : m_fields) {
        out += f.displayName;
        out += ": ";
        out += f.value;
        out += "\r\n";
    }
    out += "\r\n";

    return out;
}


std::size_t JobHttpHeader::indexOf(std::string_view name) const noexcept
{
    for (std::size_t i = 0; i < m_fields.size(); ++i) {
        if (equalsIgnoreCase(m_fields[i].name, name))
            return i;
    }
    return npos;
}

std::size_t JobHttpHeader::lastIndexOf(std::string_view name) const noexcept
{
    for (std::size_t i = m_fields.size(); i-- > 0;) {
        if (equalsIgnoreCase(m_fields[i].name, name))
            return i;
    }
    return npos;
}

bool JobHttpHeader::contains(std::string_view name) const noexcept
{
    return indexOf(name) != npos;
}

bool JobHttpHeader::contains(JobIana::IanaHeaders name) const noexcept
{
    return contains(JobIana::toString(name));
}

std::size_t JobHttpHeader::count(std::string_view name) const noexcept
{
    std::size_t n = 0;
    for (const Field &f : m_fields) {
        if (equalsIgnoreCase(f.name, name))
            ++n;
    }
    return n;
}

std::size_t JobHttpHeader::count(JobIana::IanaHeaders name) const noexcept
{
    return count(JobIana::toString(name));
}

std::string_view JobHttpHeader::value(std::string_view name, std::string_view defaultVal) const noexcept
{
    const std::size_t pos = indexOf(name);
    return pos == npos ? defaultVal : std::string_view(m_fields[pos].value);
}

std::string_view JobHttpHeader::value(JobIana::IanaHeaders name, std::string_view defaultVal) const noexcept
{
    return value(JobIana::toString(name), defaultVal);
}

std::string_view JobHttpHeader::lastValue(std::string_view name, std::string_view defaultVal) const noexcept
{
    const std::size_t pos = lastIndexOf(name);
    return pos == npos ? defaultVal : std::string_view(m_fields[pos].value);
}

std::string_view JobHttpHeader::lastValue(JobIana::IanaHeaders name, std::string_view defaultVal) const noexcept
{
    return lastValue(JobIana::toString(name), defaultVal);
}

std::vector<std::string_view> JobHttpHeader::values(std::string_view name) const
{
    std::vector<std::string_view> out;
    for (const Field &f : m_fields) {
        if (equalsIgnoreCase(f.name, name))
            out.emplace_back(f.value);
    }
    return out;
}

std::vector<std::string_view> JobHttpHeader::values(JobIana::IanaHeaders name) const
{
    return values(JobIana::toString(name));
}

std::string JobHttpHeader::joinedValue(std::string_view name, std::string_view sep) const
{
    std::string out;
    bool first = true;

    for (const Field &f : m_fields) {
        if (!equalsIgnoreCase(f.name, name))
            continue;

        if (!first)
            out += sep;
        out += f.value;
        first = false;
    }
    return out;
}

std::string JobHttpHeader::joinedValue(JobIana::IanaHeaders name, std::string_view sep) const
{
    return joinedValue(JobIana::toString(name), sep);
}

std::vector<std::string_view> JobHttpHeader::listMembers(std::string_view name) const
{
    std::vector<std::string_view> out;

    for (const Field &f : m_fields) {
        if (!equalsIgnoreCase(f.name, name))
            continue;

        std::string_view rest(f.value);
        for (;;) {
            const std::size_t comma = rest.find(',');
            if (comma == std::string_view::npos) {
                out.push_back(trimOws(rest));
                break;
            }
            out.push_back(trimOws(rest.substr(0, comma)));
            rest.remove_prefix(comma + 1);
        }
    }
    return out;
}

std::vector<std::string_view> JobHttpHeader::listMembers(JobIana::IanaHeaders name) const
{
    return listMembers(JobIana::toString(name));
}

const JobHttpHeader::Field *JobHttpHeader::fieldAt(std::size_t pos) const noexcept
{
    return pos < m_fields.size() ? &m_fields[pos] : nullptr;
}

std::string_view JobHttpHeader::nameAt(std::size_t pos) const noexcept
{
    return pos < m_fields.size() ? std::string_view(m_fields[pos].name) : std::string_view{};
}

std::string_view JobHttpHeader::valueAt(std::size_t pos) const noexcept
{
    return pos < m_fields.size() ? std::string_view(m_fields[pos].value) : std::string_view{};
}

bool JobHttpHeader::append(std::string_view name, std::string_view value)
{
    try {
        Field f;
        if (!makeField(name, value, f))
            return false;

        m_fields.push_back(std::move(f));
        return true;
    } catch (const std::exception &e) {
        JOB_LOG_ERROR("JobHttpHeader::append() exception: {}", e.what());
    } catch (...) {
        JOB_LOG_ERROR("JobHttpHeader::append() unknown exception");
    }
    return false;
}

bool JobHttpHeader::append(JobIana::IanaHeaders name, std::string_view value)
{
    return append(JobIana::toString(name), value);
}

bool JobHttpHeader::prepend(std::string_view name, std::string_view value)
{
    return insert(name, value, 0);
}

bool JobHttpHeader::prepend(JobIana::IanaHeaders name, std::string_view value)
{
    return prepend(JobIana::toString(name), value);
}

bool JobHttpHeader::insert(std::string_view name, std::string_view value, std::size_t pos)
{
    try {
        Field f;
        if (!makeField(name, value, f))
            return false;

        if (pos > m_fields.size())
            pos = m_fields.size();

        m_fields.insert(m_fields.begin() + static_cast<std::ptrdiff_t>(pos), std::move(f));
        return true;
    } catch (const std::exception &e) {
        JOB_LOG_ERROR("JobHttpHeader::insert() exception: {}", e.what());
    } catch (...) {
        JOB_LOG_ERROR("JobHttpHeader::insert() unknown exception");
    }
    return false;
}

bool JobHttpHeader::insert(JobIana::IanaHeaders name, std::string_view value, std::size_t pos)
{
    return insert(JobIana::toString(name), value, pos);
}

bool JobHttpHeader::set(std::string_view name, std::string_view value)
{
    try {
        Field f;
        if (!makeField(name, value, f))
            return false;

        const std::size_t pos = indexOf(f.name);
        if (pos == npos) {
            m_fields.push_back(std::move(f));
            return true;
        }

        // Drop every later duplicate, then overwrite the first occurrence so
        // the field keeps its original position in the block.
        const std::string key = f.name;
        m_fields.erase(std::remove_if(m_fields.begin() + static_cast<std::ptrdiff_t>(pos) + 1,
                                      m_fields.end(),
                                      [&key](const Field &e) {
                                          return e.name == key;
                                      }),
                       m_fields.end());

        m_fields[pos] = std::move(f);
        return true;
    } catch (const std::exception &e) {
        JOB_LOG_ERROR("JobHttpHeader::set() exception: {}", e.what());
    } catch (...) {
        JOB_LOG_ERROR("JobHttpHeader::set() unknown exception");
    }
    return false;
}

bool JobHttpHeader::set(JobIana::IanaHeaders name, std::string_view value)
{
    return set(JobIana::toString(name), value);
}

bool JobHttpHeader::appendToList(std::string_view name, std::string_view value)
{
    try {
        if (!isCombinable(name)) {
            JOB_LOG_WARN("JobHttpHeader::appendToList() refused for non-combinable field");
            return false;
        }

        Field f;
        if (!makeField(name, value, f))
            return false;

        const std::size_t pos = lastIndexOf(f.name);
        if (pos == npos) {
            m_fields.push_back(std::move(f));
            return true;
        }

        std::string &existing = m_fields[pos].value;
        if (!existing.empty() && !f.value.empty())
            existing += ", ";
        existing += f.value;
        return true;
    } catch (const std::exception &e) {
        JOB_LOG_ERROR("JobHttpHeader::appendToList() exception: {}", e.what());
    } catch (...) {
        JOB_LOG_ERROR("JobHttpHeader::appendToList() unknown exception");
    }
    return false;
}

bool JobHttpHeader::appendToList(JobIana::IanaHeaders name, std::string_view value)
{
    return appendToList(JobIana::toString(name), value);
}

bool JobHttpHeader::replace(std::size_t pos, std::string_view name, std::string_view value)
{
    try {
        if (pos >= m_fields.size()) {
            JOB_LOG_WARN("JobHttpHeader::replace() invalid position: {}", pos);
            return false;
        }

        Field f;
        if (!makeField(name, value, f))
            return false;

        m_fields[pos] = std::move(f);
        return true;
    } catch (const std::exception &e) {
        JOB_LOG_ERROR("JobHttpHeader::replace() exception: {}", e.what());
    } catch (...) {
        JOB_LOG_ERROR("JobHttpHeader::replace() unknown exception");
    }
    return false;
}

bool JobHttpHeader::replace(std::size_t pos, JobIana::IanaHeaders name, std::string_view value)
{
    return replace(pos, JobIana::toString(name), value);
}

bool JobHttpHeader::removeAt(std::size_t pos)
{
    if (pos >= m_fields.size()) {
        JOB_LOG_WARN("JobHttpHeader::removeAt() invalid position: {}", pos);
        return false;
    }

    m_fields.erase(m_fields.begin() + static_cast<std::ptrdiff_t>(pos));
    return true;
}

std::size_t JobHttpHeader::removeAll(std::string_view name)
{
    const auto it = std::remove_if(m_fields.begin(), m_fields.end(),
                                   [name](const Field &f) {
                                       return equalsIgnoreCase(f.name, name);
                                   });

    const std::size_t removed = static_cast<std::size_t>(std::distance(it, m_fields.end()));
    m_fields.erase(it, m_fields.end());
    return removed;
}

std::size_t JobHttpHeader::removeAll(JobIana::IanaHeaders name)
{
    return removeAll(JobIana::toString(name));
}

void JobHttpHeader::clear() noexcept
{
    m_fields.clear();
}

void JobHttpHeader::reserve(std::size_t n)
{
    m_fields.reserve(n);
}


bool JobHttpHeader::isEmpty() const noexcept
{
    return m_fields.empty();
}

std::size_t JobHttpHeader::size() const noexcept
{
    return m_fields.size();
}

std::size_t JobHttpHeader::count() const noexcept
{
    return m_fields.size();
}

} // namespace job::net
