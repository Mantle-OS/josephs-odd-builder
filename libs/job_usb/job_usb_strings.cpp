#include "job_usb_strings.h"

#include <utility>

namespace job::usb {

JobUsbStrings::Ptr JobUsbStrings::createShared()
{
    return std::make_shared<JobUsbStrings>();
}

JobUsbStrings::Ptr JobUsbStrings::createShared(std::optional<std::string> manufacturer,
                                               std::optional<std::string> product,
                                               std::optional<std::string> serialNumber)
{
    return std::make_shared<JobUsbStrings>(std::move(manufacturer),
                                           std::move(product),
                                           std::move(serialNumber));
}

JobUsbStrings::UPtr JobUsbStrings::createUniq()
{
    return std::make_unique<JobUsbStrings>();
}

JobUsbStrings::UPtr JobUsbStrings::createUniq(std::optional<std::string> manufacturer,
                                              std::optional<std::string> product,
                                              std::optional<std::string> serialNumber)
{
    return std::make_unique<JobUsbStrings>(std::move(manufacturer),
                                           std::move(product),
                                           std::move(serialNumber));
}

} // namespace job::usb