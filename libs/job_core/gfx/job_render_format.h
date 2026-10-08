#pragma once

#include <cstddef>
#include <cstdint>
#include <array>

#include <endian_utils.h>
#include "job_render_format_type.h"
#include "job_color_channel_info.h"

namespace job::core {

class JobRenderFormat
{
public:
    // kMaximumChannelCount is in job_color_channel_info.h
    using Channels = std::array<JobColorChannelInfo, kMaximumChannelCount>;

    struct Info
    {
        std::uint8_t        planeCount{};
        std::uint16_t       bitsPerPixel{};
        std::uint8_t        horizontalSubsampling{};
        std::uint8_t        verticalSubsampling{};
        bool                alpha{};
        bool                greyscale{};
        bool                yuv{};
        bool                floatingPoint{};
        bool                colorIndexed{};
        core::EndianType    endianType{core::EndianType::Native};
        Channels            channels{};
    };

    JobRenderFormat() = delete;
    ~JobRenderFormat() = delete;

    JobRenderFormat(const JobRenderFormat &) = delete;
    JobRenderFormat &operator=(const JobRenderFormat &) = delete;
    JobRenderFormat(JobRenderFormat &&) = delete;
    JobRenderFormat &operator=(JobRenderFormat &&) = delete;

    [[nodiscard]] static constexpr bool valid(FormatType format) noexcept
    {
        return format != FormatType::Unknown;
    }


    [[nodiscard]] static constexpr Info info(FormatType format) noexcept
    {
        switch (format) {
        case FormatType::C1:
            return makeInfo(1, 1, false, false, false, false, true, 1, 1, core::EndianType::Native);
        case FormatType::C2:
            return makeInfo(1, 2, false, false, false, false, true, 1, 1, core::EndianType::Native);
        case FormatType::C4:
            return makeInfo(1, 4, false, false, false, false, true, 1, 1, core::EndianType::Native);
        case FormatType::C8:
            return makeInfo(1, 8, false, false, false, false, true, 1, 1, core::EndianType::Native);

        case FormatType::D1:
            return makeInfo(1, 1, false, true, false, false, false, 1, 1, core::EndianType::Native);
        case FormatType::D2:
            return makeInfo(1, 2, false, true, false, false, false, 1, 1, core::EndianType::Native);
        case FormatType::D4:
            return makeInfo(1, 4, false, true, false, false, false, 1, 1, core::EndianType::Native);
        case FormatType::D8:
            return makeInfo(1, 8, false, true, false, false, false, 1, 1, core::EndianType::Native);

        case FormatType::R1:
            return makeInfo(1, 1, false, false, false, false, false, 1, 1, core::EndianType::Native,
                            makeChannels({JobColorChannel::Red, 1, 0}));

        case FormatType::R2:
            return makeInfo(1, 2, false, false, false, false, false, 1, 1, core::EndianType::Native,
                            makeChannels({JobColorChannel::Red, 2, 0}));

        case FormatType::R4:
            return makeInfo(1, 4, false, false, false, false, false, 1, 1, core::EndianType::Native,
                            makeChannels({JobColorChannel::Red, 4, 0}));

        case FormatType::R8:
            return makeInfo(1, 8, false, false, false, false, false, 1, 1, core::EndianType::Native,
                            makeChannels({JobColorChannel::Red, 8, 0}));

        case FormatType::R10:
            return makeInfo(1, 16, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Red, 10, 0}));

        case FormatType::R12:
            return makeInfo(1, 16, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Red, 12, 0}));

        case FormatType::R16:
            return makeInfo(1, 16, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Red, 16, 0}));

        case FormatType::RG88:
            return makeInfo(1, 16, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Red, 8, 8},
                                         {JobColorChannel::Green, 8, 0}));

        case FormatType::GR88:
            return makeInfo(1, 16, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Green, 8, 8},
                                         {JobColorChannel::Red, 8, 0}));

        case FormatType::RG1616:
            return makeInfo(1, 32, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Red, 16, 16},
                                         {JobColorChannel::Green, 16, 0}));

        case FormatType::GR1616:
            return makeInfo(1, 32, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Green, 16, 16},
                                         {JobColorChannel::Red, 16, 0}));

        case FormatType::RGB332:
            return makeInfo(1, 8, false, false, false, false, false, 1, 1, core::EndianType::Native,
                            makeChannels({JobColorChannel::Red, 3, 5},
                                         {JobColorChannel::Green, 3, 2},
                                         {JobColorChannel::Blue, 2, 0}));

        case FormatType::BGR233:
            return makeInfo(1, 8, false, false, false, false, false, 1, 1, core::EndianType::Native,
                            makeChannels({JobColorChannel::Blue, 2, 6},
                                         {JobColorChannel::Green, 3, 3},
                                         {JobColorChannel::Red, 3, 0}));

        case FormatType::XRGB4444:
            return makeInfo(1, 16, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Red, 4, 8},
                                         {JobColorChannel::Green, 4, 4},
                                         {JobColorChannel::Blue, 4, 0}));

        case FormatType::XBGR4444:
            return makeInfo(1, 16, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Blue, 4, 8},
                                         {JobColorChannel::Green, 4, 4},
                                         {JobColorChannel::Red, 4, 0}));

        case FormatType::RGBX4444:
            return makeInfo(1, 16, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Red, 4, 12},
                                         {JobColorChannel::Green, 4, 8},
                                         {JobColorChannel::Blue, 4, 4}));

        case FormatType::BGRX4444:
            return makeInfo(1, 16, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels( {JobColorChannel::Blue, 4, 12},
                                         {JobColorChannel::Green, 4, 8},
                                         {JobColorChannel::Red, 4, 4}));

        case FormatType::ARGB4444:
            return makeInfo(1, 16, true, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Alpha, 4, 12},
                                         {JobColorChannel::Red, 4, 8},
                                         {JobColorChannel::Green, 4, 4},
                                         {JobColorChannel::Blue, 4, 0}));

        case FormatType::ABGR4444:
            return makeInfo(1, 16, true, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Alpha, 4, 12},
                                         {JobColorChannel::Blue, 4, 8},
                                         {JobColorChannel::Green, 4, 4},
                                         {JobColorChannel::Red, 4, 0}));

        case FormatType::RGBA4444:
            return makeInfo(1, 16, true, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Red, 4, 12},
                                         {JobColorChannel::Green, 4, 8},
                                         {JobColorChannel::Blue, 4, 4},
                                         {JobColorChannel::Alpha, 4, 0}));

        case FormatType::BGRA4444:
            return makeInfo(1, 16, true, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Blue, 4, 12},
                                         {JobColorChannel::Green, 4, 8},
                                         {JobColorChannel::Red, 4, 4},
                                         {JobColorChannel::Alpha, 4, 0}));

        case FormatType::XRGB1555:
            return makeInfo(1, 16, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Red, 5, 10},
                                         {JobColorChannel::Green, 5, 5},
                                         {JobColorChannel::Blue, 5, 0}));

        case FormatType::XBGR1555:
            return makeInfo(1, 16, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Blue, 5, 10},
                                         {JobColorChannel::Green, 5, 5},
                                         {JobColorChannel::Red, 5, 0}));

        case FormatType::RGBX5551:
            return makeInfo(1, 16, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Red, 5, 11},
                                         {JobColorChannel::Green, 5, 6},
                                         {JobColorChannel::Blue, 5, 1}));

        case FormatType::BGRX5551:
            return makeInfo(1, 16, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Blue, 5, 11},
                                         {JobColorChannel::Green, 5, 6},
                                         {JobColorChannel::Red, 5, 1}));

        case FormatType::ARGB1555:
            return makeInfo(1, 16, true, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Alpha, 1, 15},
                                         {JobColorChannel::Red, 5, 10},
                                         {JobColorChannel::Green, 5, 5},
                                         {JobColorChannel::Blue, 5, 0}));

        case FormatType::ABGR1555:
            return makeInfo(1, 16, true, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Alpha, 1, 15},
                                         {JobColorChannel::Blue, 5, 10},
                                         {JobColorChannel::Green, 5, 5},
                                         {JobColorChannel::Red, 5, 0}));

        case FormatType::RGBA5551:
            return makeInfo(1, 16, true, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Red, 5, 11},
                                         {JobColorChannel::Green, 5, 6},
                                         {JobColorChannel::Blue, 5, 1},
                                         {JobColorChannel::Alpha, 1, 0}));

        case FormatType::BGRA5551:
            return makeInfo(1, 16, true, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Blue, 5, 11},
                                         {JobColorChannel::Green, 5, 6},
                                         {JobColorChannel::Red, 5, 1},
                                         {JobColorChannel::Alpha, 1, 0}));

        case FormatType::RGB565:
            return makeInfo(1, 16, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Red, 5, 11},
                                         {JobColorChannel::Green, 6, 5},
                                         {JobColorChannel::Blue, 5, 0}));

        case FormatType::BGR565:
            return makeInfo(1, 16, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Blue, 5, 11},
                                         {JobColorChannel::Green, 6, 5},
                                         {JobColorChannel::Red, 5, 0}));

        case FormatType::RGB888:
            return makeInfo(1, 24, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Red, 8, 16},
                                         {JobColorChannel::Green, 8, 8},
                                         {JobColorChannel::Blue, 8, 0}));

        case FormatType::BGR888:
            return makeInfo(1, 24, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Blue, 8, 16},
                                         {JobColorChannel::Green, 8, 8},
                                         {JobColorChannel::Red, 8, 0}));

        case FormatType::XRGB8888:
            return makeInfo(1, 32, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Red, 8, 16},
                                         {JobColorChannel::Green, 8, 8},
                                         {JobColorChannel::Blue, 8, 0}));

        case FormatType::XBGR8888:
            return makeInfo(1, 32, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Blue, 8, 16},
                                         {JobColorChannel::Green, 8, 8},
                                         {JobColorChannel::Red, 8, 0}));

        case FormatType::RGBX8888:
            return makeInfo(1, 32, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Red, 8, 24},
                                         {JobColorChannel::Green, 8, 16},
                                         {JobColorChannel::Blue, 8, 8}));

        case FormatType::BGRX8888:
            return makeInfo(1, 32, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Blue, 8, 24},
                                         {JobColorChannel::Green, 8, 16},
                                         {JobColorChannel::Red, 8, 8}));

        case FormatType::ARGB8888:
            return makeInfo(1, 32, true, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Alpha, 8, 24},
                                         {JobColorChannel::Red, 8, 16},
                                         {JobColorChannel::Green, 8, 8},
                                         {JobColorChannel::Blue, 8, 0}));

        case FormatType::ABGR8888:
            return makeInfo(1, 32, true, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Alpha, 8, 24},
                                         {JobColorChannel::Blue, 8, 16},
                                         {JobColorChannel::Green, 8, 8},
                                         {JobColorChannel::Red, 8, 0}));

        case FormatType::RGBA8888:
            return makeInfo(1, 32, true, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Red, 8, 24},
                                         {JobColorChannel::Green, 8, 16},
                                         {JobColorChannel::Blue, 8, 8},
                                         {JobColorChannel::Alpha, 8, 0}));

        case FormatType::BGRA8888:
            return makeInfo(1, 32, true, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Blue, 8, 24},
                                         {JobColorChannel::Green, 8, 16},
                                         {JobColorChannel::Red, 8, 8},
                                         {JobColorChannel::Alpha, 8, 0}));

        case FormatType::XRGB2101010:
            return makeInfo(1, 32, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Red, 10, 20},
                                         {JobColorChannel::Green, 10, 10},
                                         {JobColorChannel::Blue, 10, 0}));

        case FormatType::XBGR2101010:
            return makeInfo(1, 32, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Blue, 10, 20},
                                         {JobColorChannel::Green, 10, 10},
                                         {JobColorChannel::Red, 10, 0}));

        case FormatType::RGBX1010102:
            return makeInfo(1, 32, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Red, 10, 22},
                                         {JobColorChannel::Green, 10, 12},
                                         {JobColorChannel::Blue, 10, 2}));

        case FormatType::BGRX1010102:
            return makeInfo(1, 32, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Blue, 10, 22},
                                         {JobColorChannel::Green, 10, 12},
                                         {JobColorChannel::Red, 10, 2}));

        case FormatType::ARGB2101010:
            return makeInfo(1, 32, true, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Alpha, 2, 30},
                                         {JobColorChannel::Red, 10, 20},
                                         {JobColorChannel::Green, 10, 10},
                                         {JobColorChannel::Blue, 10, 0}));

        case FormatType::ABGR2101010:
            return makeInfo(1, 32, true, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Alpha, 2, 30},
                                         {JobColorChannel::Blue, 10, 20},
                                         {JobColorChannel::Green, 10, 10},
                                         {JobColorChannel::Red, 10, 0}));

        case FormatType::RGBA1010102:
            return makeInfo(1, 32, true, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Red, 10, 22},
                                         {JobColorChannel::Green, 10, 12},
                                         {JobColorChannel::Blue, 10, 2},
                                         {JobColorChannel::Alpha, 2, 0}));

        case FormatType::BGRA1010102:
            return makeInfo(1, 32, true, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Blue, 10, 22},
                                         {JobColorChannel::Green, 10, 12},
                                         {JobColorChannel::Red, 10, 2},
                                         {JobColorChannel::Alpha, 2, 0}));

        case FormatType::RGB161616:
            return makeInfo(1, 48, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Red, 16, 32},
                                         {JobColorChannel::Green, 16, 16},
                                         {JobColorChannel::Blue, 16, 0}));

        case FormatType::BGR161616:
            return makeInfo(1, 48, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Blue, 16, 32},
                                         {JobColorChannel::Green, 16, 16},
                                         {JobColorChannel::Red, 16, 0}));

        case FormatType::XRGB16161616:
            return makeInfo(1, 64, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Red, 16, 32},
                                         {JobColorChannel::Green, 16, 16},
                                         {JobColorChannel::Blue, 16, 0}));

        case FormatType::XBGR16161616:
            return makeInfo(1, 64, false, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Blue, 16, 32},
                                         {JobColorChannel::Green, 16, 16},
                                         {JobColorChannel::Red, 16, 0}));

        case FormatType::ARGB16161616:
            return makeInfo(1, 64, true, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Alpha, 16, 48},
                                         {JobColorChannel::Red, 16, 32},
                                         {JobColorChannel::Green, 16, 16},
                                         {JobColorChannel::Blue, 16, 0}));

        case FormatType::ABGR16161616:
            return makeInfo(1, 64, true, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Alpha, 16, 48},
                                         {JobColorChannel::Blue, 16, 32},
                                         {JobColorChannel::Green, 16, 16},
                                         {JobColorChannel::Red, 16, 0}));

        case FormatType::XRGB16161616F:
            return makeInfo(1, 64, false, false, false, true, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Red, 16, 32},
                                         {JobColorChannel::Green, 16, 16},
                                         {JobColorChannel::Blue, 16, 0}));

        case FormatType::XBGR16161616F:
            return makeInfo(1, 64, false, false, false, true, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Blue, 16, 32},
                                         {JobColorChannel::Green, 16, 16},
                                         {JobColorChannel::Red, 16, 0}));

        case FormatType::ARGB16161616F:
            return makeInfo(1, 64, true, false, false, true, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Alpha, 16, 48},
                                         {JobColorChannel::Red, 16, 32},
                                         {JobColorChannel::Green, 16, 16},
                                         {JobColorChannel::Blue, 16, 0}));

        case FormatType::ABGR16161616F:
            return makeInfo(1, 64, true, false, false, true, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Alpha, 16, 48},
                                         {JobColorChannel::Blue, 16, 32},
                                         {JobColorChannel::Green, 16, 16},
                                         {JobColorChannel::Red, 16, 0}));

        case FormatType::R16F:
            return makeInfo(1, 16, false, false, false, true, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Red, 16, 0}));

        case FormatType::GR1616F:
            return makeInfo(1, 32, false, false, false, true, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Green, 16, 16},
                                         {JobColorChannel::Red, 16, 0}));

        case FormatType::BGR161616F:
            return makeInfo(1, 48, false, false, false, true, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Blue, 16, 32},
                                         {JobColorChannel::Green, 16, 16},
                                         {JobColorChannel::Red, 16, 0}));

        case FormatType::R32F:
            return makeInfo(1, 32, false, false, false, true, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Red, 32, 0}));

        case FormatType::GR3232F:
            return makeInfo(1, 64, false, false, false, true, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Green, 32, 32},
                                         {JobColorChannel::Red, 32, 0}));

        case FormatType::BGR323232F:
            return makeInfo(1, 96, false, false, false, true, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Blue, 32, 64},
                                         {JobColorChannel::Green, 32, 32},
                                         {JobColorChannel::Red, 32, 0}));

        case FormatType::ABGR32323232F:
            return makeInfo(1, 128, true, false, false, true, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Alpha, 32, 96},
                                         {JobColorChannel::Blue, 32, 64},
                                         {JobColorChannel::Green, 32, 32},
                                         {JobColorChannel::Red, 32, 0}));

        case FormatType::AXBXGXRX106106106106:
            return makeInfo(1, 64, true, false, false, false, false, 1, 1, core::EndianType::Little,
                            makeChannels({JobColorChannel::Alpha, 10, 54},
                                         {JobColorChannel::Blue, 10, 38},
                                         {JobColorChannel::Green, 10, 22},
                                         {JobColorChannel::Red, 10, 6}));

        case FormatType::YUYV:
        case FormatType::YVYU:
        case FormatType::UYVY:
        case FormatType::VYUY:
            return makeInfo(1, 16, false, false, true, false, false, 2, 1, core::EndianType::Little);

        case FormatType::AYUV:
        case FormatType::AVUY8888:
            return makeInfo(1, 32, true, false, true, false, false, 1, 1, core::EndianType::Little);

        case FormatType::XYUV8888:
        case FormatType::XVUY8888:
            return makeInfo(1, 32, false, false, true, false, false, 1, 1, core::EndianType::Little);

        case FormatType::VUY888:
            return makeInfo(1, 24, false, false, true, false, false, 1, 1, core::EndianType::Little);

        case FormatType::VUY101010:
            return makeInfo(1, 30, false, false, true, false, false, 1, 1, core::EndianType::Native);

        case FormatType::XVUY2101010:
            return makeInfo(1, 32, false, false, true, false, false, 1, 1, core::EndianType::Little);

        case FormatType::Y210:
        case FormatType::Y212:
        case FormatType::Y216:
            return makeInfo(1, 32, false, false, true, false, false, 2, 1, core::EndianType::Little);

        case FormatType::Y410:
            return makeInfo(1, 32, true, false, true, false, false, 1, 1, core::EndianType::Little);

        case FormatType::Y412:
        case FormatType::Y416:
            return makeInfo(1, 64, true, false, true, false, false, 1, 1, core::EndianType::Little);

        case FormatType::XVYU2101010:
            return makeInfo(1, 32, false, false, true, false, false, 1, 1, core::EndianType::Little);

        case FormatType::XVYU12_16161616:
        case FormatType::XVYU16161616:
            return makeInfo(1, 64, false, false, true, false, false, 1, 1, core::EndianType::Little);

        case FormatType::Y0L0:
            return makeInfo(1, 16, true, false, true, false, false, 2, 2, core::EndianType::Little);

        case FormatType::X0L0:
            return makeInfo(1, 16, false, false, true, false, false, 2, 2, core::EndianType::Little);

        case FormatType::Y0L2:
            return makeInfo(1, 16, true, false, true, false, false, 2, 2, core::EndianType::Little);

        case FormatType::X0L2:
            return makeInfo(1, 16, false, false, true, false, false, 2, 2, core::EndianType::Little);

        case FormatType::YUV420_8BIT:
            return makeInfo(1, 12, false, false, true, false, false, 2, 2, core::EndianType::Native);

        case FormatType::YUV420_10BIT:
            return makeInfo(1, 15, false, false, true, false, false, 2, 2, core::EndianType::Native);

        case FormatType::XRGB8888_A8:
        case FormatType::XBGR8888_A8:
        case FormatType::RGBX8888_A8:
        case FormatType::BGRX8888_A8:
            return makeInfo(2, 40, true, false, false, false, false, 1, 1, core::EndianType::Little);

        case FormatType::RGB888_A8:
        case FormatType::BGR888_A8:
            return makeInfo(2, 32, true, false, false, false, false, 1, 1, core::EndianType::Little);

        case FormatType::RGB565_A8:
        case FormatType::BGR565_A8:
            return makeInfo(2, 24, true, false, false, false, false, 1, 1, core::EndianType::Little);

        case FormatType::NV12:
        case FormatType::NV21:
            return makeInfo(2, 12, false, false, true, false, false, 2, 2, core::EndianType::Little);

        case FormatType::NV16:
        case FormatType::NV61:
            return makeInfo(2, 16, false, false, true, false, false, 2, 1, core::EndianType::Little);

        case FormatType::NV24:
        case FormatType::NV42:
            return makeInfo(2, 24, false, false, true, false, false, 1, 1, core::EndianType::Little);

        case FormatType::NV15:
            return makeInfo(2, 15, false, false, true, false, false, 2, 2, core::EndianType::Little);

        case FormatType::NV20:
            return makeInfo(2, 20, false, false, true, false, false, 2, 1, core::EndianType::Little);

        case FormatType::NV30:
            return makeInfo(2, 30, false, false, true, false, false, 1, 1, core::EndianType::Little);

        case FormatType::P210:
            return makeInfo(2, 20, false, false, true, false, false, 2, 1, core::EndianType::Little);

        case FormatType::P010:
            return makeInfo(2, 15, false, false, true, false, false, 2, 2, core::EndianType::Little);

        case FormatType::P012:
            return makeInfo(2, 18, false, false, true, false, false, 2, 2, core::EndianType::Little);

        case FormatType::P016:
            return makeInfo(2, 24, false, false, true, false, false, 2, 2, core::EndianType::Little);

        case FormatType::P030:
            return makeInfo(2, 15, false, false, true, false, false, 2, 2, core::EndianType::Little);

        case FormatType::P230:
            return makeInfo(2, 20, false, false, true, false, false, 2, 1, core::EndianType::Little);

        case FormatType::Q410:
        case FormatType::Q401:
            return makeInfo(3, 30, false, false, true, false, false, 1, 1, core::EndianType::Little);

        case FormatType::T430:
            return makeInfo(3, 30, false, false, true, false, false, 1, 1, core::EndianType::Little);

        case FormatType::S010:
            return makeInfo(3, 15, false, false, true, false, false, 2, 2, core::EndianType::Little);

        case FormatType::S210:
            return makeInfo(3, 20, false, false, true, false, false, 2, 1, core::EndianType::Little);

        case FormatType::S410:
            return makeInfo(3, 30, false, false, true, false, false, 1, 1, core::EndianType::Little);

        case FormatType::S012:
            return makeInfo(3, 18, false, false, true, false, false, 2, 2, core::EndianType::Little);

        case FormatType::S212:
            return makeInfo(3, 24, false, false, true, false, false, 2, 1, core::EndianType::Little);

        case FormatType::S412:
            return makeInfo(3, 36, false, false, true, false, false, 1, 1, core::EndianType::Little);

        case FormatType::S016:
            return makeInfo(3, 24, false, false, true, false, false, 2, 2, core::EndianType::Little);

        case FormatType::S216:
            return makeInfo(3, 32, false, false, true, false, false, 2, 1, core::EndianType::Little);

        case FormatType::S416:
            return makeInfo(3, 48, false, false, true, false, false, 1, 1, core::EndianType::Little);

        case FormatType::YUV410:
        case FormatType::YVU410:
            return makeInfo(3, 9, false, false, true, false, false, 4, 4, core::EndianType::Native);

        case FormatType::YUV411:
        case FormatType::YVU411:
            return makeInfo(3, 12, false, false, true, false, false, 4, 1, core::EndianType::Native);

        case FormatType::YUV420:
        case FormatType::YVU420:
            return makeInfo(3, 12, false, false, true, false, false, 2, 2, core::EndianType::Native);

        case FormatType::YUV422:
        case FormatType::YVU422:
            return makeInfo(3, 16, false, false, true, false, false, 2, 1, core::EndianType::Native);

        case FormatType::YUV444:
        case FormatType::YVU444:
            return makeInfo(3, 24, false, false, true, false, false, 1, 1, core::EndianType::Native);

        case FormatType::Y8:
            return makeInfo(1, 8, false, true, true, false, false, 1, 1, core::EndianType::Native);

        case FormatType::XYYY2101010:
            return makeInfo(1, 32, false, true, true, false, false, 1, 1, core::EndianType::Little);

        case FormatType::Unknown:
            break;
        }

        return {};
    }
    [[nodiscard]] static constexpr std::size_t planeCount(FormatType format) noexcept
    {
        return info(format).planeCount;
    }

    [[nodiscard]] static constexpr std::size_t bitsPerPixel(FormatType format) noexcept
    {
        return info(format).bitsPerPixel;
    }

    [[nodiscard]] static constexpr bool byteAligned(FormatType format) noexcept
    {
        const auto bits = bitsPerPixel(format);
        return bits != 0 && (bits % 8) == 0;
    }

    [[nodiscard]] static constexpr std::size_t bytesPerPixel(FormatType format) noexcept
    {
        const auto formatInfo = info(format);
        if (formatInfo.planeCount != 1 || formatInfo.bitsPerPixel == 0 || (formatInfo.bitsPerPixel % 8) != 0)
            return 0;

        return formatInfo.bitsPerPixel / 8;
    }

    [[nodiscard]] static constexpr bool alpha(FormatType format) noexcept
    {
        return info(format).alpha;
    }

    [[nodiscard]] static constexpr bool greyscale(FormatType format) noexcept
    {
        return info(format).greyscale;
    }

    [[nodiscard]] static constexpr bool yuv(FormatType format) noexcept
    {
        return info(format).yuv;
    }

    [[nodiscard]] static constexpr bool floatingPoint(FormatType format) noexcept
    {
        return info(format).floatingPoint;
    }

    [[nodiscard]] static constexpr bool colorIndexed(FormatType format) noexcept
    {
        return info(format).colorIndexed;
    }

    [[nodiscard]] static constexpr std::size_t horizontalSubsampling(FormatType format) noexcept
    {
        return info(format).horizontalSubsampling;
    }

    [[nodiscard]] static constexpr std::size_t verticalSubsampling(FormatType format) noexcept
    {
        return info(format).verticalSubsampling;
    }

    [[nodiscard]] static constexpr Channels channels(FormatType format) noexcept
    {
        return info(format).channels;
    }

private:
    [[nodiscard]] static constexpr Channels makeChannels(JobColorChannelInfo c0 = {},
                                                         JobColorChannelInfo c1 = {},
                                                         JobColorChannelInfo c2 = {},
                                                         JobColorChannelInfo c3 = {}) noexcept
    {
        return {c0, c1, c2, c3};
    }

    [[nodiscard]] static constexpr Info makeInfo(std::uint8_t planeCount,
                                                 std::uint16_t bitsPerPixel,
                                                 bool alpha,
                                                 bool greyscale,
                                                 bool yuv,
                                                 bool floatingPoint,
                                                 bool colorIndexed,
                                                 std::uint8_t horizontalSubsampling,
                                                 std::uint8_t verticalSubsampling,
                                                 core::EndianType endianType,
                                                 Channels channels = {}) noexcept
    {
        return {
            .planeCount = planeCount,
            .bitsPerPixel = bitsPerPixel,
            .horizontalSubsampling = horizontalSubsampling,
            .verticalSubsampling = verticalSubsampling,
            .alpha = alpha,
            .greyscale = greyscale,
            .yuv = yuv,
            .floatingPoint = floatingPoint,
            .colorIndexed = colorIndexed,
            .endianType = endianType,
            .channels = channels
        };
    }
};

}