#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <xf86drmMode.h>

namespace job::render {

class JobDrmPlaneResources
{
public:
    using Ptr  = std::shared_ptr<JobDrmPlaneResources>;
    using WPtr = std::weak_ptr<JobDrmPlaneResources>;
    using UPtr = std::unique_ptr<JobDrmPlaneResources>;

    using Id        = std::uint32_t;
    using PlaneIds  = std::vector<Id>;

    JobDrmPlaneResources() = default;
    ~JobDrmPlaneResources() = default;

    JobDrmPlaneResources(const JobDrmPlaneResources &) = default;
    JobDrmPlaneResources &operator=(const JobDrmPlaneResources &) = default;

    JobDrmPlaneResources(JobDrmPlaneResources &&) noexcept = default;
    JobDrmPlaneResources &operator=(JobDrmPlaneResources &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobDrmPlaneResources>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobDrmPlaneResources>();
    }

    [[nodiscard]] static JobDrmPlaneResources fromDrm(const drmModePlaneRes &resources)
    {
        JobDrmPlaneResources result;

        if (resources.count_planes == 0 || resources.planes == nullptr)
            return result;

        result.m_planeIds.reserve(resources.count_planes);

        for (std::uint32_t i = 0; i < resources.count_planes; ++i)
            result.m_planeIds.push_back(resources.planes[i]);

        return result;
    }

    [[nodiscard]] const PlaneIds &planeIds() const noexcept
    {
        return m_planeIds;
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return m_planeIds.size();
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return m_planeIds.empty();
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return !m_planeIds.empty();
    }

private:
    PlaneIds m_planeIds;
};

}