#pragma once

#include <cstdint>
#include <memory>
#include <utility>

#include <job_object.h>
#include <job_signal.h>
#include <job_obj_annotation.h>

#include "job_usb_descriptor.h"
#include "job_usb_location.h"
#include "job_usb_state.h"
#include "job_usb_strings.h"
#include "jobusb_export.h"

struct libusb_device;

namespace job::usb {

class JOBUSB_EXPORT JobUsbDevice : public core::Object
{
public:
    using Ptr = std::shared_ptr<JobUsbDevice>;
    using WPtr = std::weak_ptr<JobUsbDevice>;
    using UPtr = std::unique_ptr<JobUsbDevice>;

    JobUsbDevice() = default;

    JobUsbDevice(uint64_t uid, JobUsbDescriptor descriptor, JobUsbLocation location, JobUsbStrings strings, JobUsbState state) noexcept :
        m_descriptor(std::move(descriptor)),
        m_location(std::move(location)),
        m_strings(std::move(strings)),
        m_state(std::move(state)),
        m_uid(uid)
    {
    }

    ~JobUsbDevice() override = default;
    JobUsbDevice(const JobUsbDevice &) = delete;
    JobUsbDevice &operator=(const JobUsbDevice &) = delete;
    JobUsbDevice(JobUsbDevice &&) = delete;
    JobUsbDevice &operator=(JobUsbDevice &&) = delete;

    [[nodiscard]] static Ptr createShared();
    [[nodiscard]] static Ptr createShared(uint64_t uid,
                                          JobUsbDescriptor descriptor,
                                          JobUsbLocation location,
                                          JobUsbStrings strings,
                                          JobUsbState state);

    [[nodiscard]] static UPtr createUniq();
    [[nodiscard]] static UPtr createUniq(uint64_t uid,
                                         JobUsbDescriptor descriptor,
                                         JobUsbLocation location,
                                         JobUsbStrings strings,
                                         JobUsbState state);

    [[nodiscard]] uint64_t uid() const noexcept
    {
        return m_uid;
    }

    [[nodiscard]] const JobUsbDescriptor &descriptor() const noexcept
    {
        return m_descriptor;
    }

    [[nodiscard]] JobUsbDescriptor &descriptor() noexcept
    {
        return m_descriptor;
    }

    [[nodiscard]] const JobUsbLocation &location() const noexcept
    {
        return m_location;
    }

    [[nodiscard]] JobUsbLocation &location() noexcept
    {
        return m_location;
    }

    [[nodiscard]] const JobUsbStrings &strings() const noexcept
    {
        return m_strings;
    }

    [[nodiscard]] JobUsbStrings &strings() noexcept
    {
        return m_strings;
    }

    [[nodiscard]] const JobUsbState &state() const noexcept
    {
        return m_state;
    }

    [[nodiscard]] JobUsbState &state() noexcept
    {
        return m_state;
    }

    // Shorthand for runtime code that needs the retained native libusb object.
    [[nodiscard]] libusb_device *nativeDevice() const noexcept
    {
        return m_state.nativeDevice();
    }

    void setUid(uint64_t value) noexcept
    {
        m_uid = value;
    }

    void setDescriptor(JobUsbDescriptor value)
    {
        m_descriptor = std::move(value);
    }

    void setLocation(JobUsbLocation value)
    {
        m_location = std::move(value);
    }

    void setStrings(JobUsbStrings value)
    {
        m_strings = std::move(value);
    }

    void setState(JobUsbState value)
    {
        m_state = std::move(value);
    }


    core::Signal<const JobUsbDevice &> updated;
    void handleUpdated(const JobUsbDevice &device)
    {
        setDescriptor(device.descriptor());
        setLocation(device.location());
        setStrings(device.strings());
    }

private:
    JobUsbDescriptor m_descriptor;
    JobUsbLocation m_location;
    JobUsbStrings m_strings;
    JobUsbState m_state;

    [[=core::NoSerialize{}]] uint64_t m_uid = 0;
};

} // namespace job::usb