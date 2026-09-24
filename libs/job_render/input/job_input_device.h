#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "job_input_event.h"

namespace job::input {

enum class InputCapability : std::uint32_t
{
    None       = 0,
    Keyboard   = 1u << 0,
    Pointer    = 1u << 1,
    Touch      = 1u << 2,
    TabletTool = 1u << 3,
    TabletPad  = 1u << 4,
    Gesture    = 1u << 5,
    Switch     = 1u << 6
};

[[nodiscard]] constexpr InputCapability operator|(InputCapability a, InputCapability b) noexcept
{
    return static_cast<InputCapability>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

[[nodiscard]] constexpr InputCapability operator&(InputCapability a, InputCapability b) noexcept
{
    return static_cast<InputCapability>(static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b));
}

constexpr InputCapability &operator|=(InputCapability &a, InputCapability b) noexcept
{
    a = a | b;
    return a;
}

[[nodiscard]] constexpr bool hasCapability(InputCapability value, InputCapability capability) noexcept
{
    return (static_cast<std::uint32_t>(value) & static_cast<std::uint32_t>(capability)) != 0;
}

class JobInputDevice
{
public:
    using Ptr  = std::shared_ptr<JobInputDevice>;
    using WPtr = std::weak_ptr<JobInputDevice>;
    using UPtr = std::unique_ptr<JobInputDevice>;

    JobInputDevice();
    ~JobInputDevice();

    JobInputDevice(const JobInputDevice &) = delete;
    JobInputDevice &operator=(const JobInputDevice &) = delete;
    JobInputDevice(JobInputDevice &&) = delete;
    JobInputDevice &operator=(JobInputDevice &&) = delete;

    [[nodiscard]] static Ptr createShared();
    [[nodiscard]] static UPtr createUniq();

    [[nodiscard]] InputDeviceId id() const noexcept;
    void setId(InputDeviceId id) noexcept;

    [[nodiscard]] const std::string &name() const noexcept;
    void setName(std::string name);

    [[nodiscard]] const std::string &sysName() const noexcept;
    void setSysName(std::string sysName);

    [[nodiscard]] std::uint32_t vendorId() const noexcept;
    void setVendorId(std::uint32_t vendorId) noexcept;

    [[nodiscard]] std::uint32_t productId() const noexcept;
    void setProductId(std::uint32_t productId) noexcept;

    [[nodiscard]] InputCapability capabilities() const noexcept;
    void setCapabilities(InputCapability capabilities) noexcept;

    [[nodiscard]] bool hasCapability(InputCapability capability) const noexcept;

private:
    InputDeviceId   m_id{};
    std::string     m_name;
    std::string     m_sysName;
    std::uint32_t   m_vendorId{};
    std::uint32_t   m_productId{};
    InputCapability m_capabilities{InputCapability::None};
};

} // namespace job::input