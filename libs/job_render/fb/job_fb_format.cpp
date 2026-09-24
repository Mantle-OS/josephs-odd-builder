#include "job_fb_format.h"

#include <linux/videodev2.h>

namespace job::render {

JobFbFormatResult JobFbFormat::detect(const JobFbFixedInfo &fixed, const JobFbVariableInfo &variable) noexcept
{
    if (!fixed.valid() || !variable.valid())
        return {};

    if (fixed.type().fourCc() || fixed.visual().fourCc())
        return detectFourCc(fixed, variable);

    return detectLegacy(fixed, variable);
}

FormatType JobFbFormat::fromFourCc(FourCc fourCc) noexcept
{
    switch (fourCc) {
    case V4L2_PIX_FMT_RGB565:
        return FormatType::RGB565;
    case V4L2_PIX_FMT_RGB24:
        return FormatType::RGB888;
    case V4L2_PIX_FMT_BGR24:
        return FormatType::BGR888;
    case V4L2_PIX_FMT_XRGB32:
        return FormatType::XRGB8888;
    case V4L2_PIX_FMT_XBGR32:
        return FormatType::XBGR8888;
    case V4L2_PIX_FMT_RGBX32:
        return FormatType::RGBX8888;
    case V4L2_PIX_FMT_BGRX32:
        return FormatType::BGRX8888;
    case V4L2_PIX_FMT_ARGB32:
        return FormatType::ARGB8888;
    case V4L2_PIX_FMT_ABGR32:
        return FormatType::ABGR8888;
    case V4L2_PIX_FMT_RGBA32:
        return FormatType::RGBA8888;
    case V4L2_PIX_FMT_BGRA32:
        return FormatType::BGRA8888;
    case V4L2_PIX_FMT_GREY:
        return FormatType::Y8;
    case V4L2_PIX_FMT_YUYV:
        return FormatType::YUYV;
    case V4L2_PIX_FMT_YVYU:
        return FormatType::YVYU;
    case V4L2_PIX_FMT_UYVY:
        return FormatType::UYVY;
    case V4L2_PIX_FMT_VYUY:
        return FormatType::VYUY;
    case V4L2_PIX_FMT_AYUV32:
        return FormatType::AYUV;
    default:
        return FormatType::Unknown;
    }
}

JobFbFormatResult JobFbFormat::detectFourCc(const JobFbFixedInfo &fixed, const JobFbVariableInfo &variable) noexcept
{
    if (!fixed.capabilities().fourCc() || !fixed.type().fourCc() || !fixed.visual().fourCc() || !variable.fourCcMode())
        return {};

    const FourCc fourCc = variable.fourCc();
    const FormatType format = fromFourCc(fourCc);
    if (format == FormatType::Unknown)
        return {};

    return JobFbFormatResult{
        .format = format,
        .source = FbFormatSource::FourCc,
        .fourCc = fourCc
    };
}

JobFbFormatResult JobFbFormat::detectLegacy(const JobFbFixedInfo &fixed, const JobFbVariableInfo &variable) noexcept
{
    if (!fixed.type().packedPixels())
        return {};

    FormatType format = FormatType::Unknown;

    if (fixed.visual().trueColor() || fixed.visual().directColor())
        format = detectTrueColor( variable);
    else if (fixed.visual().monochrome())
        format = detectMonochrome(fixed, variable);
    else if (fixed.visual().pseudoColor() || fixed.visual().staticPseudoColor())
        format = detectPseudoColor( fixed, variable);

    if (format == FormatType::Unknown)
        return {};

    return JobFbFormatResult{
        .format = format,
        .source = FbFormatSource::Legacy,
        .fourCc = 0
    };
}

FormatType JobFbFormat::detectTrueColor(const JobFbVariableInfo &variable) noexcept
{
    const JobFbBitfield &red    = variable.red();
    const JobFbBitfield &green  = variable.green();
    const JobFbBitfield &blue   = variable.blue();
    const JobFbBitfield &alpha  = variable.alpha();

    if (!red.valid() || !green.valid() || !blue.valid() || !alpha.valid())
        return FormatType::Unknown;

    if (red.msbRight || green.msbRight || blue.msbRight || alpha.msbRight)
        return FormatType::Unknown;

    if (variable.bitsPerPixel() == 16 &&
        red.offset == 11 && red.length == 5 &&
        green.offset == 5 && green.length == 6 &&
        blue.offset == 0 && blue.length == 5 &&
        alpha.length == 0) {
        return FormatType::RGB565;
    }

    if (variable.bitsPerPixel() == 16 &&
        red.offset == 0 && red.length == 5 &&
        green.offset == 5 && green.length == 6 &&
        blue.offset == 11 && blue.length == 5 &&
        alpha.length == 0) {
        return FormatType::BGR565;
    }

    if (variable.bitsPerPixel() == 24 &&
        red.offset == 16 && red.length == 8 &&
        green.offset == 8 && green.length == 8 &&
        blue.offset == 0 && blue.length == 8 &&
        alpha.length == 0) {
        return FormatType::RGB888;
    }

    if (variable.bitsPerPixel() == 24 &&
        red.offset == 0 && red.length == 8 &&
        green.offset == 8 && green.length == 8 &&
        blue.offset == 16 && blue.length == 8 &&
        alpha.length == 0) {
        return FormatType::BGR888;
    }

    if (variable.bitsPerPixel() == 32 &&
        red.offset == 16 && red.length == 8 &&
        green.offset == 8 && green.length == 8 &&
        blue.offset == 0 && blue.length == 8) {
        if (alpha.length == 0)
            return FormatType::XRGB8888;

        if (alpha.offset == 24 &&
            alpha.length == 8) {
            return FormatType::ARGB8888;
        }

        return FormatType::Unknown;
    }

    if (variable.bitsPerPixel() == 32 &&
        red.offset == 0 && red.length == 8 &&
        green.offset == 8 && green.length == 8 &&
        blue.offset == 16 && blue.length == 8) {
        if (alpha.length == 0)
            return FormatType::XBGR8888;

        if (alpha.offset == 24 &&
            alpha.length == 8) {
            return FormatType::ABGR8888;
        }

        return FormatType::Unknown;
    }

    return FormatType::Unknown;
}

FormatType JobFbFormat::detectMonochrome(const JobFbFixedInfo &fixed, const JobFbVariableInfo &variable) noexcept
{
    if (!fixed.visual().monochrome())
        return FormatType::Unknown;

    if (variable.bitsPerPixel() != 1)
        return FormatType::Unknown;

    return FormatType::C1;
}

FormatType JobFbFormat::detectPseudoColor(const JobFbFixedInfo &fixed, const JobFbVariableInfo &variable) noexcept
{
    if (!fixed.visual().pseudoColor() && !fixed.visual().staticPseudoColor())
        return FormatType::Unknown;

    switch (variable.bitsPerPixel()) {
    case 1:
        return FormatType::C1;
    case 2:
        return FormatType::C2;
    case 4:
        return FormatType::C4;
    case 8:
        return FormatType::C8;
    default:
        return FormatType::Unknown;
    }
}

}