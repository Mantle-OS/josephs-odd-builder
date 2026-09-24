#pragma once

#include <memory>
#include <span>
#include <utility>
#include <vector>

#include "job_rect.h"

namespace job::render {

class JobDamage
{
public:
    using Ptr  = std::shared_ptr<JobDamage>;
    using WPtr = std::weak_ptr<JobDamage>;
    using UPtr = std::unique_ptr<JobDamage>;

    using Rect  = JobRect;
    using Rects = std::vector<Rect>;

    JobDamage() = default;

    explicit JobDamage(Rects rects) :
        m_rects(std::move(rects))
    {
    }

    ~JobDamage() = default;

    JobDamage(const JobDamage &) = default;
    JobDamage &operator=(const JobDamage &) = default;

    JobDamage(JobDamage &&) noexcept = default;
    JobDamage &operator=(JobDamage &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobDamage>();
    }

    [[nodiscard]] static Ptr createShared(Rects rects)
    {
        return std::make_shared<JobDamage>(std::move(rects));
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobDamage>();
    }

    [[nodiscard]] static UPtr createUniq(Rects rects)
    {
        return std::make_unique<JobDamage>(std::move(rects));
    }

    [[nodiscard]] std::span<const Rect> rects() const noexcept
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

    void add(Rect rect)
    {
        m_rects.push_back(rect);
    }

    void clear() noexcept
    {
        m_rects.clear();
    }

    [[nodiscard]] friend bool operator==(const JobDamage &, const JobDamage &) noexcept = default;
private:
    Rects m_rects;
};

}