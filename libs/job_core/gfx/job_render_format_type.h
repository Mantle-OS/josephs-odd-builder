#pragma once

#include <cstdint>

namespace job::core {

enum class FormatType : std::uint16_t
{
    Unknown,

    C1,
    C2,
    C4,
    C8,

    D1,
    D2,
    D4,
    D8,

    R1,
    R2,
    R4,
    R8,
    R10,
    R12,
    R16,

    RG88,
    GR88,

    RG1616,
    GR1616,

    RGB332,
    BGR233,

    XRGB4444,
    XBGR4444,
    RGBX4444,
    BGRX4444,

    ARGB4444,
    ABGR4444,
    RGBA4444,
    BGRA4444,

    XRGB1555,
    XBGR1555,
    RGBX5551,
    BGRX5551,

    ARGB1555,
    ABGR1555,
    RGBA5551,
    BGRA5551,

    RGB565,
    BGR565,

    RGB888,
    BGR888,

    XRGB8888,
    XBGR8888,
    RGBX8888,
    BGRX8888,

    ARGB8888,
    ABGR8888,
    RGBA8888,
    BGRA8888,

    XRGB2101010,
    XBGR2101010,
    RGBX1010102,
    BGRX1010102,

    ARGB2101010,
    ABGR2101010,
    RGBA1010102,
    BGRA1010102,

    RGB161616,
    BGR161616,

    XRGB16161616,
    XBGR16161616,
    ARGB16161616,
    ABGR16161616,

    XRGB16161616F,
    XBGR16161616F,
    ARGB16161616F,
    ABGR16161616F,

    R16F,
    GR1616F,
    BGR161616F,

    R32F,
    GR3232F,
    BGR323232F,
    ABGR32323232F,

    AXBXGXRX106106106106,

    YUYV,
    YVYU,
    UYVY,
    VYUY,

    AYUV,
    AVUY8888,
    XYUV8888,
    XVUY8888,
    VUY888,
    VUY101010,
    XVUY2101010,

    Y210,
    Y212,
    Y216,

    Y410,
    Y412,
    Y416,

    XVYU2101010,
    XVYU12_16161616,
    XVYU16161616,

    Y0L0,
    X0L0,
    Y0L2,
    X0L2,

    YUV420_8BIT,
    YUV420_10BIT,

    XRGB8888_A8,
    XBGR8888_A8,
    RGBX8888_A8,
    BGRX8888_A8,
    RGB888_A8,
    BGR888_A8,
    RGB565_A8,
    BGR565_A8,

    NV12,
    NV21,
    NV16,
    NV61,
    NV24,
    NV42,

    NV15,
    NV20,
    NV30,

    P210,
    P010,
    P012,
    P016,
    P030,
    P230,

    Q410,
    Q401,

    T430,

    S010,
    S210,
    S410,

    S012,
    S212,
    S412,

    S016,
    S216,
    S416,

    YUV410,
    YVU410,
    YUV411,
    YVU411,
    YUV420,
    YVU420,
    YUV422,
    YVU422,
    YUV444,
    YVU444,

    Y8,
    XYYY2101010
};
}