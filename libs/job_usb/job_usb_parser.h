#pragma once

#include <job_usb_device.h>

#include "jobusb_export.h"

struct libusb_device;
struct libusb_device_descriptor;

namespace job::usb {

class JobUsbIdDatabase;
class JobUsbUdev;

class JOBUSB_EXPORT JobUsbParser
{
public:
    JobUsbParser() = delete;
    ~JobUsbParser() = delete;

    JobUsbParser(const JobUsbParser &) = delete;
    JobUsbParser &operator=(const JobUsbParser &) = delete;
    JobUsbParser(JobUsbParser &&) = delete;
    JobUsbParser &operator=(JobUsbParser &&) = delete;

    [[nodiscard]] static JobUsbDevice::Ptr parse(libusb_device *device,
                                                 const libusb_device_descriptor &descriptor,
                                                 const JobUsbIdDatabase *database = nullptr,
                                                 const JobUsbUdev *udev = nullptr);
};

} // namespace job::usb