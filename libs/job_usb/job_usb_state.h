#pragma once

#include <memory>

#include <job_base_obj.h>
#include <job_obj_annotation.h>

#include "jobusb_export.h"

struct libusb_device;

namespace job::usb {

class JOBUSB_EXPORT JobUsbState : public core::BaseObject
{
public:
    using Ptr = std::shared_ptr<JobUsbState>;
    using WPtr = std::weak_ptr<JobUsbState>;
    using UPtr = std::unique_ptr<JobUsbState>;

    JobUsbState() = default;
    explicit JobUsbState(libusb_device *nativeDevice, bool present = true);

    ~JobUsbState() override = default;

    JobUsbState(const JobUsbState &) = default;
    JobUsbState &operator=(const JobUsbState &) = default;
    JobUsbState(JobUsbState &&) noexcept = default;
    JobUsbState &operator=(JobUsbState &&) noexcept = default;

    [[nodiscard]] static Ptr createShared();
    [[nodiscard]] static Ptr createShared(libusb_device *nativeDevice, bool present = true);

    [[nodiscard]] static UPtr createUniq();
    [[nodiscard]] static UPtr createUniq(libusb_device *nativeDevice, bool present = true);

    [[nodiscard]] libusb_device *nativeDevice() const noexcept
    {
        return m_nativeDevice.get();
    }

    [[nodiscard]] bool present() const noexcept
    {
        return m_present;
    }

    [[nodiscard]] bool openedByJob() const noexcept
    {
        return m_openedByJob;
    }

    void setNativeDevice(libusb_device *device);

    void setPresent(bool value) noexcept
    {
        m_present = value;
    }

    void setOpenedByJob(bool value) noexcept
    {
        m_openedByJob = value;
    }

private:
    struct NativeDeviceDeleter
    {
        void operator()(libusb_device *device) const noexcept;
    };

    using NativeDevicePtr = std::shared_ptr<libusb_device>;

    [[nodiscard]] static NativeDevicePtr retainNativeDevice(libusb_device *device);

    [[=core::NoSerialize{}]]
        NativeDevicePtr m_nativeDevice;

    [[=core::NoSerialize{}]]
        bool m_present = false;

    [[=core::NoSerialize{}]]
        bool m_openedByJob = false;
};

} // namespace job::usb