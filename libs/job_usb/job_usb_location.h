#pragma once

#include <cstdint>
#include <inplace_vector>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <job_base_obj.h>

#include "jobusb_export.h"

namespace job::usb {

class JOBUSB_EXPORT JobUsbLocation : public core::BaseObject
{
public:
    using Ptr = std::shared_ptr<JobUsbLocation>;
    using WPtr = std::weak_ptr<JobUsbLocation>;
    using UPtr = std::unique_ptr<JobUsbLocation>;

    // max depth 7 -> https://libusb.sourceforge.io/api-1.0/group__libusb__dev.html
    using PortPath = std::inplace_vector<uint8_t, 7>;

    JobUsbLocation() = default;

    JobUsbLocation(uint8_t busNumber, uint8_t deviceAddress, PortPath portPath, std::optional<std::string> path = std::nullopt) noexcept :
        m_portPath(std::move(portPath)),
        m_path(std::move(path)),
        m_busNumber(busNumber),
        m_deviceAddress(deviceAddress)
    {
    }

    ~JobUsbLocation() override = default;

    JobUsbLocation(const JobUsbLocation &) = default;
    JobUsbLocation &operator=(const JobUsbLocation &) = default;
    JobUsbLocation(JobUsbLocation &&) noexcept = default;
    JobUsbLocation &operator=(JobUsbLocation &&) noexcept = default;

    [[nodiscard]] static Ptr createShared();
    [[nodiscard]] static Ptr createShared(uint8_t busNumber,
                                          uint8_t deviceAddress,
                                          PortPath portPath,
                                          std::optional<std::string> path = std::nullopt);

    [[nodiscard]] static UPtr createUniq();
    [[nodiscard]] static UPtr createUniq(uint8_t busNumber,
                                         uint8_t deviceAddress,
                                         PortPath portPath,
                                         std::optional<std::string> path = std::nullopt);

    [[nodiscard]] uint8_t busNumber() const noexcept
    {
        return m_busNumber;
    }

    [[nodiscard]] uint8_t deviceAddress() const noexcept
    {
        return m_deviceAddress;
    }

    [[nodiscard]] const PortPath &portPath() const noexcept
    {
        return m_portPath;
    }

    [[nodiscard]] const std::optional<std::string> &path() const noexcept
    {
        return m_path;
    }

    [[nodiscard]] std::string_view pathView() const noexcept
    {
        if (!m_path)
            return {};

        return {m_path->data(), m_path->size()};
    }

    void setBusNumber(uint8_t value) noexcept
    {
        m_busNumber = value;
    }

    void setDeviceAddress(uint8_t value) noexcept
    {
        m_deviceAddress = value;
    }

    void setPortPath(PortPath value) noexcept
    {
        m_portPath = std::move(value);
    }

    void setPath(std::optional<std::string> value) noexcept
    {
        m_path = std::move(value);
    }

private:
    PortPath                   m_portPath;
    std::optional<std::string> m_path;
    uint8_t                    m_busNumber     = 0;
    uint8_t                    m_deviceAddress = 0;

    // DFU-specific location/runtime information deliberately does not live here.
};

} // namespace job::usb