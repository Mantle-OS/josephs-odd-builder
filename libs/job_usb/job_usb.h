#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <job_light_object.h>
#include <job_signal.h>
#include <job_io_async_thread.h>

#include "job_usb_device.h"
#include "job_usb_monitor.h"
#include "job_usb_udev.h"
#include "jobusb_export.h"

struct libusb_context;

namespace job::usb {

class JOBUSB_EXPORT JobUsb : public core::LightObject
{
public:
    using Ptr  = std::shared_ptr<JobUsb>;
    using WPtr = std::weak_ptr<JobUsb>;
    using UPtr = std::unique_ptr<JobUsb>;

    explicit JobUsb(bool autoStart = false);
    ~JobUsb() override;

    JobUsb(const JobUsb &) = delete;
    JobUsb &operator=(const JobUsb &) = delete;
    JobUsb(JobUsb &&) = delete;
    JobUsb &operator=(JobUsb &&) = delete;

    [[nodiscard]] static Ptr  createShared(bool autoStart = false);
    [[nodiscard]] static UPtr createUniq(bool autoStart = false);

    [[nodiscard]] bool start();
    void stop();

    [[nodiscard]] bool scan();

    [[nodiscard]] bool initialized() const noexcept;
    [[nodiscard]] std::string_view lastError() const noexcept;

    [[nodiscard]] JobUsbDevice *deviceByUid(uint64_t uid) noexcept;
    [[nodiscard]] JobUsbDevice *deviceByPath(std::string_view path) noexcept;
    [[nodiscard]] JobUsbDevice *deviceByBusAndAddr(uint8_t bus, uint8_t addr) noexcept;

    [[nodiscard]] std::size_t deviceCount() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

    [[nodiscard]] std::string_view vendorName(uint16_t vendorId) const noexcept;
    [[nodiscard]] std::string_view productName(uint16_t vendorId, uint16_t productId) const noexcept;


    core::Signal<JobUsbMonitorAction, std::string> deviceChanged;

private:
    void onDeviceChanged(JobUsbMonitorAction action, std::string path);

    libusb_context                  *m_context = nullptr;
    threads::JobIoAsyncThread::Ptr  m_loop;
    JobUsbMonitor::Ptr              m_monitor;
    JobUsbUdev::Ptr                 m_udev;
    std::vector<JobUsbDevice::Ptr>  m_devices;
    std::string                     m_lastError;
    bool                            m_initialized = false;
    uint64_t                        m_nextUid = 1;
};

} // namespace job::usb