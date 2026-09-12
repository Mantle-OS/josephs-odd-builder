#pragma once

#include <cstdint>
#include <memory>

#include <job_base_obj.h>

#include "jobusb_export.h"

namespace job::usb {

class JOBUSB_EXPORT JobUsbDescriptor : public core::BaseObject
{
public:
    using Ptr = std::shared_ptr<JobUsbDescriptor>;
    using WPtr = std::weak_ptr<JobUsbDescriptor>;
    using UPtr = std::unique_ptr<JobUsbDescriptor>;

    JobUsbDescriptor() = default;

    JobUsbDescriptor(uint16_t vendorId,
                     uint16_t productId,
                     uint16_t usbSpecVersion,
                     uint16_t deviceVersion,
                     uint8_t deviceClass,
                     uint8_t subClass,
                     uint8_t protocol) noexcept :
        m_vendorId(vendorId),
        m_productId(productId),
        m_usbSpecVersion(usbSpecVersion),
        m_deviceVersion(deviceVersion),
        m_deviceClass(deviceClass),
        m_subClass(subClass),
        m_protocol(protocol)
    {
    }

    ~JobUsbDescriptor() override = default;

    JobUsbDescriptor(const JobUsbDescriptor &) = default;
    JobUsbDescriptor &operator=(const JobUsbDescriptor &) = default;
    JobUsbDescriptor(JobUsbDescriptor &&) noexcept = default;
    JobUsbDescriptor &operator=(JobUsbDescriptor &&) noexcept = default;

    [[nodiscard]] static Ptr createShared();
    [[nodiscard]] static Ptr createShared(uint16_t vendorId,
                                          uint16_t productId,
                                          uint16_t usbSpecVersion,
                                          uint16_t deviceVersion,
                                          uint8_t deviceClass,
                                          uint8_t subClass,
                                          uint8_t protocol);

    [[nodiscard]] static UPtr createUniq();
    [[nodiscard]] static UPtr createUniq(uint16_t vendorId,
                                         uint16_t productId,
                                         uint16_t usbSpecVersion,
                                         uint16_t deviceVersion,
                                         uint8_t deviceClass,
                                         uint8_t subClass,
                                         uint8_t protocol);

    [[nodiscard]] uint16_t vendorId() const noexcept
    {
        return m_vendorId;
    }

    [[nodiscard]] uint16_t productId() const noexcept
    {
        return m_productId;
    }

    [[nodiscard]] uint16_t usbSpecVersion() const noexcept
    {
        return m_usbSpecVersion;
    }

    [[nodiscard]] uint16_t deviceVersion() const noexcept
    {
        return m_deviceVersion;
    }

    [[nodiscard]] uint8_t deviceClass() const noexcept
    {
        return m_deviceClass;
    }

    [[nodiscard]] uint8_t subClass() const noexcept
    {
        return m_subClass;
    }

    [[nodiscard]] uint8_t protocol() const noexcept
    {
        return m_protocol;
    }

    void setVendorId(uint16_t value) noexcept
    {
        m_vendorId = value;
    }

    void setProductId(uint16_t value) noexcept
    {
        m_productId = value;
    }

    void setUsbSpecVersion(uint16_t value) noexcept
    {
        m_usbSpecVersion = value;
    }

    void setDeviceVersion(uint16_t value) noexcept
    {
        m_deviceVersion = value;
    }

    void setDeviceClass(uint8_t value) noexcept
    {
        m_deviceClass = value;
    }

    void setSubClass(uint8_t value) noexcept
    {
        m_subClass = value;
    }

    void setProtocol(uint8_t value) noexcept
    {
        m_protocol = value;
    }

private:
    uint16_t m_vendorId       = 0;
    uint16_t m_productId      = 0;
    uint16_t m_usbSpecVersion = 0;
    uint16_t m_deviceVersion  = 0;

    uint8_t m_deviceClass = 0;
    uint8_t m_subClass    = 0;
    uint8_t m_protocol    = 0;
};

} // namespace job::usb