#include "job_http_header.h"
#include <iostream>
#include <sstream>

namespace job::net {

JobHttpHeader::JobHttpHeader() = default;

JobHttpHeader::JobHttpHeader(std::string_view name, std::string_view value)
{
    if(!append(name, value))
        std::cerr << "Unable to append to headers\n";
}

JobHttpHeader::JobHttpHeader(const JobHttpHeader &other)
    : m_headers(other.m_headers)
{
}

JobHttpHeader::~JobHttpHeader() = default;

JobHttpHeader &JobHttpHeader::operator=(JobHttpHeader &&other)
{
    if (this != &other) {
        m_headers = std::move(other.m_headers);
    }
    return *this;
}

JobHttpHeader &JobHttpHeader::operator=(const JobHttpHeader &other)
{
    if (this != &other) {
        m_headers = other.m_headers;
    }
    return *this;
}

std::string JobHttpHeader::toString() const
{
    std::ostringstream oss;
    for (const auto &pair : m_headers) {
        const auto &hv = pair.second;
        oss << hv.displayKey << ": " << hv.value << "\r\n";
    }
    oss << "\r\n"; // terminate header section
    return oss.str();
}

bool JobHttpHeader::contains(std::string_view name) const
{
    return m_headers.find(normalizeKey(name)) != m_headers.end();
}

bool JobHttpHeader::contains(JobIana::IanaHeaders name) const
{
    return contains(JobIana::toString(name));
}

bool JobHttpHeader::append(std::string_view name, std::string_view value)
{
    std::string key = normalizeKey(name);
    auto it = m_headers.find(key);
    if (it == m_headers.end()) {
        m_headers.emplace(std::move(key), HeaderValue{std::string(name), std::string(value)});
        return true;
    }
    it->second.value += ", " + std::string(value);
    return true;
}

bool JobHttpHeader::append(JobIana::IanaHeaders name, std::string_view value)
{
    return append(JobIana::toString(name), value);
}

bool JobHttpHeader::prepend(std::string_view name, std::string_view value)
{
    std::string key = normalizeKey(name);
    auto it = m_headers.find(key);
    if (it == m_headers.end()) {
        m_headers.emplace(std::move(key), HeaderValue{std::string(name), std::string(value)});
        return true;
    }
    it->second.value = std::string(value) + ", " + it->second.value;
    return true;
}


bool JobHttpHeader::prepend(JobIana::IanaHeaders name, std::string_view value)
{
    return prepend(JobIana::toString(name), value);
}

bool JobHttpHeader::insert(std::string_view name, std::string_view value, uint16_t /*pos*/)
{
    // HTTP headers are unordered by RFC semantics; insert acts like append.
    return append(name, value);
}

bool JobHttpHeader::insert(JobIana::IanaHeaders name, std::string_view value, uint16_t pos)
{
    return insert(JobIana::toString(name), value, pos);
}

bool JobHttpHeader::replace(size_t /*pos*/, std::string_view name, std::string_view val)
{
    std::string key = normalizeKey(name);
    auto it = m_headers.find(key);
    if (it != m_headers.end()) {
        it->second.value = std::string(val);
        it->second.displayKey = std::string(name);
        return true;
    }
    return false;
}


bool JobHttpHeader::replace(size_t pos, JobIana::IanaHeaders name, std::string_view val)
{
    return replace(pos, JobIana::toString(name), val);
}

void JobHttpHeader::removeAt(uint16_t pos)
{
    if (pos >= m_headers.size())
        return;

    auto it = m_headers.begin();
    std::advance(it, pos);
    m_headers.erase(it);
}

void JobHttpHeader::removeAll(std::string_view name)
{
    m_headers.erase(normalizeKey(name));
}

void JobHttpHeader::removeAll(JobIana::IanaHeaders name)
{
    removeAll(JobIana::toString(name));
}

bool JobHttpHeader::isEmpty() const noexcept
{
    return m_headers.empty();
}

size_t JobHttpHeader::size() const noexcept
{
    return m_headers.size();
}

size_t JobHttpHeader::count() const noexcept
{
    return m_headers.size();
}

void JobHttpHeader::clear()
{
    m_headers.clear();
}


std::string_view JobHttpHeader::value(std::string_view name, std::string_view defaultVal) const
{
    auto it = m_headers.find(normalizeKey(name));
    if (it != m_headers.end())
        return it->second.value;
    return defaultVal;
}

std::string_view JobHttpHeader::value(JobIana::IanaHeaders name, std::string_view defaultVal) const
{
    return value(JobIana::toString(name), defaultVal);
}

std::string_view JobHttpHeader::valueAt(size_t pos) const
{
    if (pos >= m_headers.size())
        return {};
    auto it = m_headers.begin();
    std::advance(it, pos);
    return it->second.value;
}



std::forward_list<std::string_view> JobHttpHeader::values(std::string_view name) const
{
    std::forward_list<std::string_view> ret;
    auto it = m_headers.find(normalizeKey(name));
    if (it != m_headers.end())
        ret.push_front(it->second.value);
    return ret;
}

} // namespace job::net
