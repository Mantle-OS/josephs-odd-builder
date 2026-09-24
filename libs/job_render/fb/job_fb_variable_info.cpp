#include "job_fb_variable_info.h"

namespace job::render {

JobFbVariableInfo JobFbVariableInfo::fromFb(const fb_var_screeninfo &info) noexcept
{
    JobFbVariableInfo result;
    result.m_width              = info.xres;
    result.m_height             = info.yres;
    result.m_virtualWidth       = info.xres_virtual;
    result.m_virtualHeight      = info.yres_virtual;
    result.m_xOffset            = info.xoffset;
    result.m_yOffset            = info.yoffset;
    result.m_bitsPerPixel       = info.bits_per_pixel;
    result.m_grayscale          = info.grayscale;
    result.m_red                = JobFbBitfield::fromFb(info.red);
    result.m_green              = JobFbBitfield::fromFb(info.green);
    result.m_blue               = JobFbBitfield::fromFb(info.blue);
    result.m_alpha              = JobFbBitfield::fromFb(info.transp);
    result.m_nonStandard        = info.nonstd;
    result.m_activate           = info.activate;
    result.m_physicalHeightMm   = info.height;
    result.m_physicalWidthMm    = info.width;
    result.m_accelerationFlags  = info.accel_flags;
    result.m_pixelClockPs       = info.pixclock;
    result.m_leftMargin         = info.left_margin;
    result.m_rightMargin        = info.right_margin;
    result.m_upperMargin        = info.upper_margin;
    result.m_lowerMargin        = info.lower_margin;
    result.m_hsyncLength        = info.hsync_len;
    result.m_vsyncLength        = info.vsync_len;
    result.m_sync               = info.sync;
    result.m_videoMode          = info.vmode;
    result.m_rotation           = info.rotate;
    result.m_colorSpace         = info.colorspace;
    result.m_reserved           = { info.reserved[0], info.reserved[1], info.reserved[2], info.reserved[3] };

    return result;
}

fb_var_screeninfo JobFbVariableInfo::toFb() const noexcept
{
    fb_var_screeninfo info{};
    info.xres           = m_width;
    info.yres           = m_height;
    info.xres_virtual   = m_virtualWidth;
    info.yres_virtual   = m_virtualHeight;
    info.xoffset        = m_xOffset;
    info.yoffset        = m_yOffset;
    info.bits_per_pixel = m_bitsPerPixel;
    info.grayscale      = m_grayscale;
    info.red            = m_red.toFb();
    info.green          = m_green.toFb();
    info.blue           = m_blue.toFb();
    info.transp         = m_alpha.toFb();
    info.nonstd         = m_nonStandard;
    info.activate       = m_activate;
    info.height         = m_physicalHeightMm;
    info.width          = m_physicalWidthMm;
    info.accel_flags    = m_accelerationFlags;
    info.pixclock       = m_pixelClockPs;
    info.left_margin    = m_leftMargin;
    info.right_margin   = m_rightMargin;
    info.upper_margin   = m_upperMargin;
    info.lower_margin   = m_lowerMargin;
    info.hsync_len      = m_hsyncLength;
    info.vsync_len      = m_vsyncLength;
    info.sync           = m_sync;
    info.vmode          = m_videoMode;
    info.rotate         = m_rotation;
    info.colorspace     = m_colorSpace;
    info.reserved[0]    = m_reserved[0];
    info.reserved[1]    = m_reserved[1];
    info.reserved[2]    = m_reserved[2];
    info.reserved[3]    = m_reserved[3];

    return info;
}

}