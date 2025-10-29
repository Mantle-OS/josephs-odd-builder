#pragma once

#include <string_view>
#include <unordered_map>
#include <algorithm>
#include <forward_list>
#include <vector>

#include "job_iana.h"

namespace job::net {

class JobHttpHeader {
public:
    JobHttpHeader();
    JobHttpHeader(std::string_view name, std::string_view value);
    JobHttpHeader(const JobHttpHeader &other);
    ~JobHttpHeader();
    static std::string normalizeKey(const std::string_view &input)
    {
        std::string key(input);
        std::transform(key.begin(), key.end(), key.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return key;
    }

    [[nodiscard]] std::string toString() const;

    [[nodiscard]] bool contains(std::string_view name) const;
    [[nodiscard]] bool contains(JobIana::IanaHeaders name) const;

    [[nodiscard]] bool append(std::string_view  name, std::string_view value);
    [[nodiscard]] bool append(JobIana::IanaHeaders name, std::string_view value);

    [[nodiscard]] bool prepend(std::string_view  name, std::string_view value);
    [[nodiscard]] bool prepend(JobIana::IanaHeaders name, std::string_view value);

    [[nodiscard]] bool insert(std::string_view name, std::string_view value, uint16_t pos);
    [[nodiscard]] bool insert(JobIana::IanaHeaders name, std::string_view value, uint16_t pos);

    [[nodiscard]] bool replace(size_t pos, std::string_view name, std::string_view val);
    [[nodiscard]] bool replace(size_t pos, JobIana::IanaHeaders name, std::string_view val);

    void removeAt(uint16_t pos);
    void removeAll(std::string_view name);
    void removeAll(JobIana::IanaHeaders name);

    [[nodiscard]] bool isEmpty() const noexcept;
    [[nodiscard]] size_t size()  const noexcept;
    [[nodiscard]] size_t count() const noexcept;

    void clear();

    [[nodiscard]] std::string_view value(std::string_view name, std::string_view defaultVal = {}) const;
    [[nodiscard]] std::string_view value(JobIana::IanaHeaders name, std::string_view defaultVal = {}) const;
    [[nodiscard]] std::string_view valueAt(size_t pos) const;
    [[nodiscard]] std::forward_list<std::string_view> values(std::string_view name) const;

    JobHttpHeader & operator=(JobHttpHeader &&other);
    JobHttpHeader & operator=(const JobHttpHeader &other);

    [[nodiscard]] auto begin() noexcept
    {
        return m_header_list.begin();
    }
    [[nodiscard]] auto end() noexcept
    {
        return m_header_list.end();
    }
    [[nodiscard]] auto begin() const noexcept
    {
        return m_header_list.begin();
    }
    [[nodiscard]] auto end() const noexcept
    {
        return m_header_list.end();
    }
    [[nodiscard]] auto cbegin() const noexcept
    {
        return m_header_list.cbegin();
    }
    [[nodiscard]] auto cend() const noexcept
    {
        return m_header_list.cend();
    }

private:
    struct HeaderValue {
        std::string displayKey;
        std::string value;
    };

    std::vector<std::pair<std::string, HeaderValue>> m_header_list;
    std::unordered_map<std::string, size_t> m_index;
};
} // job::net
