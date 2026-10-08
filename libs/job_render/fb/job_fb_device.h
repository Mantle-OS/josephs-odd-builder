#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>

#include <io_base.h>
#include <job_mmap.h>

#include "job_fb_fixed_info.h"
#include "job_fb_format_result.h"
#include "job_fb_variable_info.h"

namespace job::render {

class JobFbDevice final : public io::IODevice
{
public:
    using Ptr  = std::shared_ptr<JobFbDevice>;
    using WPtr = std::weak_ptr<JobFbDevice>;
    using UPtr = std::unique_ptr<JobFbDevice>;

    using NativeHandle = int;
    using Size = std::size_t;

    JobFbDevice() = default;
    explicit JobFbDevice(std::filesystem::path path);
    ~JobFbDevice() override;

    JobFbDevice(const JobFbDevice &) = delete;
    JobFbDevice &operator=(const JobFbDevice &) = delete;

    JobFbDevice(JobFbDevice &&) noexcept = delete;
    JobFbDevice &operator=(JobFbDevice &&) noexcept = delete;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobFbDevice>();
    }

    [[nodiscard]] static Ptr createShared(std::filesystem::path path)
    {
        return std::make_shared<JobFbDevice>(std::move(path));
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobFbDevice>();
    }

    [[nodiscard]] static UPtr createUniq(std::filesystem::path path)
    {
        return std::make_unique<JobFbDevice>(std::move(path));
    }

    [[nodiscard]] bool openDevice() override;
    void closeDevice() override;

    [[nodiscard]] ssize_t read(char *buffer, size_t maxlen) override;
    [[nodiscard]] ssize_t write(const char *data, size_t len) override;

    [[nodiscard]] bool isOpen() const override;
    [[nodiscard]] int fd() const override;

    void setNonBlocking(bool enabled) override;
    void setReadCallback(ReadCallback cb) override;

    [[nodiscard]] bool open(std::filesystem::path path);

    [[nodiscard]] bool refresh();

    [[nodiscard]] bool map();
    void unmap() noexcept;

    [[nodiscard]] const std::filesystem::path &path() const noexcept
    {
        return m_path;
    }

    [[nodiscard]] const JobFbFixedInfo &fixedInfo() const noexcept
    {
        return m_fixed;
    }

    [[nodiscard]] const JobFbVariableInfo &variableInfo() const noexcept
    {
        return m_variable;
    }

    [[nodiscard]] const JobFbFormatResult &formatInfo() const noexcept
    {
        return m_format;
    }

    [[nodiscard]] core::FormatType format() const noexcept
    {
        return m_format.format;
    }

    [[nodiscard]] std::uint32_t width() const noexcept
    {
        return m_variable.width();
    }

    [[nodiscard]] std::uint32_t height() const noexcept
    {
        return m_variable.height();
    }

    [[nodiscard]] std::uint32_t virtualWidth() const noexcept
    {
        return m_variable.virtualWidth();
    }

    [[nodiscard]] std::uint32_t virtualHeight() const noexcept
    {
        return m_variable.virtualHeight();
    }

    [[nodiscard]] std::uint32_t xOffset() const noexcept
    {
        return m_variable.xOffset();
    }

    [[nodiscard]] std::uint32_t yOffset() const noexcept
    {
        return m_variable.yOffset();
    }

    [[nodiscard]] std::uint32_t bitsPerPixel() const noexcept
    {
        return m_variable.bitsPerPixel();
    }

    [[nodiscard]] std::uint32_t stride() const noexcept
    {
        return m_fixed.lineLength();
    }

    [[nodiscard]] Size mappedSize() const noexcept
    {
        return static_cast<Size>(m_fixed.memoryLength());
    }

    [[nodiscard]] bool mapped() const noexcept
    {
        return m_map && m_map->isValid();
    }

    [[nodiscard]] std::byte *data() noexcept;
    [[nodiscard]] const std::byte *data() const noexcept;

    [[nodiscard]] std::span<std::byte> bytes() noexcept
    {
        return {data(), mapped() ? mappedSize() : 0};
    }

    [[nodiscard]] std::span<const std::byte> bytes() const noexcept
    {
        return {data(), mapped() ? mappedSize() : 0};
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return isOpen() && m_fixed.valid() && m_variable.valid() && m_format.valid();
    }

private:
    [[nodiscard]] bool      queryFixedInfo();
    [[nodiscard]] bool      queryVariableInfo();
    [[nodiscard]] bool      detectFormat();

    std::filesystem::path   m_path{"/dev/fb0"};
    NativeHandle            m_fd{-1};
    JobFbFixedInfo          m_fixed;
    JobFbVariableInfo       m_variable;
    JobFbFormatResult       m_format;
    io::JobMmap::UPtr       m_map;
    ReadCallback            m_readCallback;
};

}