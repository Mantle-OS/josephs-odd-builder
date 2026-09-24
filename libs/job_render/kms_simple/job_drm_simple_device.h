#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>

#include <xf86drmMode.h>

#include <io_base.h>

namespace job::render {

class JobDrmSimpleDevice final : public io::IODevice
{
public:
    using Ptr  = std::shared_ptr<JobDrmSimpleDevice>;
    using WPtr = std::weak_ptr<JobDrmSimpleDevice>;
    using UPtr = std::unique_ptr<JobDrmSimpleDevice>;

    JobDrmSimpleDevice();
    explicit JobDrmSimpleDevice(std::string path);
    ~JobDrmSimpleDevice() override;

    JobDrmSimpleDevice(const JobDrmSimpleDevice &) = delete;
    JobDrmSimpleDevice &operator=(const JobDrmSimpleDevice &) = delete;
    JobDrmSimpleDevice(JobDrmSimpleDevice &&) = delete;
    JobDrmSimpleDevice &operator=(JobDrmSimpleDevice &&) = delete;

    [[nodiscard]] static Ptr createShared();
    [[nodiscard]] static Ptr createShared(std::string path);

    [[nodiscard]] static UPtr createUniq();
    [[nodiscard]] static UPtr createUniq(std::string path);

    [[nodiscard]] bool openDevice() override;
    void closeDevice() override;

    [[nodiscard]] ssize_t read(char *buffer, std::size_t maxlen) override;
    [[nodiscard]] ssize_t write(const char *data, std::size_t len) override;

    [[nodiscard]] bool isOpen() const override;
    [[nodiscard]] int fd() const override;

    void setNonBlocking(bool enabled) override;
    void setReadCallback(ReadCallback callback) override;

    [[nodiscard]] const std::string &path() const noexcept;
    void setPath(std::string path);

    [[nodiscard]] bool refreshResources();

    [[nodiscard]] drmModeRes *resources() noexcept;
    [[nodiscard]] const drmModeRes *resources() const noexcept;

    [[nodiscard]] std::size_t fbCount() const noexcept;
    [[nodiscard]] std::span<const std::uint32_t> fbs() const noexcept;

    [[nodiscard]] std::size_t crtcCount() const noexcept;
    [[nodiscard]] std::span<const std::uint32_t> crtcs() const noexcept;

    [[nodiscard]] std::size_t connectorCount() const noexcept;
    [[nodiscard]] std::span<const std::uint32_t> connectors() const noexcept;

    // So JobDrmSimpleDevice::connector(id) eventually needs to be race-aware rather than just:


    [[nodiscard]] std::size_t encoderCount() const noexcept;
    [[nodiscard]] std::span<const std::uint32_t> encoders() const noexcept;

    [[nodiscard]] std::uint32_t minWidth() const noexcept;
    [[nodiscard]] std::uint32_t maxWidth() const noexcept;

    [[nodiscard]] std::uint32_t minHeight() const noexcept;
    [[nodiscard]] std::uint32_t maxHeight() const noexcept;

    [[nodiscard]] bool dumbBuffers() const noexcept;
    [[nodiscard]] bool atomic() const noexcept;

private:
    struct DrmModeResDeleter
    {
        void operator()(drmModeRes *resources) const noexcept;
    };
    using DrmModeResPtr = std::unique_ptr<drmModeRes, DrmModeResDeleter>;
    void clearResources() noexcept;
    void readCapabilities();

    std::string     m_path{"/dev/dri/card0"};
    int             m_fd{-1};
    DrmModeResPtr   m_resources;
    ReadCallback    m_readCallback;
    bool            m_dumbBuffers{false};
    bool            m_atomic{false};
};

} // namespace job::render