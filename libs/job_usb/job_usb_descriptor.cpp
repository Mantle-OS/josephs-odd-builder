#include "job_usb_descriptor.h"

namespace job::usb {

JobUsbDescriptor::Ptr JobUsbDescriptor::createShared()
{
    return std::make_shared<JobUsbDescriptor>();
}

JobUsbDescriptor::Ptr JobUsbDescriptor::createShared(uint16_t vendorId,
                                                     uint16_t productId,
                                                     uint16_t usbSpecVersion,
                                                     uint16_t deviceVersion,
                                                     uint8_t deviceClass,
                                                     uint8_t subClass,
                                                     uint8_t protocol)
{
    return std::make_shared<JobUsbDescriptor>(vendorId,
                                              productId,
                                              usbSpecVersion,
                                              deviceVersion,
                                              deviceClass,
                                              subClass,
                                              protocol);
}

JobUsbDescriptor::UPtr JobUsbDescriptor::createUniq()
{
    return std::make_unique<JobUsbDescriptor>();
}

JobUsbDescriptor::UPtr JobUsbDescriptor::createUniq(uint16_t vendorId,
                                                    uint16_t productId,
                                                    uint16_t usbSpecVersion,
                                                    uint16_t deviceVersion,
                                                    uint8_t deviceClass,
                                                    uint8_t subClass,
                                                    uint8_t protocol)
{
    return std::make_unique<JobUsbDescriptor>(vendorId,
                                              productId,
                                              usbSpecVersion,
                                              deviceVersion,
                                              deviceClass,
                                              subClass,
                                              protocol);
}

} // namespace job::usb