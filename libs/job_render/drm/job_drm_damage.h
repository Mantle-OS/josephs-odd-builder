#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include "job_drm_damage_rect.h"

namespace job::render {

class JobDrmDamage
{
public:
    using Ptr  = std::shared_ptr<JobDrmDamage>;
    using WPtr = std::weak_ptr<JobDrmDamage>;
    using UPtr = std::unique_ptr<JobDrmDamage>;

    using Rects = std::vector<JobDrmDamageRect>;

    JobDrmDamage() = default;

    explicit JobDrmDamage(Rects rects) noexcept :
        m_rects(std::move(rects))
    {
    }

    ~JobDrmDamage() = default;

    JobDrmDamage(const JobDrmDamage &) = default;
    JobDrmDamage &operator=(const JobDrmDamage &) = default;

    JobDrmDamage(JobDrmDamage &&) noexcept = default;
    JobDrmDamage &operator=(JobDrmDamage &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobDrmDamage>();
    }

    [[nodiscard]] static Ptr createShared(Rects rects)
    {
        return std::make_shared<JobDrmDamage>(std::move(rects));
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobDrmDamage>();
    }

    [[nodiscard]] static UPtr createUniq(Rects rects)
    {
        return std::make_unique<JobDrmDamage>(std::move(rects));
    }

    [[nodiscard]] std::span<const JobDrmDamageRect> rects() const noexcept
    {
        return m_rects;
    }

    [[nodiscard]] std::size_t count() const noexcept
    {
        return m_rects.size();
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return m_rects.empty();
    }

    [[nodiscard]] bool valid() const noexcept
    {
        for (const auto &rect : m_rects) {
            if (!rect.valid())
                return false;
        }

        return true;
    }

    void add(JobDrmDamageRect rect)
    {
        m_rects.push_back(rect);
    }

    void clear() noexcept
    {
        m_rects.clear();
    }

    [[nodiscard]] friend bool operator==(const JobDrmDamage &, const JobDrmDamage &) = default;

private:
    Rects m_rects;
};

}