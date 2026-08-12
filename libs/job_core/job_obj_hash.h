#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <iterator>

#include "job_obj_hash_concept.h"
#include "job_siphash.h"


namespace job::core {

template<typename T>
struct JobObjectPointerTraits;

template<typename T>
struct JobObjectPointerTraits<T *>
{
    using object_type = T;
    using pointer_type = T *;

    [[nodiscard]] static T *get(T *ptr) noexcept
    {
        return ptr;
    }
};

template<typename T>
struct JobObjectPointerTraits<std::shared_ptr<T>>
{
    using object_type = T;
    using pointer_type = std::shared_ptr<T>;

    [[nodiscard]] static T *get(const std::shared_ptr<T> &ptr) noexcept
    {
        return ptr.get();
    }
};

template<typename T>
struct JobObjectPointerTraits<std::unique_ptr<T>>
{
    using object_type = T;
    using pointer_type = std::unique_ptr<T>;

    [[nodiscard]] static T *get(const std::unique_ptr<T> &ptr) noexcept
    {
        return ptr.get();
    }
};

template<JobObjectPointer Ptr, typename Hash = JobSipHash>
class JobObjHash
{
public:
    using pointer_type = Ptr;
    using object_type = typename JobObjectPointerTraits<Ptr>::object_type;
    using key_type = std::string;
    using mapped_type = Ptr;
    using hash_type = Hash;
    using storage_type = std::unordered_map<std::string, Ptr, Hash>;
    using value_type = typename storage_type::value_type;
    using size_type = std::size_t;
    using iterator = typename storage_type::iterator;
    using const_iterator = typename storage_type::const_iterator;

    static_assert(JobObject<object_type>, "JobObjHash pointer must reference a JobObject");

    JobObjHash()
        : m_items(0, defaultHash())
    {
    }

    explicit JobObjHash(const Hash &hash)
        : m_items(0, hash)
    {
    }

    ~JobObjHash()
    {
        clear();
    }

    JobObjHash(const JobObjHash &) = delete;
    JobObjHash &operator=(const JobObjHash &) = delete;

    JobObjHash(JobObjHash &&) noexcept = default;
    JobObjHash &operator=(JobObjHash &&other) noexcept
    {
        if (this == &other)
            return *this;

        clear();
        m_items = std::move(other.m_items);

        return *this;
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

    [[nodiscard]] bool contains(const std::string &uid) const
    {
        return m_items.find(uid) != m_items.end();
    }

    [[nodiscard]] iterator find(const std::string &uid)
    {
        return m_items.find(uid);
    }

    [[nodiscard]] const_iterator find(const std::string &uid) const
    {
        return m_items.find(uid);
    }

    [[nodiscard]] object_type *at(const std::string &uid)
    {
        return object(m_items.at(uid));
    }

    [[nodiscard]] object_type *at(int idx)
    {
        if (idx < 0 || static_cast<size_type>(idx) >= count())
            return nullptr;

        auto it = m_items.begin();
        std::advance(it, idx);
        return object(it->second);
    }

    [[nodiscard]] const object_type *at(int idx) const
    {
        if (idx < 0 || static_cast<size_type>(idx) >= count())
            return nullptr;

        auto it = m_items.cbegin();
        std::advance(it, idx);
        return object(it->second);
    }

    [[nodiscard]] const object_type *at(const std::string &uid) const
    {
        return object(m_items.at(uid));
    }

    void insert(Ptr value)
    {
        object_type *ptr = object(value);

        if (!ptr) {
            destroy(value);
            throw std::invalid_argument("JobObjHash cannot insert a null object");
        }

        const std::string uid = std::string(ptr->uid());

        if (uid.empty()) {
            destroy(value);
            throw std::invalid_argument("JobObjHash cannot insert an object with an empty uid");
        }

        const auto [it, inserted] = m_items.try_emplace(uid, std::move(value));

        if (!inserted) {
            if constexpr (std::is_pointer_v<Ptr>) {
                destroy(value);
            }
            throw std::invalid_argument("JobObjHash cannot insert a duplicate uid");
        }
    }

    [[nodiscard]] Ptr take(const std::string &uid)
    {
        const auto it = m_items.find(uid);

        if (it == m_items.end())
            throw std::out_of_range("JobObjHash::take uid not found");

        Ptr value = std::move(it->second);
        m_items.erase(it);

        return value;
    }

    [[nodiscard]] bool remove(const std::string &uid)
    {
        const auto it = m_items.find(uid);

        if (it == m_items.end())
            return false;

        destroy(it->second);
        m_items.erase(it);

        return true;
    }

    void clear() noexcept
    {
        if constexpr (std::is_pointer_v<Ptr>) {
            for (auto &[uid, value] : m_items)
                delete value;
        }

        m_items.clear();
    }

    void reserve(size_type size)
    {
        m_items.reserve(size);
    }

private:
    [[nodiscard]] static object_type *object(Ptr &value) noexcept
    {
        return JobObjectPointerTraits<Ptr>::get(value);
    }

    [[nodiscard]] static const object_type *object(const Ptr &value) noexcept
    {
        return JobObjectPointerTraits<Ptr>::get(value);
    }

    static void destroy(Ptr &value) noexcept
    {
        if constexpr (std::is_pointer_v<Ptr>) {
            delete value;
            value = nullptr;
        }
    }

    [[nodiscard]] static Hash defaultHash()
    {
        if constexpr (std::is_same_v<Hash, JobSipHash>) {
            Hash hash{0, 0};

            if (!hash.seed())
                throw std::runtime_error("JobObjHash: failed to seed SipHash from OS RNG");

            return hash;
        } else {
            return Hash{};
        }
    }


    storage_type m_items;
};

template<JobObjectPointer Ptr>
using JobObjHashFast = JobObjHash<Ptr, std::hash<std::string>>;

} // namespace job::core