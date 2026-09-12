#pragma once

#include <cstddef>

#include <libusb-1.0/libusb.h>

namespace job::usb::tests {

class LibUsbDeviceFixture
{
public:
    LibUsbDeviceFixture()
    {
        if (libusb_init(&m_context) != LIBUSB_SUCCESS)
            return;

        const auto count = libusb_get_device_list(m_context, &m_devices);

        if (count <= 0)
            return;

        m_deviceCount = static_cast<std::size_t>(count);
        m_device = m_devices[0];
    }

    ~LibUsbDeviceFixture()
    {
        releaseDeviceList();

        if (m_context)
            libusb_exit(m_context);
    }

    LibUsbDeviceFixture(const LibUsbDeviceFixture &) = delete;
    LibUsbDeviceFixture &operator=(const LibUsbDeviceFixture &) = delete;
    LibUsbDeviceFixture(LibUsbDeviceFixture &&) = delete;
    LibUsbDeviceFixture &operator=(LibUsbDeviceFixture &&) = delete;

    [[nodiscard]] bool initialized() const noexcept
    {
        return m_context != nullptr;
    }

    [[nodiscard]] bool hasDevice() const noexcept
    {
        return m_device != nullptr;
    }

    [[nodiscard]] libusb_context *context() const noexcept
    {
        return m_context;
    }

    [[nodiscard]] libusb_device *device() const noexcept
    {
        return m_device;
    }

    [[nodiscard]] std::size_t deviceCount() const noexcept
    {
        return m_deviceCount;
    }
    [[nodiscard]] uint8_t busNumber() const noexcept
    {
        if (!m_device)
            return 0;

        return libusb_get_bus_number(m_device);
    }

    [[nodiscard]] uint8_t deviceAddress() const noexcept
    {
        if (!m_device)
            return 0;

        return libusb_get_device_address(m_device);
    }

    [[nodiscard]] libusb_device_descriptor descriptor() const noexcept
    {
        libusb_device_descriptor descriptor{};

        if (m_device)
            libusb_get_device_descriptor(m_device, &descriptor);

        return descriptor;
    }

    void releaseDeviceList() noexcept
    {
        if (!m_devices)
            return;

        libusb_free_device_list(m_devices, 1);
        m_devices = nullptr;
        m_device = nullptr;
        m_deviceCount = 0;
    }

private:
    libusb_context  *m_context      = nullptr;
    libusb_device   **m_devices     = nullptr;
    libusb_device   *m_device       = nullptr;
    std::size_t     m_deviceCount   = 0;
};

} // namespace job::usb::tests