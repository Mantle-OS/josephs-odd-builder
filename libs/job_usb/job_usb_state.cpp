#include "job_usb_state.h"

#include <libusb-1.0/libusb.h>

namespace job::usb {

JobUsbState::JobUsbState(libusb_device *nativeDevice, bool present) :
    m_nativeDevice(retainNativeDevice(nativeDevice)),
    m_present(present)
{
}

JobUsbState::Ptr JobUsbState::createShared()
{
    return std::make_shared<JobUsbState>();
}

JobUsbState::Ptr JobUsbState::createShared(libusb_device *nativeDevice, bool present)
{
    return std::make_shared<JobUsbState>(nativeDevice, present);
}

JobUsbState::UPtr JobUsbState::createUniq()
{
    return std::make_unique<JobUsbState>();
}

JobUsbState::UPtr JobUsbState::createUniq(libusb_device *nativeDevice, bool present)
{
    return std::make_unique<JobUsbState>(nativeDevice, present);
}

void JobUsbState::NativeDeviceDeleter::operator()(libusb_device *device) const noexcept
{
    if (device)
        libusb_unref_device(device);
}

JobUsbState::NativeDevicePtr JobUsbState::retainNativeDevice(libusb_device *device)
{
    if (!device)
        return {};

    libusb_ref_device(device);

    return NativeDevicePtr{
        device,
        NativeDeviceDeleter{}
    };
}

void JobUsbState::setNativeDevice(libusb_device *device)
{
    if (m_nativeDevice.get() == device)
        return;

    m_nativeDevice = retainNativeDevice(device);
}

} // namespace job::usb