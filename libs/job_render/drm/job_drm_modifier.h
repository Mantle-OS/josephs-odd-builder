#pragma once

#include <cstdint>

#include <drm_fourcc.h>

namespace job::render {

enum class DrmModifierVendor : std::uint8_t
{
    None      = DRM_FORMAT_MOD_VENDOR_NONE,
    Intel     = DRM_FORMAT_MOD_VENDOR_INTEL,
    Amd       = DRM_FORMAT_MOD_VENDOR_AMD,
    Nvidia    = DRM_FORMAT_MOD_VENDOR_NVIDIA,
    Samsung   = DRM_FORMAT_MOD_VENDOR_SAMSUNG,
    Qualcomm  = DRM_FORMAT_MOD_VENDOR_QCOM,
    Vivante   = DRM_FORMAT_MOD_VENDOR_VIVANTE,
    Broadcom  = DRM_FORMAT_MOD_VENDOR_BROADCOM,
    Arm       = DRM_FORMAT_MOD_VENDOR_ARM,
    Allwinner = DRM_FORMAT_MOD_VENDOR_ALLWINNER,
    Amlogic   = DRM_FORMAT_MOD_VENDOR_AMLOGIC,
    Mediatek  = DRM_FORMAT_MOD_VENDOR_MTK,
    Apple     = DRM_FORMAT_MOD_VENDOR_APPLE
};

class JobDrmModifier
{
public:
    using Value = std::uint64_t;

    constexpr JobDrmModifier() = default;

    explicit constexpr JobDrmModifier(Value value) noexcept :
        m_value(value)
    {
    }

    ~JobDrmModifier() = default;

    JobDrmModifier(const JobDrmModifier &) = default;
    JobDrmModifier &operator=(const JobDrmModifier &) = default;

    JobDrmModifier(JobDrmModifier &&) noexcept = default;
    JobDrmModifier &operator=(JobDrmModifier &&) noexcept = default;

    [[nodiscard]] constexpr Value value() const noexcept
    {
        return m_value;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_value != DRM_FORMAT_MOD_INVALID;
    }

    [[nodiscard]] constexpr bool linear() const noexcept
    {
        return m_value == DRM_FORMAT_MOD_LINEAR;
    }

    [[nodiscard]] constexpr DrmModifierVendor vendor() const noexcept
    {
        return static_cast<DrmModifierVendor>(fourcc_mod_get_vendor(m_value));
    }

    [[nodiscard]] constexpr bool vendor(DrmModifierVendor vendor) const noexcept
    {
        return this->vendor() == vendor;
    }

    [[nodiscard]] static constexpr JobDrmModifier invalid() noexcept
    {
        return JobDrmModifier{DRM_FORMAT_MOD_INVALID};
    }

    [[nodiscard]] static constexpr JobDrmModifier linearModifier() noexcept
    {
        return JobDrmModifier{DRM_FORMAT_MOD_LINEAR};
    }

    [[nodiscard]] friend constexpr bool operator==(const JobDrmModifier &, const JobDrmModifier &) noexcept = default;

private:
    Value m_value{DRM_FORMAT_MOD_INVALID};
};

}