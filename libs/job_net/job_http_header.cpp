#include "job_http_header.h"
#include <sstream>

#include <job_logger.h>

namespace job::net {

JobHttpHeader::JobHttpHeader() = default;

JobHttpHeader::JobHttpHeader(std::string_view name, std::string_view value)
{
    if(!append(name, value))
        JOB_LOG_ERROR("Unable to append to headers\n");
}

JobHttpHeader::JobHttpHeader(const JobHttpHeader &other) :
    m_header_list(other.m_header_list),
    m_index(other.m_index)
{
}

JobHttpHeader::~JobHttpHeader() = default;

JobHttpHeader &JobHttpHeader::operator=(JobHttpHeader &&other)
{
    if (this != &other) {
        m_header_list = std::move(other.m_header_list);
        m_index = std::move(other.m_index);
    }
    return *this;
}

JobHttpHeader &JobHttpHeader::operator=(const JobHttpHeader &other)
{
    if (this != &other) {
        m_header_list = other.m_header_list;
        m_index = other.m_index;
    }
    return *this;
}

std::string JobHttpHeader::toString() const
{
    std::ostringstream oss;
    for (const auto &pair : m_header_list) {
        const auto &hv = pair.second;
        oss << hv.displayKey << ": " << hv.value << "\r\n";
    }
    oss << "\r\n";
    return oss.str();
}

bool JobHttpHeader::contains(std::string_view name) const
{
    return m_index.contains(normalizeKey(name));
}

bool JobHttpHeader::contains(JobIana::IanaHeaders name) const
{
    return contains(JobIana::toString(name));
}

bool JobHttpHeader::append(std::string_view name, std::string_view value)
{
    bool ret = false;

    try {
        std::string key = normalizeKey(name);
        auto it = m_index.find(key);

        if (it == m_index.end()) {
            // new header, append to end
            HeaderValue hv{std::string(name), std::string(value)};
            m_header_list.push_back({key, std::move(hv)});
            m_index[key] = m_header_list.size() - 1;
            ret = true;
        } else {
            // existing header, append to its value
            auto &existing = m_header_list[it->second].second.value;
            existing += ", ";
            existing += value;
            ret = true;
        }
    } catch (const std::exception &e) {
        JOB_LOG_ERROR("JobHttpHeader::append() exception: {}", e.what());
        ret = false;
    } catch (...) {
        JOB_LOG_ERROR("JobHttpHeader::append() unknown exception");
        ret = false;
    }

    return ret;
}


bool JobHttpHeader::append(JobIana::IanaHeaders name, std::string_view value)
{
    return append(JobIana::toString(name), value);
}

bool JobHttpHeader::prepend(std::string_view name, std::string_view value)
{
    bool ret = false;

    try {
        std::string key = normalizeKey(name);
        auto it = m_index.find(key);

        if (it == m_index.end()) {
            // insert new header at front
            HeaderValue hv{std::string(name), std::string(value)};
            m_header_list.insert(m_header_list.begin(), {key, std::move(hv)});

            // reindex everything
            for (size_t i = 0; i < m_header_list.size(); ++i)
                m_index[m_header_list[i].first] = i;

            ret = true;
        } else {
            // header exists — prepend to existing value
            auto &existing = m_header_list[it->second].second.value;
            existing.insert(0, std::string(value) + ", ");
            ret = true;
        }
    } catch (const std::exception &e) {
        JOB_LOG_ERROR("JobHttpHeader::prepend() exception: {}", e.what());
        ret = false;
    } catch (...) {
        JOB_LOG_ERROR("JobHttpHeader::prepend() unknown exception");
        ret = false;
    }

    return ret;
}

bool JobHttpHeader::prepend(JobIana::IanaHeaders name, std::string_view value)
{
    return prepend(JobIana::toString(name), value);
}

bool JobHttpHeader::insert(std::string_view name, std::string_view value, uint16_t pos)
{
    bool ret = false;
    try {
        std::string key = normalizeKey(name);
        if (m_index.contains(key)) {
            m_header_list[m_index[key]].second.value = std::string(value);
            ret = true;
        } else {
            HeaderValue hv{std::string(name), std::string(value)};
            if (pos > m_header_list.size())
                pos = m_header_list.size();

            m_header_list.insert(m_header_list.begin() + pos, {key, std::move(hv)});
            m_index[key] = pos;

            for (size_t i = pos + 1; i < m_header_list.size(); ++i)
                m_index[m_header_list[i].first] = i;

            ret = true;
        }
    } catch (...) {
        JOB_LOG_ERROR("JobHttpHeader::insert() exception");
        ret = false;
    }
    return ret;
}

bool JobHttpHeader::insert(JobIana::IanaHeaders name, std::string_view value, uint16_t pos)
{
    return insert(JobIana::toString(name), value, pos);
}

bool JobHttpHeader::replace(size_t pos, std::string_view name, std::string_view val)
{
    bool ret = false;

    try {
        // Validate position first
        if (pos >= m_header_list.size()) {
            JOB_LOG_WARN("JobHttpHeader::replace() invalid position: {}", pos);
            return false;
        }

        std::string key = normalizeKey(name);
        auto &entry = m_header_list[pos];

        // Update existing header if the key matches
        if (entry.first == key) {
            entry.second.displayKey = std::string(name);
            entry.second.value = std::string(val);
            ret = true;
        } else {
            // Key mismatch: overwrite key + value and update index mapping
            m_index.erase(entry.first);
            entry.first = key;
            entry.second.displayKey = std::string(name);
            entry.second.value = std::string(val);
            m_index[key] = pos;
            ret = true;
        }
    } catch (const std::exception &e) {
        JOB_LOG_ERROR("JobHttpHeader::replace() exception: {}", e.what());
        ret = false;
    } catch (...) {
        JOB_LOG_ERROR("JobHttpHeader::replace() unknown exception");
        ret = false;
    }

    return ret;
}

bool JobHttpHeader::replace(size_t pos, JobIana::IanaHeaders name, std::string_view val)
{
    return replace(pos, JobIana::toString(name), val);
}

void JobHttpHeader::removeAt(uint16_t pos)
{
    if (pos >= m_header_list.size()) {
        JOB_LOG_WARN("JobHttpHeader::removeAt() invalid position: {}", pos);
        return;
    }

    const auto key = m_header_list[pos].first;
    m_header_list.erase(m_header_list.begin() + pos);
    m_index.erase(key);

    // Reindex everything after the removed position
    for (size_t i = pos; i < m_header_list.size(); ++i)
        m_index[m_header_list[i].first] = i;
}


void JobHttpHeader::removeAll(std::string_view name)
{
    std::string key = normalizeKey(name);
    auto it = m_index.find(key);
    if (it == m_index.end())
        return;

    size_t pos = it->second;
    removeAt(static_cast<uint16_t>(pos));
}

void JobHttpHeader::removeAll(JobIana::IanaHeaders name)
{
    removeAll(JobIana::toString(name));
}

// update
bool JobHttpHeader::isEmpty() const noexcept
{
    return m_header_list.empty();
}

// Update
size_t JobHttpHeader::size() const noexcept
{
    return m_header_list.size();
}

// Update
size_t JobHttpHeader::count() const noexcept
{
    return m_header_list.size();
}

void JobHttpHeader::clear()
{
    m_header_list.clear();
    m_index.clear();
}


std::string_view JobHttpHeader::value(std::string_view name, std::string_view defaultVal) const
{
    std::string key = normalizeKey(name);
    auto it = m_index.find(key);
    if (it == m_index.end())
        return defaultVal;
    return m_header_list[it->second].second.value;
}

std::string_view JobHttpHeader::value(JobIana::IanaHeaders name, std::string_view defaultVal) const
{
    return value(JobIana::toString(name), defaultVal);
}

std::string_view JobHttpHeader::valueAt(size_t pos) const
{
    if (pos >= m_header_list.size())
        return {};
    return m_header_list[pos].second.value;
}

std::forward_list<std::string_view> JobHttpHeader::values(std::string_view name) const
{
    std::forward_list<std::string_view> ret;
    std::string key = normalizeKey(name);

    for (const auto &pair : m_header_list) {
        if (pair.first == key)
            ret.push_front(pair.second.value);
    }
    return ret;
}


} // namespace job::net
