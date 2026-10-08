#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include <drm_fourcc.h>

#include "job_render_format.h"

#include "job_drm_simple_device.h"

namespace job::render {

class JobKmsSimpleSurface
{
public:
    using Ptr  = std::shared_ptr<JobKmsSimpleSurface>;
    using WPtr = std::weak_ptr<JobKmsSimpleSurface>;
    using UPtr = std::unique_ptr<JobKmsSimpleSurface>;

    explicit JobKmsSimpleSurface(JobDrmSimpleDevice::Ptr device);
    ~JobKmsSimpleSurface();

    JobKmsSimpleSurface(const JobKmsSimpleSurface &) = delete;
    JobKmsSimpleSurface &operator=(const JobKmsSimpleSurface &) = delete;
    JobKmsSimpleSurface(JobKmsSimpleSurface &&) = delete;
    JobKmsSimpleSurface &operator=(JobKmsSimpleSurface &&) = delete;

    [[nodiscard]] static Ptr createShared(JobDrmSimpleDevice::Ptr device);
    [[nodiscard]] static UPtr createUniq(JobDrmSimpleDevice::Ptr device);

    [[nodiscard]] bool create(std::uint32_t width, std::uint32_t height, core::FormatType format = core::FormatType::XRGB8888);

    void destroy();

    [[nodiscard]] std::uint32_t width() const noexcept;
    [[nodiscard]] std::uint32_t height() const noexcept;

    [[nodiscard]] core::FormatType format() const noexcept;

    [[nodiscard]] std::size_t stride() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

    [[nodiscard]] std::span<std::byte> data() noexcept;
    [[nodiscard]] std::span<const std::byte> data() const noexcept;

    [[nodiscard]] bool valid() const noexcept;

    [[nodiscard]] std::uint32_t handle() const noexcept;
    [[nodiscard]] std::uint32_t framebufferId() const noexcept;

private:
    [[nodiscard]] bool createDumbBuffer();
    [[nodiscard]] bool createFramebuffer();
    [[nodiscard]] bool map();
    void unmap() noexcept;

    JobDrmSimpleDevice::Ptr m_device;

    std::uint32_t   m_width{};
    std::uint32_t   m_height{};

    core::FormatType      m_format{core::FormatType::Unknown};

    std::uint32_t   m_handle{};
    std::uint32_t   m_framebufferId{};

    std::size_t     m_stride{};
    std::size_t     m_size{};

    std::byte       *m_data{nullptr};


    [[nodiscard]] static constexpr std::uint8_t dumbColorMode(core::FormatType format) noexcept
    {

        switch (format) {
        case core::FormatType::Unknown:
            return 0;

        case core::FormatType::XRGB8888:
        case core::FormatType::BGRX8888:
        case core::FormatType::RGBX8888:
        case core::FormatType::XBGR8888:
            return 32;

        case core::FormatType::RGB888:
        case core::FormatType::BGR888:
            return 24;

        case core::FormatType::RGB565:
        case core::FormatType::BGR565:
            return 16;

        case core::FormatType::XRGB1555:
        case core::FormatType::BGRX5551:
        case core::FormatType::RGBX5551:
        case core::FormatType::XBGR1555:
            return 15;

        case core::FormatType::C8:
        case core::FormatType::D8:
        case core::FormatType::R8:
            return 8;

        case core::FormatType::C4:
        case core::FormatType::D4:
        case core::FormatType::R4:
            return 4;

        case core::FormatType::C2:
        case core::FormatType::D2:
        case core::FormatType::R2:
            return 2;

        case core::FormatType::C1:
        case core::FormatType::D1:
        case core::FormatType::R1:
            return 1;

        default:
            return 0;
        }
        return 0;
    }

    [[nodiscard]] static constexpr bool dumbValidFormats(core::FormatType format)
    {
        switch (format) {
        case core::FormatType::Unknown:
            return false;
        case core::FormatType::XRGB8888:
        case core::FormatType::BGRX8888:
        case core::FormatType::RGBX8888:
        case core::FormatType::XBGR8888:
        case core::FormatType::RGB888:
        case core::FormatType::BGR888:
        case core::FormatType::RGB565:
        case core::FormatType::BGR565:
        case core::FormatType::XRGB1555:
        case core::FormatType::BGRX5551:
        case core::FormatType::RGBX5551:
        case core::FormatType::XBGR1555:
        case core::FormatType::C8:
        case core::FormatType::D8:
        case core::FormatType::R8:
        case core::FormatType::C4:
        case core::FormatType::D4:
        case core::FormatType::R4:
        case core::FormatType::C2:
        case core::FormatType::D2:
        case core::FormatType::R2:
        case core::FormatType::C1:
        case core::FormatType::D1:
        case core::FormatType::R1:
            return true;
        default:
            return false;
        }
        return false;
    }


    // Might use in other places.
    [[nodiscard]] static constexpr std::vector<std::uint32_t> dumbCompatibleFormats(core::FormatType format)
    {

        switch (format) {
        case core::FormatType::Unknown:
            return {DRM_FORMAT_INVALID};

        case core::FormatType::XRGB8888:
            return { DRM_FORMAT_BGRX8888, DRM_FORMAT_RGBX8888, DRM_FORMAT_XBGR8888 };

        case core::FormatType::RGB888:
            return {DRM_FORMAT_BGR888};

        case core::FormatType::RGB565:
            return {DRM_FORMAT_BGR565};

        case core::FormatType::XRGB1555:
            return {DRM_FORMAT_BGRX5551, DRM_FORMAT_RGBX5551, DRM_FORMAT_XBGR1555};


        case core::FormatType::C8:
            return { DRM_FORMAT_D8, DRM_FORMAT_R8 };

        case core::FormatType::C4:
            return { DRM_FORMAT_D4, DRM_FORMAT_R4 };

        case core::FormatType::C2:
            return { DRM_FORMAT_D2, DRM_FORMAT_R2 };

        case core::FormatType::C1:
            return { DRM_FORMAT_D1, DRM_FORMAT_R1 };

        default:
            return { DRM_FORMAT_INVALID };
        }
        return { DRM_FORMAT_INVALID };
    }


    [[nodiscard]] static constexpr std::uint32_t dumbFramebufferFormat(core::FormatType format) noexcept
    {
        switch (format) {
        case core::FormatType::Unknown:
            return DRM_FORMAT_INVALID;

        case core::FormatType::XRGB8888:
            return DRM_FORMAT_XRGB8888;
        case core::FormatType::BGRX8888: return DRM_FORMAT_BGRX8888;
        case core::FormatType::RGBX8888: return DRM_FORMAT_RGBX8888;
        case core::FormatType::XBGR8888: return DRM_FORMAT_XBGR8888;

        case core::FormatType::RGB888:
            return DRM_FORMAT_RGB888;
        case core::FormatType::BGR888: return DRM_FORMAT_BGR888;

        case core::FormatType::RGB565:
            return DRM_FORMAT_RGB565;
        case core::FormatType::BGR565: return DRM_FORMAT_BGR565;

        case core::FormatType::XRGB1555:
            return DRM_FORMAT_XRGB1555;
        case core::FormatType::BGRX5551: return DRM_FORMAT_BGRX5551;
        case core::FormatType::RGBX5551: return DRM_FORMAT_RGBX5551;
        case core::FormatType::XBGR1555: return DRM_FORMAT_XBGR1555;

        case core::FormatType::C8:
            return DRM_FORMAT_C8;
        case core::FormatType::D8: return DRM_FORMAT_D8;
        case core::FormatType::R8: return DRM_FORMAT_R8;

        case core::FormatType::C4:
            return DRM_FORMAT_C4;
        case core::FormatType::D4: return DRM_FORMAT_D4;
        case core::FormatType::R4: return DRM_FORMAT_R4;

        case core::FormatType::C2:
            return DRM_FORMAT_C2;
        case core::FormatType::D2: return DRM_FORMAT_D2;
        case core::FormatType::R2: return DRM_FORMAT_R2;

        case core::FormatType::C1:
            return DRM_FORMAT_C1;
        case core::FormatType::D1: return DRM_FORMAT_D1;
        case core::FormatType::R1: return DRM_FORMAT_R1;

        default:
            return DRM_FORMAT_INVALID;
        }

        return DRM_FORMAT_INVALID;
    }

};

} // namespace job::render