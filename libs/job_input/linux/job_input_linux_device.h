#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include <io_base.h>

#include "job_input_device.h"

namespace job::input {

namespace fs = std::filesystem;

class JobInputLinuxDevice final : public io::IODevice
{
public:
    using Ptr  = std::shared_ptr<JobInputLinuxDevice>;
    using WPtr = std::weak_ptr<JobInputLinuxDevice>;
    using UPtr = std::unique_ptr<JobInputLinuxDevice>;

    explicit JobInputLinuxDevice(fs::path path) noexcept;
    ~JobInputLinuxDevice() override;

    JobInputLinuxDevice(const JobInputLinuxDevice &) = delete;
    JobInputLinuxDevice &operator=(const JobInputLinuxDevice &) = delete;

    JobInputLinuxDevice(JobInputLinuxDevice &&other) noexcept;
    JobInputLinuxDevice &operator=(JobInputLinuxDevice &&other) noexcept;

    [[nodiscard]] static Ptr createShared(fs::path path)
    {
        return std::make_shared<JobInputLinuxDevice>(std::move(path));
    }

    [[nodiscard]] static UPtr createUniq(fs::path path)
    {
        return std::make_unique<JobInputLinuxDevice>(std::move(path));
    }

    [[nodiscard]] bool openDevice() override;
    void closeDevice() override;

    [[nodiscard]] ssize_t read(char *buffer, size_t maxlen) override;
    [[nodiscard]] ssize_t write(const char *data, size_t len) override;

    [[nodiscard]] bool isOpen() const override;
    [[nodiscard]] int fd() const override;

    void setNonBlocking(bool enabled) override;
    void setReadCallback(ReadCallback cb) override;

    [[nodiscard]] const fs::path &path() const noexcept
    {
        return m_path;
    }

    [[nodiscard]] std::string pathStr() const
    {
        return m_path.string();
    }

    [[nodiscard]] std::wstring pathWs() const
    {
        return m_path.wstring();
    }

    [[nodiscard]] const fs::path::value_type *pathChar() const noexcept
    {
        return m_path.c_str();
    }

    [[nodiscard]] bool hasDevice() const noexcept
    {
        return m_device.has_value();
    }

    [[nodiscard]] const JobInputDevice *device() const noexcept
    {
        if (!m_device)
            return nullptr;

        return &(*m_device);
    }

private:
    [[nodiscard]] bool interrogate();

    [[nodiscard]] bool queryName(std::string &name) const;
    [[nodiscard]] bool queryPhysicalLocation(std::string &physicalLocation) const;
    [[nodiscard]] bool queryUniqueId(std::string &uniqueId) const;
    [[nodiscard]] bool queryIdentity(std::uint16_t &busType,
                                     std::uint16_t &vendor,
                                     std::uint16_t &product,
                                     std::uint16_t &version) const;
    [[nodiscard]] bool queryCapabilities(JobInputCapability &capability) const;

    int                           m_fd{-1};
    fs::path                      m_path;
    ReadCallback                  m_readCallback;
    std::optional<JobInputDevice> m_device;
};

} // namespace job::input