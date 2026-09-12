#include "job_usb.h"

#include <algorithm>
#include <utility>

#include <libusb-1.0/libusb.h>

#include <job_logger.h>

#include "job_usb_id_database.h"
#include "job_usb_parser.h"

namespace job::usb {

JobUsb::JobUsb(bool autoStart)
{
    if (autoStart)
        (void)start();
}

JobUsb::~JobUsb()
{
    stop();
}

JobUsb::Ptr JobUsb::createShared(bool autoStart)
{
    return std::make_shared<JobUsb>(autoStart);
}

JobUsb::UPtr JobUsb::createUniq(bool autoStart)
{
    return std::make_unique<JobUsb>(autoStart);
}

bool JobUsb::start()
{
    if (m_initialized)
        return true;

    m_lastError.clear();
    const int usbResult = libusb_init(&m_context);
    if (usbResult != LIBUSB_SUCCESS || !m_context) {
        m_lastError = "Failed to initialize libusb context.";
        stop();
        return false;
    }

    auto &database = JobUsbIdDatabase::instance();
    if (!database.load()) {
        m_lastError = "Failed to load usb.ids database.";

        if (!database.lastError().empty()) {
            m_lastError += " ";
            m_lastError += database.lastError();
        }

        stop();
        return false;
    }

    m_udev = JobUsbUdev::createShared();
    if (!m_udev || !m_udev->valid()) {
        m_lastError = "Failed to initialize udev helper.";
        stop();
        return false;
    }

    m_loop = threads::JobIoAsyncThread::createShared();
    if (!m_loop) {
        m_lastError = "Failed to create USB async event loop.";
        stop();
        return false;
    }

    m_loop->start();

    if (!m_loop->isRunning()) {
        m_lastError = "Failed to start USB async event loop.";
        stop();
        return false;
    }

    if (!scan()) {
        stop();
        return false;
    }

    m_monitor = JobUsbMonitor::createShared(m_loop);

    if (!m_monitor) {
        m_lastError = "Failed to create USB monitor.";
        stop();
        return false;
    }

    if (!m_monitor->start([this](JobUsbMonitorAction action, std::string_view path) { onDeviceChanged(action, std::string{path}); })) {
        m_lastError = "Failed to start USB monitor.";
        stop();
        return false;
    }

    m_initialized = true;

    return true;
}

void JobUsb::stop()
{
    /*
     * Mark the facade unavailable before dismantling runtime pieces.
     * An already queued monitor callback must not initiate another scan.
     */
    m_initialized = false;

    if (m_monitor)
        m_monitor->stop();

    /*
     * The monitor's JobIoAsyncThread callback captures the monitor/facade
     * runtime. Stop and join the loop before destroying those objects so
     * an already queued callback cannot run against released state.
     */
    if (m_loop)
        m_loop->stop();

    m_monitor.reset();

    /*
     * JobUsbDevice owns retained libusb_device references through
     * JobUsbState, so devices must die before libusb_exit().
     */
    m_devices.clear();

    if (m_context) {
        libusb_exit(m_context);
        m_context = nullptr;
    }

    m_udev.reset();
    m_loop.reset();

    m_lastError.clear();
}

bool JobUsb::scan()
{
    if (!m_context) {
        m_lastError = "Cannot scan USB devices without a libusb context.";
        return false;
    }

    libusb_device **nativeDevices = nullptr;
    const ssize_t deviceCount = libusb_get_device_list(m_context, &nativeDevices);
    if (deviceCount < 0 || !nativeDevices) {
        m_lastError = "Failed to enumerate USB devices.";
        return false;
    }

    struct DeviceListDeleter
    {
        void operator()(libusb_device **devices) const noexcept
        {
            if (devices)
                libusb_free_device_list(devices, 1);
        }
    };

    using DeviceListPtr = std::unique_ptr<libusb_device *, DeviceListDeleter>;
    DeviceListPtr deviceList{nativeDevices};

    std::vector<JobUsbDevice::Ptr> discovered;
    discovered.reserve(static_cast<std::size_t>(deviceCount));

    JobUsbIdDatabase *database = nullptr;

    auto &usbIds = JobUsbIdDatabase::instance();

    if (usbIds.ready())
        database = &usbIds;

    for (ssize_t i = 0; i < deviceCount; ++i) {
        libusb_device *nativeDevice = nativeDevices[i];

        if (!nativeDevice)
            continue;

        libusb_device_descriptor nativeDescriptor{};

        if (libusb_get_device_descriptor(nativeDevice, &nativeDescriptor) != LIBUSB_SUCCESS)
            continue;

        auto device = JobUsbParser::parse(nativeDevice, nativeDescriptor, database, m_udev.get());

        if (!device)
            continue;

        const auto &newLocation = device->location();

        /*
         * Preserve runtime identity across scans.
         *
         * sysfs path is preferred because bus/address are transient.
         * Bus/address remain a useful fallback when udev did not provide
         * a path for a device.
         */
        const auto existing = std::find_if(m_devices.begin(), m_devices.end(), [&](const JobUsbDevice::Ptr &candidate) {
            if (!candidate)
                return false;

            const auto &oldLocation = candidate->location();

            if (newLocation.path() && oldLocation.path() && *newLocation.path() == *oldLocation.path())
                return true;

            return newLocation.busNumber() == oldLocation.busNumber() &&
                   newLocation.deviceAddress() == oldLocation.deviceAddress();
        });

        if (existing != m_devices.end())
            device->setUid((*existing)->uid());
        else
            device->setUid(m_nextUid++);

        discovered.push_back(std::move(device));
    }

    /*
     * Commit only after enumeration/parsing completes. Until this point the
     * previous canonical snapshot remains untouched.
     */
    m_devices = std::move(discovered);

    m_lastError.clear();

    return true;
}

bool JobUsb::initialized() const noexcept
{
    return m_initialized;
}

std::string_view JobUsb::lastError() const noexcept
{
    return m_lastError;
}

JobUsbDevice *JobUsb::deviceByUid(uint64_t uid) noexcept
{
    const auto it = std::find_if(m_devices.begin(), m_devices.end(), [uid](const JobUsbDevice::Ptr &device) {
        return device && device->uid() == uid;
    });

    if (it == m_devices.end())
        return nullptr;

    return it->get();
}

JobUsbDevice *JobUsb::deviceByPath(std::string_view path) noexcept
{
    const auto it = std::find_if(m_devices.begin(), m_devices.end(), [path](const JobUsbDevice::Ptr &device) {
        if (!device)
            return false;

        const auto &devicePath = device->location().path();
        return devicePath && *devicePath == path;
    });

    if (it == m_devices.end())
        return nullptr;

    return it->get();
}

JobUsbDevice *JobUsb::deviceByBusAndAddr(uint8_t bus,
                                         uint8_t addr) noexcept
{
    const auto it = std::find_if(m_devices.begin(), m_devices.end(), [bus, addr](const JobUsbDevice::Ptr &device) {
        if (!device)
            return false;

        const auto &location = device->location();

        return location.busNumber() == bus &&
               location.deviceAddress() == addr;
    });

    if (it == m_devices.end())
        return nullptr;

    return it->get();
}

std::size_t JobUsb::deviceCount() const noexcept
{
    return m_devices.size();
}

bool JobUsb::empty() const noexcept
{
    return m_devices.empty();
}

std::string_view JobUsb::vendorName(uint16_t vendorId) const noexcept
{
    const auto &database = JobUsbIdDatabase::instance();

    if (!database.ready())
        return {};

    return database.vendorName(vendorId);
}

std::string_view JobUsb::productName(uint16_t vendorId,
                                     uint16_t productId) const noexcept
{
    const auto &database = JobUsbIdDatabase::instance();

    if (!database.ready())
        return {};

    return database.productName(vendorId, productId);
}

void JobUsb::onDeviceChanged(JobUsbMonitorAction action, std::string path)
{
    /*
     * stop() marks the facade unavailable before shutting down the event loop.
     * This also makes an already queued monitor callback harmless.
     */
    if (!m_initialized)
        return;

    if (!scan())
        JOB_LOG_WARN("[JobUsb] Failed to rescan after USB device event: {}", m_lastError);

    deviceChanged.emit(action, std::move(path));
}

} // namespace job::usb