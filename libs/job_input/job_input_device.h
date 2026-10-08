#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "job_input_capability.h"
#include "job_input_device_classification.h"

namespace job::input {

class JobInputDevice
{
public:
    using Ptr  = std::shared_ptr<JobInputDevice>;
    using WPtr = std::weak_ptr<JobInputDevice>;
    using UPtr = std::unique_ptr<JobInputDevice>;

    constexpr JobInputDevice() noexcept = default;

    JobInputDevice(std::string name,
                   std::string physicalLocation,
                   std::string uniqueId,
                   std::uint16_t busType,
                   std::uint16_t vendor,
                   std::uint16_t product,
                   std::uint16_t version,
                   JobInputCapability capability) noexcept :
        m_name(std::move(name)),
        m_physicalLocation(std::move(physicalLocation)),
        m_uniqueId(std::move(uniqueId)),
        m_busType(busType),
        m_vendor(vendor),
        m_product(product),
        m_version(version),
        m_capability(std::move(capability)),
        m_classification(m_capability)
    {
    }

    ~JobInputDevice() = default;

    JobInputDevice(const JobInputDevice &) = default;
    JobInputDevice &operator=(const JobInputDevice &) = default;

    JobInputDevice(JobInputDevice &&) noexcept = default;
    JobInputDevice &operator=(JobInputDevice &&) noexcept = default;

    [[nodiscard]] static Ptr createShared(std::string name,
                                          std::string physicalLocation,
                                          std::string uniqueId,
                                          std::uint16_t busType,
                                          std::uint16_t vendor,
                                          std::uint16_t product,
                                          std::uint16_t version,
                                          JobInputCapability capability)
    {
        return std::make_shared<JobInputDevice>(std::move(name),
                                                std::move(physicalLocation),
                                                std::move(uniqueId),
                                                busType,
                                                vendor,
                                                product,
                                                version,
                                                std::move(capability));
    }

    [[nodiscard]] static UPtr createUniq(std::string name,
                                         std::string physicalLocation,
                                         std::string uniqueId,
                                         std::uint16_t busType,
                                         std::uint16_t vendor,
                                         std::uint16_t product,
                                         std::uint16_t version,
                                         JobInputCapability capability)
    {
        return std::make_unique<JobInputDevice>(std::move(name),
                                                std::move(physicalLocation),
                                                std::move(uniqueId),
                                                busType,
                                                vendor,
                                                product,
                                                version,
                                                std::move(capability));
    }

    [[nodiscard]] std::string_view name() const noexcept
    {
        return m_name;
    }

    [[nodiscard]] std::string_view physicalLocation() const noexcept
    {
        return m_physicalLocation;
    }

    [[nodiscard]] std::string_view uniqueId() const noexcept
    {
        return m_uniqueId;
    }

    [[nodiscard]] constexpr std::uint16_t busType() const noexcept
    {
        return m_busType;
    }

    [[nodiscard]] constexpr std::uint16_t vendor() const noexcept
    {
        return m_vendor;
    }

    [[nodiscard]] constexpr std::uint16_t product() const noexcept
    {
        return m_product;
    }

    [[nodiscard]] constexpr std::uint16_t version() const noexcept
    {
        return m_version;
    }

    [[nodiscard]] constexpr const JobInputCapability &capability() const noexcept
    {
        return m_capability;
    }

    [[nodiscard]] constexpr const JobInputDeviceClassification &classification() const noexcept
    {
        return m_classification;
    }

private:
    std::string                  m_name;
    std::string                  m_physicalLocation;
    std::string                  m_uniqueId;

    std::uint16_t                m_busType{};
    std::uint16_t                m_vendor{};
    std::uint16_t                m_product{};
    std::uint16_t                m_version{};

    JobInputCapability           m_capability;
    JobInputDeviceClassification m_classification;
};

} // namespace job::input