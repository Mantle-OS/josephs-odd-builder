#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

#include "jobusb_export.h"

struct udev;
struct udev_device;

namespace job::usb {

class JOBUSB_EXPORT JobUsbUdev
{
public:
    using Ptr = std::shared_ptr<JobUsbUdev>;
    using WPtr = std::weak_ptr<JobUsbUdev>;
    using UPtr = std::unique_ptr<JobUsbUdev>;

    JobUsbUdev();
    ~JobUsbUdev() = default;

    JobUsbUdev(const JobUsbUdev &) = delete;
    JobUsbUdev &operator=(const JobUsbUdev &) = delete;
    JobUsbUdev(JobUsbUdev &&) = delete;
    JobUsbUdev &operator=(JobUsbUdev &&) = delete;

    [[nodiscard]] static Ptr createShared();
    [[nodiscard]] static UPtr createUniq();

    [[nodiscard]] bool valid() const noexcept;

    [[nodiscard]] std::optional<std::string> sysfsPath(uint8_t busNumber,
                                                       uint8_t deviceAddress) const;

    [[nodiscard]] std::optional<std::string> property(uint8_t busNumber,
                                                      uint8_t deviceAddress,
                                                      std::string_view name) const;

private:
    [[nodiscard]] static bool parseUnsigned(std::string_view text, unsigned int &value) noexcept;

    struct UdevDeleter
    {
        void operator()(udev *context) const noexcept;
    };

    struct UdevDeviceDeleter
    {
        void operator()(udev_device *device) const noexcept;
    };

    using UdevPtr = std::unique_ptr<udev, UdevDeleter>;
    using UdevDevicePtr = std::unique_ptr<udev_device, UdevDeviceDeleter>;

    // Caller MUST HOLD m_mutex.
    [[nodiscard]] UdevDevicePtr findDevice(uint8_t busNumber, uint8_t deviceAddress) const;

    UdevPtr             m_udev;
    mutable std::mutex  m_mutex;
};

} // namespace job::usb