#pragma once

#include <cstddef>
#include <utility>
#include <algorithm>
#include <vector>
#include <stdexcept>

#include "job_container_concept.h"

namespace job::core {

template<typename T>
class JobList {
public:
    using value_type      = T;
    using size_type       = std::size_t;
    using iterator        = typename std::vector<T>::iterator;
    using const_iterator  = typename std::vector<T>::const_iterator;

    JobList() = default;
    ~JobList() = default;

    JobList(const JobList &) = default;
    JobList &operator=(const JobList &) = default;
    JobList(JobList &&) noexcept = default;
    JobList &operator=(JobList &&) noexcept = default;

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

    [[nodiscard]] size_type capacity() const noexcept
    {
        return m_items.capacity();
    }

    void reserve(size_type size)
    {
        m_items.reserve(size);
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

    [[nodiscard]] T &at(size_type index)
    {
        return m_items.at(index);
    }

    [[nodiscard]] const T &at(size_type index) const
    {
        return m_items.at(index);
    }

    [[nodiscard]] T &operator[](size_type index)
    {
        return m_items[index];
    }

    [[nodiscard]] const T &operator[](size_type index) const
    {
        return m_items[index];
    }

    [[nodiscard]] T &first()
    {
        return m_items.front();
    }

    [[nodiscard]] const T &first() const
    {
        return m_items.front();
    }

    [[nodiscard]] T &last()
    {
        return m_items.back();
    }

    [[nodiscard]] const T &last() const
    {
        return m_items.back();
    }

    void append(const T &value)
    {
        m_items.push_back(value);
    }

    void append(T &&value)
    {
        m_items.push_back(std::move(value));
    }

    void prepend(const T &value)
    {
        m_items.insert(m_items.begin(), value);
    }

    void prepend(T &&value)
    {
        m_items.insert(m_items.begin(), std::move(value));
    }

    void insert(size_type index, const T &value)
    {
        if (index > m_items.size())
            throw std::out_of_range("JobList::insert index out of range");

        m_items.insert(
            m_items.begin() +
                static_cast<typename std::vector<T>::difference_type>(index),
            value
            );
    }

    void insert(size_type index, T &&value)
    {
        if (index > m_items.size())
            throw std::out_of_range("JobList::insert index out of range");

        m_items.insert(
            m_items.begin() +
                static_cast<typename std::vector<T>::difference_type>(index),
            std::move(value)
            );
    }

    [[nodiscard]] bool contains(const T &value) const
    {
        return std::find(m_items.begin(), m_items.end(), value) != m_items.end();
    }

    [[nodiscard]] size_type indexOf(const T &value) const
    {
        const auto it = std::find(m_items.begin(), m_items.end(), value);

        if (it == m_items.end())
            return size();

        return static_cast<size_type>(
            std::distance(m_items.begin(), it)
            );
    }

    [[nodiscard]] T takeAt(size_type index)
    {
        if (index >= m_items.size())
            throw std::out_of_range("JobList::takeAt index out of range");

        auto it = m_items.begin() +
                  static_cast<typename std::vector<T>::difference_type>(index);

        T value = std::move(*it);
        m_items.erase(it);

        return value;
    }

    [[nodiscard]] T takeFirst()
    {
        if (m_items.empty())
            throw std::out_of_range("JobList::takeFirst called on empty list");

        return takeAt(0);
    }

    [[nodiscard]] T takeLast()
    {
        if (m_items.empty())
            throw std::out_of_range("JobList::takeLast called on empty list");

        T value = std::move(m_items.back());
        m_items.pop_back();

        return value;
    }

    [[nodiscard]] bool removeFirst(const T &value)
    {
        const auto it = std::find(m_items.begin(), m_items.end(), value);

        if (it == m_items.end())
            return false;

        m_items.erase(it);
        return true;
    }

    [[nodiscard]] bool removeLast(const T &value)
    {
        const auto it = std::find(m_items.rbegin(), m_items.rend(), value);

        if (it == m_items.rend())
            return false;

        m_items.erase(std::next(it).base());
        return true;
    }

    [[nodiscard]] size_type removeAll(const T &value)
    {
        const size_type oldSize = m_items.size();

        const auto it = std::remove(
            m_items.begin(),
            m_items.end(),
            value
            );

        m_items.erase(it, m_items.end());

        return oldSize - m_items.size();
    }
    void removeAt(size_type index)
    {
        if (index >= m_items.size())
            throw std::out_of_range("JobList::removeAt index out of range");

        m_items.erase(m_items.begin() + static_cast<typename std::vector<T>::difference_type>(index));
    }

private:
    std::vector<T> m_items;
};

static_assert(JobContainer<JobList<int>>);

} // namespace job::core