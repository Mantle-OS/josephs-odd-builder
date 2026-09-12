#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <job_base_obj.h>

#include "jobusb_export.h"

namespace job::usb {

class JOBUSB_EXPORT JobUsbStrings : public core::BaseObject
{
public:
    using Ptr = std::shared_ptr<JobUsbStrings>;
    using WPtr = std::weak_ptr<JobUsbStrings>;
    using UPtr = std::unique_ptr<JobUsbStrings>;

    JobUsbStrings() = default;

    JobUsbStrings(std::optional<std::string> manufacturer,
                  std::optional<std::string> product,
                  std::optional<std::string> serialNumber) noexcept :
        m_manufacturer(std::move(manufacturer)),
        m_product(std::move(product)),
        m_serialNumber(std::move(serialNumber))
    {
    }

    ~JobUsbStrings() override = default;

    JobUsbStrings(const JobUsbStrings &) = default;
    JobUsbStrings &operator=(const JobUsbStrings &) = default;
    JobUsbStrings(JobUsbStrings &&) noexcept = default;
    JobUsbStrings &operator=(JobUsbStrings &&) noexcept = default;

    [[nodiscard]] static Ptr createShared();
    [[nodiscard]] static Ptr createShared(std::optional<std::string> manufacturer,
                                          std::optional<std::string> product,
                                          std::optional<std::string> serialNumber);

    [[nodiscard]] static UPtr createUniq();
    [[nodiscard]] static UPtr createUniq(std::optional<std::string> manufacturer,
                                         std::optional<std::string> product,
                                         std::optional<std::string> serialNumber);

    [[nodiscard]] const std::optional<std::string> &manufacturer() const noexcept
    {
        return m_manufacturer;
    }

    [[nodiscard]] const std::optional<std::string> &product() const noexcept
    {
        return m_product;
    }

    [[nodiscard]] const std::optional<std::string> &serialNumber() const noexcept
    {
        return m_serialNumber;
    }

    [[nodiscard]] std::string_view manufacturerView() const noexcept
    {
        if (!m_manufacturer)
            return {};

        return {m_manufacturer->data(), m_manufacturer->size()};
    }

    [[nodiscard]] std::string_view productView() const noexcept
    {
        if (!m_product)
            return {};

        return {m_product->data(), m_product->size()};
    }

    [[nodiscard]] std::string_view serialNumberView() const noexcept
    {
        if (!m_serialNumber)
            return {};

        return {m_serialNumber->data(), m_serialNumber->size()};
    }

    void setManufacturer(std::optional<std::string> value) noexcept
    {
        m_manufacturer = std::move(value);
    }

    void setProduct(std::optional<std::string> value) noexcept
    {
        m_product = std::move(value);
    }

    void setSerialNumber(std::optional<std::string> value) noexcept
    {
        m_serialNumber = std::move(value);
    }

private:
    std::optional<std::string> m_manufacturer;
    std::optional<std::string> m_product;
    std::optional<std::string> m_serialNumber;
};

} // namespace job::usb