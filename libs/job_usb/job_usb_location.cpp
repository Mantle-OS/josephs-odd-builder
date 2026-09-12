#include "job_usb_location.h"

#include <utility>

namespace job::usb {

JobUsbLocation::Ptr JobUsbLocation::createShared()
{
    return std::make_shared<JobUsbLocation>();
}

JobUsbLocation::Ptr JobUsbLocation::createShared(uint8_t busNumber,
                                                 uint8_t deviceAddress,
                                                 PortPath portPath,
                                                 std::optional<std::string> path)
{
    return std::make_shared<JobUsbLocation>(busNumber,
                                            deviceAddress,
                                            std::move(portPath),
                                            std::move(path));
}

JobUsbLocation::UPtr JobUsbLocation::createUniq()
{
    return std::make_unique<JobUsbLocation>();
}

JobUsbLocation::UPtr JobUsbLocation::createUniq(uint8_t busNumber,
                                                uint8_t deviceAddress,
                                                PortPath portPath,
                                                std::optional<std::string> path)
{
    return std::make_unique<JobUsbLocation>(busNumber,
                                            deviceAddress,
                                            std::move(portPath),
                                            std::move(path));
}

} // namespace job::usb