#pragma once

#include <cstddef>
#include <functional>
#include <map>
#include <stdexcept>
#include <utility>

#include "job_container_concept.h"

namespace job::core {

template<typename Key, typename T, typename Compare = std::less<Key>>
class JobMap {
public:
    using key_type        = Key;
    using mapped_type     = T;
    using value_type      = typename std::map<Key, T, Compare>::value_type;
    using size_type       = std::size_t;
    using iterator        = typename std::map<Key, T, Compare>::iterator;
    using const_iterator  = typename std::map<Key, T, Compare>::const_iterator;

    JobMap() = default;

    explicit JobMap(const Compare &compare) :
        m_items(compare)
    {
    }

    ~JobMap() = default;

    JobMap(const JobMap &) = default;
    JobMap &operator=(const JobMap &) = default;
    JobMap(JobMap &&) noexcept = default;
    JobMap &operator=(JobMap &&) noexcept = default;


    [[nodiscard]] T &operator[](const Key &key)
    {
        return m_items[key];
    }

    [[nodiscard]] T &operator[](Key &&key)
    {
        return m_items[std::move(key)];
    }

    [[nodiscard]] T operator[](std::size_t idx)
    {
        if (idx >= m_items.size())
            return {};

        auto it = m_items.begin();
        std::advance(it, idx);
        return it->second;
    }

    [[nodiscard]] const T operator[](std::size_t idx) const
    {
        if (idx >= m_items.size())
            return {};

        auto it = m_items.cbegin();
        std::advance(it, idx);
        return it->second;
    }


    [[nodiscard]] size_type size() const noexcept
    {
        return m_items.size();
    }

    [[nodiscard]] size_type count() const noexcept
    {
        return m_items.size();
    }

    [[nodiscard]] bool isEmpty() const noexcept
    {
        return m_items.empty();
    }

    void clear() noexcept
    {
        m_items.clear();
    }

    [[nodiscard]] iterator begin() noexcept
    {
        return m_items.begin();
    }

    [[nodiscard]] iterator end() noexcept
    {
        return m_items.end();
    }

    [[nodiscard]] const_iterator begin() const noexcept
    {
        return m_items.begin();
    }

    [[nodiscard]] const_iterator end() const noexcept
    {
        return m_items.end();
    }

    [[nodiscard]] const_iterator constBegin() const noexcept
    {
        return m_items.cbegin();
    }

    [[nodiscard]] const_iterator constEnd() const noexcept
    {
        return m_items.cend();
    }

    [[nodiscard]] bool contains(const Key &key) const
    {
        return m_items.find(key) != m_items.end();
    }

    [[nodiscard]] iterator find(const Key &key)
    {
        return m_items.find(key);
    }

    [[nodiscard]] const_iterator find(const Key &key) const
    {
        return m_items.find(key);
    }

    [[nodiscard]] T &at(const Key &key)
    {
        return m_items.at(key);
    }

    [[nodiscard]] const T &at(const Key &key) const
    {
        return m_items.at(key);
    }

    [[nodiscard]] T at(std::size_t idx)
    {
        if (idx >= m_items.size())
            return {};

        auto it = m_items.begin();
        std::advance(it, idx);
        return it->second;
    }

    [[nodiscard]] const T at(std::size_t idx) const
    {
        if (idx >= m_items.size())
            return {};

        auto it = m_items.cbegin();
        std::advance(it, idx);
        return it->second;
    }

    void insert(const Key &key, const T &value)
    {
        m_items.insert_or_assign(key, value);
    }

    void insert(const Key &key, T &&value)
    {
        m_items.insert_or_assign(
            key,
            std::move(value)
            );
    }

    void insert(Key &&key, const T &value)
    {
        m_items.insert_or_assign(
            std::move(key),
            value
            );
    }

    void insert(Key &&key, T &&value)
    {
        m_items.insert_or_assign(
            std::move(key),
            std::move(value)
            );
    }

    [[nodiscard]] bool remove(const Key &key)
    {
        return m_items.erase(key) != 0;
    }

    [[nodiscard]] T take(const Key &key)
    {
        const auto it = m_items.find(key);

        if (it == m_items.end())
            throw std::out_of_range("JobMap::take key not found");

        T value = std::move(it->second);
        m_items.erase(it);

        return value;
    }

private:
    std::map<Key, T, Compare> m_items;
};

static_assert(JobContainer<JobMap<int, int>>);

} // namespace job::core