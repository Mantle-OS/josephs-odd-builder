#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace job::render {

class JobDrmPropertyBlob
{
public:
    using Ptr  = std::shared_ptr<JobDrmPropertyBlob>;
    using WPtr = std::weak_ptr<JobDrmPropertyBlob>;
    using UPtr = std::unique_ptr<JobDrmPropertyBlob>;

    using Id    = std::uint32_t;
    using Data  = std::vector<std::byte>;

    JobDrmPropertyBlob() = default;

    JobDrmPropertyBlob(Id id, Data data) :
        m_id(id),
        m_data(std::move(data))
    {
    }

    ~JobDrmPropertyBlob() = default;

    JobDrmPropertyBlob(const JobDrmPropertyBlob &) = default;
    JobDrmPropertyBlob &operator=(const JobDrmPropertyBlob &) = default;

    JobDrmPropertyBlob(JobDrmPropertyBlob &&) noexcept = default;
    JobDrmPropertyBlob &operator=(JobDrmPropertyBlob &&) noexcept = default;

    [[nodiscard]] static Ptr createShared(Id id, Data data)
    {
        return std::make_shared<JobDrmPropertyBlob>(id, std::move(data));
    }

    [[nodiscard]] static UPtr createUniq(Id id, Data data)
    {
        return std::make_unique<JobDrmPropertyBlob>(id, std::move(data));
    }

    [[nodiscard]] constexpr Id id() const noexcept
    {
        return m_id;
    }

    [[nodiscard]] std::span<const std::byte> data() const noexcept
    {
        return m_data;
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return m_data.size();
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return m_data.empty();
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_id != 0;
    }

    template <typename T>
    [[nodiscard]] const T *as() const noexcept
    {
        if (m_data.size() < sizeof(T))
            return nullptr;

        return reinterpret_cast<const T *>(m_data.data());
    }

    [[nodiscard]] friend bool operator==(const JobDrmPropertyBlob &, const JobDrmPropertyBlob &) = default;

private:
    Id      m_id{};
    Data    m_data;
};

}