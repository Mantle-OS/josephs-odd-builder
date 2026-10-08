#pragma once

#include <filesystem>
#include <memory>
#include <cstdint>

#include "job_input_device.h"
#include "job_input_event.h"

namespace job::input {

namespace fs = std::filesystem;


class JobInputUInputDevice
{

public:
    using Ptr  = std::shared_ptr<JobInputUInputDevice>;
    using WPtr = std::weak_ptr<JobInputUInputDevice>;
    using UPtr = std::unique_ptr<JobInputUInputDevice>;

    inline static const fs::path kDefaultUInputPath{"/dev/uinput"};

    explicit JobInputUInputDevice(JobInputDevice device, fs::path path = kDefaultUInputPath);

    ~JobInputUInputDevice();

    JobInputUInputDevice(const JobInputUInputDevice &) = delete;
    JobInputUInputDevice &operator=(const JobInputUInputDevice &) = delete;

    JobInputUInputDevice(JobInputUInputDevice &&other) noexcept;
    JobInputUInputDevice &operator=(JobInputUInputDevice &&other) noexcept;

    [[nodiscard]] static Ptr createShared(JobInputDevice device, fs::path path = kDefaultUInputPath);
    [[nodiscard]] static UPtr createUniq(JobInputDevice device, fs::path path = kDefaultUInputPath);

    [[nodiscard]] bool create();
    void destroy();

    [[nodiscard]] bool send(const JobInputEvent &event);
    [[nodiscard]] bool sync();

    [[nodiscard]] bool isCreated() const noexcept;
    [[nodiscard]] int fd() const noexcept;

    [[nodiscard]] const JobInputDevice &device() const noexcept;
    [[nodiscard]] const fs::path &path() const noexcept;

private:
    [[nodiscard]] bool openDevice();
    void closeDevice() noexcept;

    [[nodiscard]] bool configureCapabilities();
    [[nodiscard]] bool configureIdentity();
    [[nodiscard]] bool createKernelDevice();

    [[nodiscard]] bool send(JobInputType type, JobInputCode code, JobInputValue value);

    void reset() noexcept;
    void moveFrom(JobInputUInputDevice &&other) noexcept;

    JobInputDevice m_device;
    fs::path       m_path;
    int            m_fd{-1};
    bool           m_created{false};
};

} // namespace job::input