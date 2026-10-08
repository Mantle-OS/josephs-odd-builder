#pragma once

#include <cstdint>

#include <drm_fourcc.h>

#include <job_render_format_type.h>

namespace job::render {

class JobDrmFormat
{
public:
    using Value = std::uint32_t;

    constexpr JobDrmFormat() = default;

    explicit constexpr JobDrmFormat(Value value) noexcept :
        m_value(value)
    {

    }

    ~JobDrmFormat() = default;

    JobDrmFormat(const JobDrmFormat &) = default;
    JobDrmFormat &operator=(const JobDrmFormat &) = default;

    JobDrmFormat(JobDrmFormat &&) noexcept = default;
    JobDrmFormat &operator=(JobDrmFormat &&) noexcept = default;

    [[nodiscard]] constexpr Value value() const noexcept
    {
        return m_value;
    }

    [[nodiscard]] constexpr Value baseValue() const noexcept
    {
        return m_value & ~DRM_FORMAT_BIG_ENDIAN;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return baseValue() != DRM_FORMAT_INVALID;
    }

    [[nodiscard]] constexpr bool bigEndian() const noexcept
    {
        return (m_value & DRM_FORMAT_BIG_ENDIAN) != 0;
    }


    [[nodiscard]] constexpr core::FormatType renderFormat() const noexcept
    {
        switch (baseValue()) {
        case DRM_FORMAT_INVALID:
            return core::FormatType::Unknown;


        case DRM_FORMAT_C1:
            return core::FormatType::C1;
        case DRM_FORMAT_C2:
            return core::FormatType::C2;
        case DRM_FORMAT_C4:
            return core::FormatType::C4;
        case DRM_FORMAT_C8:
            return core::FormatType::C8;

        case DRM_FORMAT_D1:
            return core::FormatType::D1;
        case DRM_FORMAT_D2:
            return core::FormatType::D2;
        case DRM_FORMAT_D4:
            return core::FormatType::D4;
        case DRM_FORMAT_D8:
            return core::FormatType::D8;

        case DRM_FORMAT_R1:
            return core::FormatType::R1;
        case DRM_FORMAT_R2:
            return core::FormatType::R2;
        case DRM_FORMAT_R4:
            return core::FormatType::R4;
        case DRM_FORMAT_R8:
            return core::FormatType::R8;
        case DRM_FORMAT_R10:
            return core::FormatType::R10;
        case DRM_FORMAT_R12:
            return core::FormatType::R12;
        case DRM_FORMAT_R16:
            return core::FormatType::R16;

        case DRM_FORMAT_RG88:
            return core::FormatType::RG88;
        case DRM_FORMAT_GR88:
            return core::FormatType::GR88;
        case DRM_FORMAT_RG1616:
            return core::FormatType::RG1616;
        case DRM_FORMAT_GR1616:
            return core::FormatType::GR1616;

        case DRM_FORMAT_RGB332:
            return core::FormatType::RGB332;
        case DRM_FORMAT_BGR233:
            return core::FormatType::BGR233;

        case DRM_FORMAT_XRGB4444:
            return core::FormatType::XRGB4444;
        case DRM_FORMAT_XBGR4444:
            return core::FormatType::XBGR4444;
        case DRM_FORMAT_RGBX4444:
            return core::FormatType::RGBX4444;
        case DRM_FORMAT_BGRX4444:
            return core::FormatType::BGRX4444;
        case DRM_FORMAT_ARGB4444:
            return core::FormatType::ARGB4444;
        case DRM_FORMAT_ABGR4444:
            return core::FormatType::ABGR4444;
        case DRM_FORMAT_RGBA4444:
            return core::FormatType::RGBA4444;
        case DRM_FORMAT_BGRA4444:
            return core::FormatType::BGRA4444;

        case DRM_FORMAT_XRGB1555:
            return core::FormatType::XRGB1555;
        case DRM_FORMAT_XBGR1555:
            return core::FormatType::XBGR1555;
        case DRM_FORMAT_RGBX5551:
            return core::FormatType::RGBX5551;
        case DRM_FORMAT_BGRX5551:
            return core::FormatType::BGRX5551;
        case DRM_FORMAT_ARGB1555:
            return core::FormatType::ARGB1555;
        case DRM_FORMAT_ABGR1555:
            return core::FormatType::ABGR1555;
        case DRM_FORMAT_RGBA5551:
            return core::FormatType::RGBA5551;
        case DRM_FORMAT_BGRA5551:
            return core::FormatType::BGRA5551;

        case DRM_FORMAT_RGB565:
            return core::FormatType::RGB565;
        case DRM_FORMAT_BGR565:
            return core::FormatType::BGR565;

        case DRM_FORMAT_RGB888:
            return core::FormatType::RGB888;
        case DRM_FORMAT_BGR888:
            return core::FormatType::BGR888;

        case DRM_FORMAT_XRGB8888:
            return core::FormatType::XRGB8888;
        case DRM_FORMAT_XBGR8888:
            return core::FormatType::XBGR8888;
        case DRM_FORMAT_RGBX8888:
            return core::FormatType::RGBX8888;
        case DRM_FORMAT_BGRX8888:
            return core::FormatType::BGRX8888;
        case DRM_FORMAT_ARGB8888:
            return core::FormatType::ARGB8888;
        case DRM_FORMAT_ABGR8888:
            return core::FormatType::ABGR8888;
        case DRM_FORMAT_RGBA8888:
            return core::FormatType::RGBA8888;
        case DRM_FORMAT_BGRA8888:
            return core::FormatType::BGRA8888;

        case DRM_FORMAT_XRGB2101010:
            return core::FormatType::XRGB2101010;
        case DRM_FORMAT_XBGR2101010:
            return core::FormatType::XBGR2101010;
        case DRM_FORMAT_RGBX1010102:
            return core::FormatType::RGBX1010102;
        case DRM_FORMAT_BGRX1010102:
            return core::FormatType::BGRX1010102;
        case DRM_FORMAT_ARGB2101010:
            return core::FormatType::ARGB2101010;
        case DRM_FORMAT_ABGR2101010:
            return core::FormatType::ABGR2101010;
        case DRM_FORMAT_RGBA1010102:
            return core::FormatType::RGBA1010102;
        case DRM_FORMAT_BGRA1010102:
            return core::FormatType::BGRA1010102;

        case DRM_FORMAT_RGB161616:
            return core::FormatType::RGB161616;
        case DRM_FORMAT_BGR161616:
            return core::FormatType::BGR161616;

        case DRM_FORMAT_XRGB16161616:
            return core::FormatType::XRGB16161616;
        case DRM_FORMAT_XBGR16161616:
            return core::FormatType::XBGR16161616;
        case DRM_FORMAT_ARGB16161616:
            return core::FormatType::ARGB16161616;
        case DRM_FORMAT_ABGR16161616:
            return core::FormatType::ABGR16161616;

        case DRM_FORMAT_XRGB16161616F:
            return core::FormatType::XRGB16161616F;
        case DRM_FORMAT_XBGR16161616F:
            return core::FormatType::XBGR16161616F;
        case DRM_FORMAT_ARGB16161616F:
            return core::FormatType::ARGB16161616F;
        case DRM_FORMAT_ABGR16161616F:
            return core::FormatType::ABGR16161616F;

        case DRM_FORMAT_R16F:
            return core::FormatType::R16F;
        case DRM_FORMAT_GR1616F:
            return core::FormatType::GR1616F;
        case DRM_FORMAT_BGR161616F:
            return core::FormatType::BGR161616F;

        case DRM_FORMAT_R32F:
            return core::FormatType::R32F;
        case DRM_FORMAT_GR3232F:
            return core::FormatType::GR3232F;
        case DRM_FORMAT_BGR323232F:
            return core::FormatType::BGR323232F;
        case DRM_FORMAT_ABGR32323232F:
            return core::FormatType::ABGR32323232F;

        case DRM_FORMAT_AXBXGXRX106106106106:
            return core::FormatType::AXBXGXRX106106106106;

        case DRM_FORMAT_YUYV:
            return core::FormatType::YUYV;
        case DRM_FORMAT_YVYU:
            return core::FormatType::YVYU;
        case DRM_FORMAT_UYVY:
            return core::FormatType::UYVY;
        case DRM_FORMAT_VYUY:
            return core::FormatType::VYUY;

        case DRM_FORMAT_AYUV:
            return core::FormatType::AYUV;
        case DRM_FORMAT_AVUY8888:
            return core::FormatType::AVUY8888;
        case DRM_FORMAT_XYUV8888:
            return core::FormatType::XYUV8888;
        case DRM_FORMAT_XVUY8888:
            return core::FormatType::XVUY8888;
        case DRM_FORMAT_VUY888:
            return core::FormatType::VUY888;
        case DRM_FORMAT_VUY101010:
            return core::FormatType::VUY101010;
        case DRM_FORMAT_XVUY2101010:
            return core::FormatType::XVUY2101010;

        case DRM_FORMAT_Y210:
            return core::FormatType::Y210;
        case DRM_FORMAT_Y212:
            return core::FormatType::Y212;
        case DRM_FORMAT_Y216:
            return core::FormatType::Y216;

        case DRM_FORMAT_Y410:
            return core::FormatType::Y410;
        case DRM_FORMAT_Y412:
            return core::FormatType::Y412;
        case DRM_FORMAT_Y416:
            return core::FormatType::Y416;

        case DRM_FORMAT_XVYU2101010:
            return core::FormatType::XVYU2101010;
        case DRM_FORMAT_XVYU12_16161616:
            return core::FormatType::XVYU12_16161616;
        case DRM_FORMAT_XVYU16161616:
            return core::FormatType::XVYU16161616;

        case DRM_FORMAT_Y0L0:
            return core::FormatType::Y0L0;
        case DRM_FORMAT_X0L0:
            return core::FormatType::X0L0;
        case DRM_FORMAT_Y0L2:
            return core::FormatType::Y0L2;
        case DRM_FORMAT_X0L2:
            return core::FormatType::X0L2;

        case DRM_FORMAT_YUV420_8BIT:
            return core::FormatType::YUV420_8BIT;
        case DRM_FORMAT_YUV420_10BIT:
            return core::FormatType::YUV420_10BIT;

        case DRM_FORMAT_XRGB8888_A8:
            return core::FormatType::XRGB8888_A8;
        case DRM_FORMAT_XBGR8888_A8:
            return core::FormatType::XBGR8888_A8;
        case DRM_FORMAT_RGBX8888_A8:
            return core::FormatType::RGBX8888_A8;
        case DRM_FORMAT_BGRX8888_A8:
            return core::FormatType::BGRX8888_A8;
        case DRM_FORMAT_RGB888_A8:
            return core::FormatType::RGB888_A8;
        case DRM_FORMAT_BGR888_A8:
            return core::FormatType::BGR888_A8;
        case DRM_FORMAT_RGB565_A8:
            return core::FormatType::RGB565_A8;
        case DRM_FORMAT_BGR565_A8:
            return core::FormatType::BGR565_A8;

        case DRM_FORMAT_NV12:
            return core::FormatType::NV12;
        case DRM_FORMAT_NV21:
            return core::FormatType::NV21;
        case DRM_FORMAT_NV16:
            return core::FormatType::NV16;
        case DRM_FORMAT_NV61:
            return core::FormatType::NV61;
        case DRM_FORMAT_NV24:
            return core::FormatType::NV24;
        case DRM_FORMAT_NV42:
            return core::FormatType::NV42;

        case DRM_FORMAT_NV15:
            return core::FormatType::NV15;
        case DRM_FORMAT_NV20:
            return core::FormatType::NV20;
        case DRM_FORMAT_NV30:
            return core::FormatType::NV30;

        case DRM_FORMAT_P210:
            return core::FormatType::P210;
        case DRM_FORMAT_P010:
            return core::FormatType::P010;
        case DRM_FORMAT_P012:
            return core::FormatType::P012;
        case DRM_FORMAT_P016:
            return core::FormatType::P016;
        case DRM_FORMAT_P030:
            return core::FormatType::P030;
        case DRM_FORMAT_P230:
            return core::FormatType::P230;

        case DRM_FORMAT_Q410:
            return core::FormatType::Q410;
        case DRM_FORMAT_Q401:
            return core::FormatType::Q401;

        case DRM_FORMAT_T430:
            return core::FormatType::T430;

        case DRM_FORMAT_S010:
            return core::FormatType::S010;
        case DRM_FORMAT_S210:
            return core::FormatType::S210;
        case DRM_FORMAT_S410:
            return core::FormatType::S410;

        case DRM_FORMAT_S012:
            return core::FormatType::S012;
        case DRM_FORMAT_S212:
            return core::FormatType::S212;
        case DRM_FORMAT_S412:
            return core::FormatType::S412;

        case DRM_FORMAT_S016:
            return core::FormatType::S016;
        case DRM_FORMAT_S216:
            return core::FormatType::S216;
        case DRM_FORMAT_S416:
            return core::FormatType::S416;

        case DRM_FORMAT_YUV410:
            return core::FormatType::YUV410;
        case DRM_FORMAT_YVU410:
            return core::FormatType::YVU410;
        case DRM_FORMAT_YUV411:
            return core::FormatType::YUV411;
        case DRM_FORMAT_YVU411:
            return core::FormatType::YVU411;
        case DRM_FORMAT_YUV420:
            return core::FormatType::YUV420;
        case DRM_FORMAT_YVU420:
            return core::FormatType::YVU420;
        case DRM_FORMAT_YUV422:
            return core::FormatType::YUV422;
        case DRM_FORMAT_YVU422:
            return core::FormatType::YVU422;
        case DRM_FORMAT_YUV444:
            return core::FormatType::YUV444;
        case DRM_FORMAT_YVU444:
            return core::FormatType::YVU444;

        case DRM_FORMAT_Y8:
            return core::FormatType::Y8;
        case DRM_FORMAT_XYYY2101010:
            return core::FormatType::XYYY2101010;

        default:
            return core::FormatType::Unknown;
        }
    }

    [[nodiscard]] static constexpr JobDrmFormat fromRenderFormat(core::FormatType format) noexcept
    {
        switch (format) {
        case core::FormatType::Unknown:
            break;

        case core::FormatType::C1:
            return JobDrmFormat{DRM_FORMAT_C1};
        case core::FormatType::C2:
            return JobDrmFormat{DRM_FORMAT_C2};
        case core::FormatType::C4:
            return JobDrmFormat{DRM_FORMAT_C4};
        case core::FormatType::C8:
            return JobDrmFormat{DRM_FORMAT_C8};

        case core::FormatType::D1:
            return JobDrmFormat{DRM_FORMAT_D1};
        case core::FormatType::D2:
            return JobDrmFormat{DRM_FORMAT_D2};
        case core::FormatType::D4:
            return JobDrmFormat{DRM_FORMAT_D4};
        case core::FormatType::D8:
            return JobDrmFormat{DRM_FORMAT_D8};

        case core::FormatType::R1:
            return JobDrmFormat{DRM_FORMAT_R1};
        case core::FormatType::R2:
            return JobDrmFormat{DRM_FORMAT_R2};
        case core::FormatType::R4:
            return JobDrmFormat{DRM_FORMAT_R4};
        case core::FormatType::R8:
            return JobDrmFormat{DRM_FORMAT_R8};
        case core::FormatType::R10:
            return JobDrmFormat{DRM_FORMAT_R10};
        case core::FormatType::R12:
            return JobDrmFormat{DRM_FORMAT_R12};
        case core::FormatType::R16:
            return JobDrmFormat{DRM_FORMAT_R16};

        case core::FormatType::RG88:
            return JobDrmFormat{DRM_FORMAT_RG88};
        case core::FormatType::GR88:
            return JobDrmFormat{DRM_FORMAT_GR88};
        case core::FormatType::RG1616:
            return JobDrmFormat{DRM_FORMAT_RG1616};
        case core::FormatType::GR1616:
            return JobDrmFormat{DRM_FORMAT_GR1616};

        case core::FormatType::RGB332:
            return JobDrmFormat{DRM_FORMAT_RGB332};
        case core::FormatType::BGR233:
            return JobDrmFormat{DRM_FORMAT_BGR233};

        case core::FormatType::XRGB4444:
            return JobDrmFormat{DRM_FORMAT_XRGB4444};
        case core::FormatType::XBGR4444:
            return JobDrmFormat{DRM_FORMAT_XBGR4444};
        case core::FormatType::RGBX4444:
            return JobDrmFormat{DRM_FORMAT_RGBX4444};
        case core::FormatType::BGRX4444:
            return JobDrmFormat{DRM_FORMAT_BGRX4444};
        case core::FormatType::ARGB4444:
            return JobDrmFormat{DRM_FORMAT_ARGB4444};
        case core::FormatType::ABGR4444:
            return JobDrmFormat{DRM_FORMAT_ABGR4444};
        case core::FormatType::RGBA4444:
            return JobDrmFormat{DRM_FORMAT_RGBA4444};
        case core::FormatType::BGRA4444:
            return JobDrmFormat{DRM_FORMAT_BGRA4444};

        case core::FormatType::XRGB1555:
            return JobDrmFormat{DRM_FORMAT_XRGB1555};
        case core::FormatType::XBGR1555:
            return JobDrmFormat{DRM_FORMAT_XBGR1555};
        case core::FormatType::RGBX5551:
            return JobDrmFormat{DRM_FORMAT_RGBX5551};
        case core::FormatType::BGRX5551:
            return JobDrmFormat{DRM_FORMAT_BGRX5551};
        case core::FormatType::ARGB1555:
            return JobDrmFormat{DRM_FORMAT_ARGB1555};
        case core::FormatType::ABGR1555:
            return JobDrmFormat{DRM_FORMAT_ABGR1555};
        case core::FormatType::RGBA5551:
            return JobDrmFormat{DRM_FORMAT_RGBA5551};
        case core::FormatType::BGRA5551:
            return JobDrmFormat{DRM_FORMAT_BGRA5551};

        case core::FormatType::RGB565:
            return JobDrmFormat{DRM_FORMAT_RGB565};
        case core::FormatType::BGR565:
            return JobDrmFormat{DRM_FORMAT_BGR565};

        case core::FormatType::RGB888:
            return JobDrmFormat{DRM_FORMAT_RGB888};
        case core::FormatType::BGR888:
            return JobDrmFormat{DRM_FORMAT_BGR888};

        case core::FormatType::XRGB8888:
            return JobDrmFormat{DRM_FORMAT_XRGB8888};
        case core::FormatType::XBGR8888:
            return JobDrmFormat{DRM_FORMAT_XBGR8888};
        case core::FormatType::RGBX8888:
            return JobDrmFormat{DRM_FORMAT_RGBX8888};
        case core::FormatType::BGRX8888:
            return JobDrmFormat{DRM_FORMAT_BGRX8888};
        case core::FormatType::ARGB8888:
            return JobDrmFormat{DRM_FORMAT_ARGB8888};
        case core::FormatType::ABGR8888:
            return JobDrmFormat{DRM_FORMAT_ABGR8888};
        case core::FormatType::RGBA8888:
            return JobDrmFormat{DRM_FORMAT_RGBA8888};
        case core::FormatType::BGRA8888:
            return JobDrmFormat{DRM_FORMAT_BGRA8888};

        case core::FormatType::XRGB2101010:
            return JobDrmFormat{DRM_FORMAT_XRGB2101010};
        case core::FormatType::XBGR2101010:
            return JobDrmFormat{DRM_FORMAT_XBGR2101010};
        case core::FormatType::RGBX1010102:
            return JobDrmFormat{DRM_FORMAT_RGBX1010102};
        case core::FormatType::BGRX1010102:
            return JobDrmFormat{DRM_FORMAT_BGRX1010102};
        case core::FormatType::ARGB2101010:
            return JobDrmFormat{DRM_FORMAT_ARGB2101010};
        case core::FormatType::ABGR2101010:
            return JobDrmFormat{DRM_FORMAT_ABGR2101010};
        case core::FormatType::RGBA1010102:
            return JobDrmFormat{DRM_FORMAT_RGBA1010102};
        case core::FormatType::BGRA1010102:
            return JobDrmFormat{DRM_FORMAT_BGRA1010102};

        case core::FormatType::RGB161616:
            return JobDrmFormat{DRM_FORMAT_RGB161616};
        case core::FormatType::BGR161616:
            return JobDrmFormat{DRM_FORMAT_BGR161616};

        case core::FormatType::XRGB16161616:
            return JobDrmFormat{DRM_FORMAT_XRGB16161616};
        case core::FormatType::XBGR16161616:
            return JobDrmFormat{DRM_FORMAT_XBGR16161616};
        case core::FormatType::ARGB16161616:
            return JobDrmFormat{DRM_FORMAT_ARGB16161616};
        case core::FormatType::ABGR16161616:
            return JobDrmFormat{DRM_FORMAT_ABGR16161616};

        case core::FormatType::XRGB16161616F:
            return JobDrmFormat{DRM_FORMAT_XRGB16161616F};
        case core::FormatType::XBGR16161616F:
            return JobDrmFormat{DRM_FORMAT_XBGR16161616F};
        case core::FormatType::ARGB16161616F:
            return JobDrmFormat{DRM_FORMAT_ARGB16161616F};
        case core::FormatType::ABGR16161616F:
            return JobDrmFormat{DRM_FORMAT_ABGR16161616F};

        case core::FormatType::R16F:
            return JobDrmFormat{DRM_FORMAT_R16F};
        case core::FormatType::GR1616F:
            return JobDrmFormat{DRM_FORMAT_GR1616F};
        case core::FormatType::BGR161616F:
            return JobDrmFormat{DRM_FORMAT_BGR161616F};

        case core::FormatType::R32F:
            return JobDrmFormat{DRM_FORMAT_R32F};
        case core::FormatType::GR3232F:
            return JobDrmFormat{DRM_FORMAT_GR3232F};
        case core::FormatType::BGR323232F:
            return JobDrmFormat{DRM_FORMAT_BGR323232F};
        case core::FormatType::ABGR32323232F:
            return JobDrmFormat{DRM_FORMAT_ABGR32323232F};

        case core::FormatType::AXBXGXRX106106106106:
            return JobDrmFormat{DRM_FORMAT_AXBXGXRX106106106106};

        case core::FormatType::YUYV:
            return JobDrmFormat{DRM_FORMAT_YUYV};
        case core::FormatType::YVYU:
            return JobDrmFormat{DRM_FORMAT_YVYU};
        case core::FormatType::UYVY:
            return JobDrmFormat{DRM_FORMAT_UYVY};
        case core::FormatType::VYUY:
            return JobDrmFormat{DRM_FORMAT_VYUY};

        case core::FormatType::AYUV:
            return JobDrmFormat{DRM_FORMAT_AYUV};
        case core::FormatType::AVUY8888:
            return JobDrmFormat{DRM_FORMAT_AVUY8888};
        case core::FormatType::XYUV8888:
            return JobDrmFormat{DRM_FORMAT_XYUV8888};
        case core::FormatType::XVUY8888:
            return JobDrmFormat{DRM_FORMAT_XVUY8888};
        case core::FormatType::VUY888:
            return JobDrmFormat{DRM_FORMAT_VUY888};
        case core::FormatType::VUY101010:
            return JobDrmFormat{DRM_FORMAT_VUY101010};
        case core::FormatType::XVUY2101010:
            return JobDrmFormat{DRM_FORMAT_XVUY2101010};

        case core::FormatType::Y210:
            return JobDrmFormat{DRM_FORMAT_Y210};
        case core::FormatType::Y212:
            return JobDrmFormat{DRM_FORMAT_Y212};
        case core::FormatType::Y216:
            return JobDrmFormat{DRM_FORMAT_Y216};

        case core::FormatType::Y410:
            return JobDrmFormat{DRM_FORMAT_Y410};
        case core::FormatType::Y412:
            return JobDrmFormat{DRM_FORMAT_Y412};
        case core::FormatType::Y416:
            return JobDrmFormat{DRM_FORMAT_Y416};

        case core::FormatType::XVYU2101010:
            return JobDrmFormat{DRM_FORMAT_XVYU2101010};
        case core::FormatType::XVYU12_16161616:
            return JobDrmFormat{DRM_FORMAT_XVYU12_16161616};
        case core::FormatType::XVYU16161616:
            return JobDrmFormat{DRM_FORMAT_XVYU16161616};

        case core::FormatType::Y0L0:
            return JobDrmFormat{DRM_FORMAT_Y0L0};
        case core::FormatType::X0L0:
            return JobDrmFormat{DRM_FORMAT_X0L0};
        case core::FormatType::Y0L2:
            return JobDrmFormat{DRM_FORMAT_Y0L2};
        case core::FormatType::X0L2:
            return JobDrmFormat{DRM_FORMAT_X0L2};

        case core::FormatType::YUV420_8BIT:
            return JobDrmFormat{DRM_FORMAT_YUV420_8BIT};
        case core::FormatType::YUV420_10BIT:
            return JobDrmFormat{DRM_FORMAT_YUV420_10BIT};

        case core::FormatType::XRGB8888_A8:
            return JobDrmFormat{DRM_FORMAT_XRGB8888_A8};
        case core::FormatType::XBGR8888_A8:
            return JobDrmFormat{DRM_FORMAT_XBGR8888_A8};
        case core::FormatType::RGBX8888_A8:
            return JobDrmFormat{DRM_FORMAT_RGBX8888_A8};
        case core::FormatType::BGRX8888_A8:
            return JobDrmFormat{DRM_FORMAT_BGRX8888_A8};
        case core::FormatType::RGB888_A8:
            return JobDrmFormat{DRM_FORMAT_RGB888_A8};
        case core::FormatType::BGR888_A8:
            return JobDrmFormat{DRM_FORMAT_BGR888_A8};
        case core::FormatType::RGB565_A8:
            return JobDrmFormat{DRM_FORMAT_RGB565_A8};
        case core::FormatType::BGR565_A8:
            return JobDrmFormat{DRM_FORMAT_BGR565_A8};

        case core::FormatType::NV12:
            return JobDrmFormat{DRM_FORMAT_NV12};
        case core::FormatType::NV21:
            return JobDrmFormat{DRM_FORMAT_NV21};
        case core::FormatType::NV16:
            return JobDrmFormat{DRM_FORMAT_NV16};
        case core::FormatType::NV61:
            return JobDrmFormat{DRM_FORMAT_NV61};
        case core::FormatType::NV24:
            return JobDrmFormat{DRM_FORMAT_NV24};
        case core::FormatType::NV42:
            return JobDrmFormat{DRM_FORMAT_NV42};

        case core::FormatType::NV15:
            return JobDrmFormat{DRM_FORMAT_NV15};
        case core::FormatType::NV20:
            return JobDrmFormat{DRM_FORMAT_NV20};
        case core::FormatType::NV30:
            return JobDrmFormat{DRM_FORMAT_NV30};

        case core::FormatType::P210:
            return JobDrmFormat{DRM_FORMAT_P210};
        case core::FormatType::P010:
            return JobDrmFormat{DRM_FORMAT_P010};
        case core::FormatType::P012:
            return JobDrmFormat{DRM_FORMAT_P012};
        case core::FormatType::P016:
            return JobDrmFormat{DRM_FORMAT_P016};
        case core::FormatType::P030:
            return JobDrmFormat{DRM_FORMAT_P030};
        case core::FormatType::P230:
            return JobDrmFormat{DRM_FORMAT_P230};

        case core::FormatType::Q410:
            return JobDrmFormat{DRM_FORMAT_Q410};
        case core::FormatType::Q401:
            return JobDrmFormat{DRM_FORMAT_Q401};

        case core::FormatType::T430:
            return JobDrmFormat{DRM_FORMAT_T430};

        case core::FormatType::S010:
            return JobDrmFormat{DRM_FORMAT_S010};
        case core::FormatType::S210:
            return JobDrmFormat{DRM_FORMAT_S210};
        case core::FormatType::S410:
            return JobDrmFormat{DRM_FORMAT_S410};

        case core::FormatType::S012:
            return JobDrmFormat{DRM_FORMAT_S012};
        case core::FormatType::S212:
            return JobDrmFormat{DRM_FORMAT_S212};
        case core::FormatType::S412:
            return JobDrmFormat{DRM_FORMAT_S412};

        case core::FormatType::S016:
            return JobDrmFormat{DRM_FORMAT_S016};
        case core::FormatType::S216:
            return JobDrmFormat{DRM_FORMAT_S216};
        case core::FormatType::S416:
            return JobDrmFormat{DRM_FORMAT_S416};

        case core::FormatType::YUV410:
            return JobDrmFormat{DRM_FORMAT_YUV410};
        case core::FormatType::YVU410:
            return JobDrmFormat{DRM_FORMAT_YVU410};
        case core::FormatType::YUV411:
            return JobDrmFormat{DRM_FORMAT_YUV411};
        case core::FormatType::YVU411:
            return JobDrmFormat{DRM_FORMAT_YVU411};
        case core::FormatType::YUV420:
            return JobDrmFormat{DRM_FORMAT_YUV420};
        case core::FormatType::YVU420:
            return JobDrmFormat{DRM_FORMAT_YVU420};
        case core::FormatType::YUV422:
            return JobDrmFormat{DRM_FORMAT_YUV422};
        case core::FormatType::YVU422:
            return JobDrmFormat{DRM_FORMAT_YVU422};
        case core::FormatType::YUV444:
            return JobDrmFormat{DRM_FORMAT_YUV444};
        case core::FormatType::YVU444:
            return JobDrmFormat{DRM_FORMAT_YVU444};

        case core::FormatType::Y8:
            return JobDrmFormat{DRM_FORMAT_Y8};
        case core::FormatType::XYYY2101010:
            return JobDrmFormat{DRM_FORMAT_XYYY2101010};

        }

        return {};
    }


    [[nodiscard]] friend constexpr bool operator==(const JobDrmFormat &, const JobDrmFormat &) noexcept = default;

private:
    std::uint32_t m_value{DRM_FORMAT_INVALID};
};

}