#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

#include "job_yaml_source_range.h"

namespace job::yaml {

struct YamlAnchorEntry
{
    std::string_view    name{};
    YamlSourceRange     range{};
    std::size_t         indent{};
    bool                complete{};

    [[nodiscard]] constexpr bool isComplete() const noexcept
    {
        return complete;
    }
};

class YamlAnchorTable
{
public:
    using Entry = YamlAnchorEntry;
    [[nodiscard]] bool reserve(std::string_view name)
    {
        if (name.empty())
            return false;

        if (find(name) != nullptr)
            return false;

        m_entries.push_back(Entry{
            .name = name,
            .range = {},
            .indent = 0,
            .complete = false
        });

        return true;
    }

    [[nodiscard]] bool complete(std::string_view name, YamlSourceRange range, std::size_t indent) noexcept
    {
        Entry *entry = findMutable(name);
        if (entry == nullptr || entry->complete)
            return false;

        entry->range = range;
        entry->indent = indent;
        entry->complete = true;
        return true;
    }

    [[nodiscard]] const Entry *find(std::string_view name) const noexcept
    {
        for (const auto &entry : m_entries) {
            if (entry.name == name)
                return &entry;
        }

        return nullptr;
    }

    [[nodiscard]] bool contains(std::string_view name) const noexcept
    {
        return find(name) != nullptr;
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return m_entries.size();
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return m_entries.empty();
    }

    void clear() noexcept
    {
        m_entries.clear();
    }

private:
    [[nodiscard]] Entry *findMutable(std::string_view name) noexcept
    {
        for (auto &entry : m_entries) {
            if (entry.name == name)
                return &entry;
        }

        return nullptr;
    }

    std::vector<Entry>  m_entries;
};

} // namespace job::yaml