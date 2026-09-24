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


    [[nodiscard]] constexpr FormatType renderFormat() const noexcept
    {
        switch (baseValue()) {
        case DRM_FORMAT_INVALID:
            return FormatType::Unknown;


        case DRM_FORMAT_C1:
            return FormatType::C1;
        case DRM_FORMAT_C2:
            return FormatType::C2;
        case DRM_FORMAT_C4:
            return FormatType::C4;
        case DRM_FORMAT_C8:
            return FormatType::C8;

        case DRM_FORMAT_D1:
            return FormatType::D1;
        case DRM_FORMAT_D2:
            return FormatType::D2;
        case DRM_FORMAT_D4:
            return FormatType::D4;
        case DRM_FORMAT_D8:
            return FormatType::D8;

        case DRM_FORMAT_R1:
            return FormatType::R1;
        case DRM_FORMAT_R2:
            return FormatType::R2;
        case DRM_FORMAT_R4:
            return FormatType::R4;
        case DRM_FORMAT_R8:
            return FormatType::R8;
        case DRM_FORMAT_R10:
            return FormatType::R10;
        case DRM_FORMAT_R12:
            return FormatType::R12;
        case DRM_FORMAT_R16:
            return FormatType::R16;

        case DRM_FORMAT_RG88:
            return FormatType::RG88;
        case DRM_FORMAT_GR88:
            return FormatType::GR88;
        case DRM_FORMAT_RG1616:
            return FormatType::RG1616;
        case DRM_FORMAT_GR1616:
            return FormatType::GR1616;

        case DRM_FORMAT_RGB332:
            return FormatType::RGB332;
        case DRM_FORMAT_BGR233:
            return FormatType::BGR233;

        case DRM_FORMAT_XRGB4444:
            return FormatType::XRGB4444;
        case DRM_FORMAT_XBGR4444:
            return FormatType::XBGR4444;
        case DRM_FORMAT_RGBX4444:
            return FormatType::RGBX4444;
        case DRM_FORMAT_BGRX4444:
            return FormatType::BGRX4444;
        case DRM_FORMAT_ARGB4444:
            return FormatType::ARGB4444;
        case DRM_FORMAT_ABGR4444:
            return FormatType::ABGR4444;
        case DRM_FORMAT_RGBA4444:
            return FormatType::RGBA4444;
        case DRM_FORMAT_BGRA4444:
            return FormatType::BGRA4444;

        case DRM_FORMAT_XRGB1555:
            return FormatType::XRGB1555;
        case DRM_FORMAT_XBGR1555:
            return FormatType::XBGR1555;
        case DRM_FORMAT_RGBX5551:
            return FormatType::RGBX5551;
        case DRM_FORMAT_BGRX5551:
            return FormatType::BGRX5551;
        case DRM_FORMAT_ARGB1555:
            return FormatType::ARGB1555;
        case DRM_FORMAT_ABGR1555:
            return FormatType::ABGR1555;
        case DRM_FORMAT_RGBA5551:
            return FormatType::RGBA5551;
        case DRM_FORMAT_BGRA5551:
            return FormatType::BGRA5551;

        case DRM_FORMAT_RGB565:
            return FormatType::RGB565;
        case DRM_FORMAT_BGR565:
            return FormatType::BGR565;

        case DRM_FORMAT_RGB888:
            return FormatType::RGB888;
        case DRM_FORMAT_BGR888:
            return FormatType::BGR888;

        case DRM_FORMAT_XRGB8888:
            return FormatType::XRGB8888;
        case DRM_FORMAT_XBGR8888:
            return FormatType::XBGR8888;
        case DRM_FORMAT_RGBX8888:
            return FormatType::RGBX8888;
        case DRM_FORMAT_BGRX8888:
            return FormatType::BGRX8888;
        case DRM_FORMAT_ARGB8888:
            return FormatType::ARGB8888;
        case DRM_FORMAT_ABGR8888:
            return FormatType::ABGR8888;
        case DRM_FORMAT_RGBA8888:
            return FormatType::RGBA8888;
        case DRM_FORMAT_BGRA8888:
            return FormatType::BGRA8888;

        case DRM_FORMAT_XRGB2101010:
            return FormatType::XRGB2101010;
        case DRM_FORMAT_XBGR2101010:
            return FormatType::XBGR2101010;
        case DRM_FORMAT_RGBX1010102:
            return FormatType::RGBX1010102;
        case DRM_FORMAT_BGRX1010102:
            return FormatType::BGRX1010102;
        case DRM_FORMAT_ARGB2101010:
            return FormatType::ARGB2101010;
        case DRM_FORMAT_ABGR2101010:
            return FormatType::ABGR2101010;
        case DRM_FORMAT_RGBA1010102:
            return FormatType::RGBA1010102;
        case DRM_FORMAT_BGRA1010102:
            return FormatType::BGRA1010102;

        case DRM_FORMAT_RGB161616:
            return FormatType::RGB161616;
        case DRM_FORMAT_BGR161616:
            return FormatType::BGR161616;

        case DRM_FORMAT_XRGB16161616:
            return FormatType::XRGB16161616;
        case DRM_FORMAT_XBGR16161616:
            return FormatType::XBGR16161616;
        case DRM_FORMAT_ARGB16161616:
            return FormatType::ARGB16161616;
        case DRM_FORMAT_ABGR16161616:
            return FormatType::ABGR16161616;

        case DRM_FORMAT_XRGB16161616F:
            return FormatType::XRGB16161616F;
        case DRM_FORMAT_XBGR16161616F:
            return FormatType::XBGR16161616F;
        case DRM_FORMAT_ARGB16161616F:
            return FormatType::ARGB16161616F;
        case DRM_FORMAT_ABGR16161616F:
            return FormatType::ABGR16161616F;

        case DRM_FORMAT_R16F:
            return FormatType::R16F;
        case DRM_FORMAT_GR1616F:
            return FormatType::GR1616F;
        case DRM_FORMAT_BGR161616F:
            return FormatType::BGR161616F;

        case DRM_FORMAT_R32F:
            return FormatType::R32F;
        case DRM_FORMAT_GR3232F:
            return FormatType::GR3232F;
        case DRM_FORMAT_BGR323232F:
            return FormatType::BGR323232F;
        case DRM_FORMAT_ABGR32323232F:
            return FormatType::ABGR32323232F;

        case DRM_FORMAT_AXBXGXRX106106106106:
            return FormatType::AXBXGXRX106106106106;

        case DRM_FORMAT_YUYV:
            return FormatType::YUYV;
        case DRM_FORMAT_YVYU:
            return FormatType::YVYU;
        case DRM_FORMAT_UYVY:
            return FormatType::UYVY;
        case DRM_FORMAT_VYUY:
            return FormatType::VYUY;

        case DRM_FORMAT_AYUV:
            return FormatType::AYUV;
        case DRM_FORMAT_AVUY8888:
            return FormatType::AVUY8888;
        case DRM_FORMAT_XYUV8888:
            return FormatType::XYUV8888;
        case DRM_FORMAT_XVUY8888:
            return FormatType::XVUY8888;
        case DRM_FORMAT_VUY888:
            return FormatType::VUY888;
        case DRM_FORMAT_VUY101010:
            return FormatType::VUY101010;
        case DRM_FORMAT_XVUY2101010:
            return FormatType::XVUY2101010;

        case DRM_FORMAT_Y210:
            return FormatType::Y210;
        case DRM_FORMAT_Y212:
            return FormatType::Y212;
        case DRM_FORMAT_Y216:
            return FormatType::Y216;

        case DRM_FORMAT_Y410:
            return FormatType::Y410;
        case DRM_FORMAT_Y412:
            return FormatType::Y412;
        case DRM_FORMAT_Y416:
            return FormatType::Y416;

        case DRM_FORMAT_XVYU2101010:
            return FormatType::XVYU2101010;
        case DRM_FORMAT_XVYU12_16161616:
            return FormatType::XVYU12_16161616;
        case DRM_FORMAT_XVYU16161616:
            return FormatType::XVYU16161616;

        case DRM_FORMAT_Y0L0:
            return FormatType::Y0L0;
        case DRM_FORMAT_X0L0:
            return FormatType::X0L0;
        case DRM_FORMAT_Y0L2:
            return FormatType::Y0L2;
        case DRM_FORMAT_X0L2:
            return FormatType::X0L2;

        case DRM_FORMAT_YUV420_8BIT:
            return FormatType::YUV420_8BIT;
        case DRM_FORMAT_YUV420_10BIT:
            return FormatType::YUV420_10BIT;

        case DRM_FORMAT_XRGB8888_A8:
            return FormatType::XRGB8888_A8;
        case DRM_FORMAT_XBGR8888_A8:
            return FormatType::XBGR8888_A8;
        case DRM_FORMAT_RGBX8888_A8:
            return FormatType::RGBX8888_A8;
        case DRM_FORMAT_BGRX8888_A8:
            return FormatType::BGRX8888_A8;
        case DRM_FORMAT_RGB888_A8:
            return FormatType::RGB888_A8;
        case DRM_FORMAT_BGR888_A8:
            return FormatType::BGR888_A8;
        case DRM_FORMAT_RGB565_A8:
            return FormatType::RGB565_A8;
        case DRM_FORMAT_BGR565_A8:
            return FormatType::BGR565_A8;

        case DRM_FORMAT_NV12:
            return FormatType::NV12;
        case DRM_FORMAT_NV21:
            return FormatType::NV21;
        case DRM_FORMAT_NV16:
            return FormatType::NV16;
        case DRM_FORMAT_NV61:
            return FormatType::NV61;
        case DRM_FORMAT_NV24:
            return FormatType::NV24;
        case DRM_FORMAT_NV42:
            return FormatType::NV42;

        case DRM_FORMAT_NV15:
            return FormatType::NV15;
        case DRM_FORMAT_NV20:
            return FormatType::NV20;
        case DRM_FORMAT_NV30:
            return FormatType::NV30;

        case DRM_FORMAT_P210:
            return FormatType::P210;
        case DRM_FORMAT_P010:
            return FormatType::P010;
        case DRM_FORMAT_P012:
            return FormatType::P012;
        case DRM_FORMAT_P016:
            return FormatType::P016;
        case DRM_FORMAT_P030:
            return FormatType::P030;
        case DRM_FORMAT_P230:
            return FormatType::P230;

        case DRM_FORMAT_Q410:
            return FormatType::Q410;
        case DRM_FORMAT_Q401:
            return FormatType::Q401;

        case DRM_FORMAT_T430:
            return FormatType::T430;

        case DRM_FORMAT_S010:
            return FormatType::S010;
        case DRM_FORMAT_S210:
            return FormatType::S210;
        case DRM_FORMAT_S410:
            return FormatType::S410;

        case DRM_FORMAT_S012:
            return FormatType::S012;
        case DRM_FORMAT_S212:
            return FormatType::S212;
        case DRM_FORMAT_S412:
            return FormatType::S412;

        case DRM_FORMAT_S016:
            return FormatType::S016;
        case DRM_FORMAT_S216:
            return FormatType::S216;
        case DRM_FORMAT_S416:
            return FormatType::S416;

        case DRM_FORMAT_YUV410:
            return FormatType::YUV410;
        case DRM_FORMAT_YVU410:
            return FormatType::YVU410;
        case DRM_FORMAT_YUV411:
            return FormatType::YUV411;
        case DRM_FORMAT_YVU411:
            return FormatType::YVU411;
        case DRM_FORMAT_YUV420:
            return FormatType::YUV420;
        case DRM_FORMAT_YVU420:
            return FormatType::YVU420;
        case DRM_FORMAT_YUV422:
            return FormatType::YUV422;
        case DRM_FORMAT_YVU422:
            return FormatType::YVU422;
        case DRM_FORMAT_YUV444:
            return FormatType::YUV444;
        case DRM_FORMAT_YVU444:
            return FormatType::YVU444;

        case DRM_FORMAT_Y8:
            return FormatType::Y8;
        case DRM_FORMAT_XYYY2101010:
            return FormatType::XYYY2101010;

        default:
            return FormatType::Unknown;
        }
    }

    [[nodiscard]] static constexpr JobDrmFormat fromRenderFormat(FormatType format) noexcept
    {
        switch (format) {
        case FormatType::Unknown:
            break;

        case FormatType::C1:
            return JobDrmFormat{DRM_FORMAT_C1};
        case FormatType::C2:
            return JobDrmFormat{DRM_FORMAT_C2};
        case FormatType::C4:
            return JobDrmFormat{DRM_FORMAT_C4};
        case FormatType::C8:
            return JobDrmFormat{DRM_FORMAT_C8};

        case FormatType::D1:
            return JobDrmFormat{DRM_FORMAT_D1};
        case FormatType::D2:
            return JobDrmFormat{DRM_FORMAT_D2};
        case FormatType::D4:
            return JobDrmFormat{DRM_FORMAT_D4};
        case FormatType::D8:
            return JobDrmFormat{DRM_FORMAT_D8};

        case FormatType::R1:
            return JobDrmFormat{DRM_FORMAT_R1};
        case FormatType::R2:
            return JobDrmFormat{DRM_FORMAT_R2};
        case FormatType::R4:
            return JobDrmFormat{DRM_FORMAT_R4};
        case FormatType::R8:
            return JobDrmFormat{DRM_FORMAT_R8};
        case FormatType::R10:
            return JobDrmFormat{DRM_FORMAT_R10};
        case FormatType::R12:
            return JobDrmFormat{DRM_FORMAT_R12};
        case FormatType::R16:
            return JobDrmFormat{DRM_FORMAT_R16};

        case FormatType::RG88:
            return JobDrmFormat{DRM_FORMAT_RG88};
        case FormatType::GR88:
            return JobDrmFormat{DRM_FORMAT_GR88};
        case FormatType::RG1616:
            return JobDrmFormat{DRM_FORMAT_RG1616};
        case FormatType::GR1616:
            return JobDrmFormat{DRM_FORMAT_GR1616};

        case FormatType::RGB332:
            return JobDrmFormat{DRM_FORMAT_RGB332};
        case FormatType::BGR233:
            return JobDrmFormat{DRM_FORMAT_BGR233};

        case FormatType::XRGB4444:
            return JobDrmFormat{DRM_FORMAT_XRGB4444};
        case FormatType::XBGR4444:
            return JobDrmFormat{DRM_FORMAT_XBGR4444};
        case FormatType::RGBX4444:
            return JobDrmFormat{DRM_FORMAT_RGBX4444};
        case FormatType::BGRX4444:
            return JobDrmFormat{DRM_FORMAT_BGRX4444};
        case FormatType::ARGB4444:
            return JobDrmFormat{DRM_FORMAT_ARGB4444};
        case FormatType::ABGR4444:
            return JobDrmFormat{DRM_FORMAT_ABGR4444};
        case FormatType::RGBA4444:
            return JobDrmFormat{DRM_FORMAT_RGBA4444};
        case FormatType::BGRA4444:
            return JobDrmFormat{DRM_FORMAT_BGRA4444};

        case FormatType::XRGB1555:
            return JobDrmFormat{DRM_FORMAT_XRGB1555};
        case FormatType::XBGR1555:
            return JobDrmFormat{DRM_FORMAT_XBGR1555};
        case FormatType::RGBX5551:
            return JobDrmFormat{DRM_FORMAT_RGBX5551};
        case FormatType::BGRX5551:
            return JobDrmFormat{DRM_FORMAT_BGRX5551};
        case FormatType::ARGB1555:
            return JobDrmFormat{DRM_FORMAT_ARGB1555};
        case FormatType::ABGR1555:
            return JobDrmFormat{DRM_FORMAT_ABGR1555};
        case FormatType::RGBA5551:
            return JobDrmFormat{DRM_FORMAT_RGBA5551};
        case FormatType::BGRA5551:
            return JobDrmFormat{DRM_FORMAT_BGRA5551};

        case FormatType::RGB565:
            return JobDrmFormat{DRM_FORMAT_RGB565};
        case FormatType::BGR565:
            return JobDrmFormat{DRM_FORMAT_BGR565};

        case FormatType::RGB888:
            return JobDrmFormat{DRM_FORMAT_RGB888};
        case FormatType::BGR888:
            return JobDrmFormat{DRM_FORMAT_BGR888};

        case FormatType::XRGB8888:
            return JobDrmFormat{DRM_FORMAT_XRGB8888};
        case FormatType::XBGR8888:
            return JobDrmFormat{DRM_FORMAT_XBGR8888};
        case FormatType::RGBX8888:
            return JobDrmFormat{DRM_FORMAT_RGBX8888};
        case FormatType::BGRX8888:
            return JobDrmFormat{DRM_FORMAT_BGRX8888};
        case FormatType::ARGB8888:
            return JobDrmFormat{DRM_FORMAT_ARGB8888};
        case FormatType::ABGR8888:
            return JobDrmFormat{DRM_FORMAT_ABGR8888};
        case FormatType::RGBA8888:
            return JobDrmFormat{DRM_FORMAT_RGBA8888};
        case FormatType::BGRA8888:
            return JobDrmFormat{DRM_FORMAT_BGRA8888};

        case FormatType::XRGB2101010:
            return JobDrmFormat{DRM_FORMAT_XRGB2101010};
        case FormatType::XBGR2101010:
            return JobDrmFormat{DRM_FORMAT_XBGR2101010};
        case FormatType::RGBX1010102:
            return JobDrmFormat{DRM_FORMAT_RGBX1010102};
        case FormatType::BGRX1010102:
            return JobDrmFormat{DRM_FORMAT_BGRX1010102};
        case FormatType::ARGB2101010:
            return JobDrmFormat{DRM_FORMAT_ARGB2101010};
        case FormatType::ABGR2101010:
            return JobDrmFormat{DRM_FORMAT_ABGR2101010};
        case FormatType::RGBA1010102:
            return JobDrmFormat{DRM_FORMAT_RGBA1010102};
        case FormatType::BGRA1010102:
            return JobDrmFormat{DRM_FORMAT_BGRA1010102};

        case FormatType::RGB161616:
            return JobDrmFormat{DRM_FORMAT_RGB161616};
        case FormatType::BGR161616:
            return JobDrmFormat{DRM_FORMAT_BGR161616};

        case FormatType::XRGB16161616:
            return JobDrmFormat{DRM_FORMAT_XRGB16161616};
        case FormatType::XBGR16161616:
            return JobDrmFormat{DRM_FORMAT_XBGR16161616};
        case FormatType::ARGB16161616:
            return JobDrmFormat{DRM_FORMAT_ARGB16161616};
        case FormatType::ABGR16161616:
            return JobDrmFormat{DRM_FORMAT_ABGR16161616};

        case FormatType::XRGB16161616F:
            return JobDrmFormat{DRM_FORMAT_XRGB16161616F};
        case FormatType::XBGR16161616F:
            return JobDrmFormat{DRM_FORMAT_XBGR16161616F};
        case FormatType::ARGB16161616F:
            return JobDrmFormat{DRM_FORMAT_ARGB16161616F};
        case FormatType::ABGR16161616F:
            return JobDrmFormat{DRM_FORMAT_ABGR16161616F};

        case FormatType::R16F:
            return JobDrmFormat{DRM_FORMAT_R16F};
        case FormatType::GR1616F:
            return JobDrmFormat{DRM_FORMAT_GR1616F};
        case FormatType::BGR161616F:
            return JobDrmFormat{DRM_FORMAT_BGR161616F};

        case FormatType::R32F:
            return JobDrmFormat{DRM_FORMAT_R32F};
        case FormatType::GR3232F:
            return JobDrmFormat{DRM_FORMAT_GR3232F};
        case FormatType::BGR323232F:
            return JobDrmFormat{DRM_FORMAT_BGR323232F};
        case FormatType::ABGR32323232F:
            return JobDrmFormat{DRM_FORMAT_ABGR32323232F};

        case FormatType::AXBXGXRX106106106106:
            return JobDrmFormat{DRM_FORMAT_AXBXGXRX106106106106};

        case FormatType::YUYV:
            return JobDrmFormat{DRM_FORMAT_YUYV};
        case FormatType::YVYU:
            return JobDrmFormat{DRM_FORMAT_YVYU};
        case FormatType::UYVY:
            return JobDrmFormat{DRM_FORMAT_UYVY};
        case FormatType::VYUY:
            return JobDrmFormat{DRM_FORMAT_VYUY};

        case FormatType::AYUV:
            return JobDrmFormat{DRM_FORMAT_AYUV};
        case FormatType::AVUY8888:
            return JobDrmFormat{DRM_FORMAT_AVUY8888};
        case FormatType::XYUV8888:
            return JobDrmFormat{DRM_FORMAT_XYUV8888};
        case FormatType::XVUY8888:
            return JobDrmFormat{DRM_FORMAT_XVUY8888};
        case FormatType::VUY888:
            return JobDrmFormat{DRM_FORMAT_VUY888};
        case FormatType::VUY101010:
            return JobDrmFormat{DRM_FORMAT_VUY101010};
        case FormatType::XVUY2101010:
            return JobDrmFormat{DRM_FORMAT_XVUY2101010};

        case FormatType::Y210:
            return JobDrmFormat{DRM_FORMAT_Y210};
        case FormatType::Y212:
            return JobDrmFormat{DRM_FORMAT_Y212};
        case FormatType::Y216:
            return JobDrmFormat{DRM_FORMAT_Y216};

        case FormatType::Y410:
            return JobDrmFormat{DRM_FORMAT_Y410};
        case FormatType::Y412:
            return JobDrmFormat{DRM_FORMAT_Y412};
        case FormatType::Y416:
            return JobDrmFormat{DRM_FORMAT_Y416};

        case FormatType::XVYU2101010:
            return JobDrmFormat{DRM_FORMAT_XVYU2101010};
        case FormatType::XVYU12_16161616:
            return JobDrmFormat{DRM_FORMAT_XVYU12_16161616};
        case FormatType::XVYU16161616:
            return JobDrmFormat{DRM_FORMAT_XVYU16161616};

        case FormatType::Y0L0:
            return JobDrmFormat{DRM_FORMAT_Y0L0};
        case FormatType::X0L0:
            return JobDrmFormat{DRM_FORMAT_X0L0};
        case FormatType::Y0L2:
            return JobDrmFormat{DRM_FORMAT_Y0L2};
        case FormatType::X0L2:
            return JobDrmFormat{DRM_FORMAT_X0L2};

        case FormatType::YUV420_8BIT:
            return JobDrmFormat{DRM_FORMAT_YUV420_8BIT};
        case FormatType::YUV420_10BIT:
            return JobDrmFormat{DRM_FORMAT_YUV420_10BIT};

        case FormatType::XRGB8888_A8:
            return JobDrmFormat{DRM_FORMAT_XRGB8888_A8};
        case FormatType::XBGR8888_A8:
            return JobDrmFormat{DRM_FORMAT_XBGR8888_A8};
        case FormatType::RGBX8888_A8:
            return JobDrmFormat{DRM_FORMAT_RGBX8888_A8};
        case FormatType::BGRX8888_A8:
            return JobDrmFormat{DRM_FORMAT_BGRX8888_A8};
        case FormatType::RGB888_A8:
            return JobDrmFormat{DRM_FORMAT_RGB888_A8};
        case FormatType::BGR888_A8:
            return JobDrmFormat{DRM_FORMAT_BGR888_A8};
        case FormatType::RGB565_A8:
            return JobDrmFormat{DRM_FORMAT_RGB565_A8};
        case FormatType::BGR565_A8:
            return JobDrmFormat{DRM_FORMAT_BGR565_A8};

        case FormatType::NV12:
            return JobDrmFormat{DRM_FORMAT_NV12};
        case FormatType::NV21:
            return JobDrmFormat{DRM_FORMAT_NV21};
        case FormatType::NV16:
            return JobDrmFormat{DRM_FORMAT_NV16};
        case FormatType::NV61:
            return JobDrmFormat{DRM_FORMAT_NV61};
        case FormatType::NV24:
            return JobDrmFormat{DRM_FORMAT_NV24};
        case FormatType::NV42:
            return JobDrmFormat{DRM_FORMAT_NV42};

        case FormatType::NV15:
            return JobDrmFormat{DRM_FORMAT_NV15};
        case FormatType::NV20:
            return JobDrmFormat{DRM_FORMAT_NV20};
        case FormatType::NV30:
            return JobDrmFormat{DRM_FORMAT_NV30};

        case FormatType::P210:
            return JobDrmFormat{DRM_FORMAT_P210};
        case FormatType::P010:
            return JobDrmFormat{DRM_FORMAT_P010};
        case FormatType::P012:
            return JobDrmFormat{DRM_FORMAT_P012};
        case FormatType::P016:
            return JobDrmFormat{DRM_FORMAT_P016};
        case FormatType::P030:
            return JobDrmFormat{DRM_FORMAT_P030};
        case FormatType::P230:
            return JobDrmFormat{DRM_FORMAT_P230};

        case FormatType::Q410:
            return JobDrmFormat{DRM_FORMAT_Q410};
        case FormatType::Q401:
            return JobDrmFormat{DRM_FORMAT_Q401};

        case FormatType::T430:
            return JobDrmFormat{DRM_FORMAT_T430};

        case FormatType::S010:
            return JobDrmFormat{DRM_FORMAT_S010};
        case FormatType::S210:
            return JobDrmFormat{DRM_FORMAT_S210};
        case FormatType::S410:
            return JobDrmFormat{DRM_FORMAT_S410};

        case FormatType::S012:
            return JobDrmFormat{DRM_FORMAT_S012};
        case FormatType::S212:
            return JobDrmFormat{DRM_FORMAT_S212};
        case FormatType::S412:
            return JobDrmFormat{DRM_FORMAT_S412};

        case FormatType::S016:
            return JobDrmFormat{DRM_FORMAT_S016};
        case FormatType::S216:
            return JobDrmFormat{DRM_FORMAT_S216};
        case FormatType::S416:
            return JobDrmFormat{DRM_FORMAT_S416};

        case FormatType::YUV410:
            return JobDrmFormat{DRM_FORMAT_YUV410};
        case FormatType::YVU410:
            return JobDrmFormat{DRM_FORMAT_YVU410};
        case FormatType::YUV411:
            return JobDrmFormat{DRM_FORMAT_YUV411};
        case FormatType::YVU411:
            return JobDrmFormat{DRM_FORMAT_YVU411};
        case FormatType::YUV420:
            return JobDrmFormat{DRM_FORMAT_YUV420};
        case FormatType::YVU420:
            return JobDrmFormat{DRM_FORMAT_YVU420};
        case FormatType::YUV422:
            return JobDrmFormat{DRM_FORMAT_YUV422};
        case FormatType::YVU422:
            return JobDrmFormat{DRM_FORMAT_YVU422};
        case FormatType::YUV444:
            return JobDrmFormat{DRM_FORMAT_YUV444};
        case FormatType::YVU444:
            return JobDrmFormat{DRM_FORMAT_YVU444};

        case FormatType::Y8:
            return JobDrmFormat{DRM_FORMAT_Y8};
        case FormatType::XYYY2101010:
            return JobDrmFormat{DRM_FORMAT_XYYY2101010};

        }

        return {};
    }


    [[nodiscard]] friend constexpr bool operator==(const JobDrmFormat &, const JobDrmFormat &) noexcept = default;

private:
    std::uint32_t m_value{DRM_FORMAT_INVALID};
};

}