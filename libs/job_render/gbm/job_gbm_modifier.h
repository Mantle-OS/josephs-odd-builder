#pragma once

#include <cstdint>
#include <memory>

#include "job_drm_modifier.h"

namespace job::render {

class JobGbmModifier
{
public:
    using Ptr  = std::shared_ptr<JobGbmModifier>;
    using WPtr = std::weak_ptr<JobGbmModifier>;
    using UPtr = std::unique_ptr<JobGbmModifier>;

    using Value = JobDrmModifier::Value;

    constexpr JobGbmModifier() = default;

    explicit constexpr JobGbmModifier(Value value) noexcept :
        m_modifier(value)
    {
    }

    explicit constexpr JobGbmModifier(JobDrmModifier modifier) noexcept :
        m_modifier(modifier)
    {
    }

    ~JobGbmModifier() = default;

    JobGbmModifier(const JobGbmModifier &) = default;
    JobGbmModifier &operator=(const JobGbmModifier &) = default;

    JobGbmModifier(JobGbmModifier &&) noexcept = default;
    JobGbmModifier &operator=(JobGbmModifier &&) noexcept = default;

    [[nodiscard]] static Ptr createShared(Value value)
    {
        return std::make_shared<JobGbmModifier>(value);
    }

    [[nodiscard]] static Ptr createShared(JobDrmModifier modifier)
    {
        return std::make_shared<JobGbmModifier>(modifier);
    }

    [[nodiscard]] static UPtr createUniq(Value value)
    {
        return std::make_unique<JobGbmModifier>(value);
    }

    [[nodiscard]] static UPtr createUniq(JobDrmModifier modifier)
    {
        return std::make_unique<JobGbmModifier>(modifier);
    }

    [[nodiscard]] constexpr Value value() const noexcept
    {
        return m_modifier.value();
    }

    [[nodiscard]] constexpr const JobDrmModifier &drmModifier() const noexcept
    {
        return m_modifier;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_modifier.valid();
    }

    [[nodiscard]] constexpr bool linear() const noexcept
    {
        return m_modifier.linear();
    }

    [[nodiscard]] constexpr DrmModifierVendor vendor() const noexcept
    {
        return m_modifier.vendor();
    }

    [[nodiscard]] constexpr bool vendor(DrmModifierVendor vendor) const noexcept
    {
        return m_modifier.vendor(vendor);
    }

    [[nodiscard]] static constexpr JobGbmModifier invalid() noexcept
    {
        return JobGbmModifier{JobDrmModifier::invalid()};
    }

    [[nodiscard]] static constexpr JobGbmModifier linearModifier() noexcept
    {
        return JobGbmModifier{JobDrmModifier::linearModifier()};
    }

    [[nodiscard]] friend constexpr bool operator==(const JobGbmModifier &, const JobGbmModifier &) noexcept = default;

private:
    JobDrmModifier m_modifier;
};

}