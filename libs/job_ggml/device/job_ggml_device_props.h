#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <ggml-backend.h>

#include "job_ggml_device_caps.h"
#include "job_ggml_enums.h"
#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlDeviceProps
{
public:
    using Ptr  = std::shared_ptr<JobGgmlDeviceProps>;
    using UPtr = std::unique_ptr<JobGgmlDeviceProps>;
    using WPtr = std::weak_ptr<JobGgmlDeviceProps>;

    explicit JobGgmlDeviceProps(ggml_backend_dev_props deviceProps);
    ~JobGgmlDeviceProps() = default;

    [[nodiscard]] static Ptr createShared(ggml_backend_dev_props deviceProps) { return std::make_shared<JobGgmlDeviceProps>(deviceProps); }
    [[nodiscard]] static UPtr createUniq(ggml_backend_dev_props deviceProps) { return std::make_unique<JobGgmlDeviceProps>(deviceProps); }

    JobGgmlDeviceProps(const JobGgmlDeviceProps &) = delete;
    JobGgmlDeviceProps &operator=(const JobGgmlDeviceProps &) = delete;
    JobGgmlDeviceProps(JobGgmlDeviceProps &&) = delete;
    JobGgmlDeviceProps &operator=(JobGgmlDeviceProps &&) = delete;

    [[nodiscard]] bool operator==(const JobGgmlDeviceProps &other) const noexcept;
    [[nodiscard]] bool operator!=(const JobGgmlDeviceProps &other) const noexcept;

    [[nodiscard]] JobGgmlDeviceType deviceType() const noexcept;
    void setDeviceType(JobGgmlDeviceType type);

    [[nodiscard]] const std::string &name() const noexcept;
    void setName(const std::string &name);

    [[nodiscard]] const std::string &description() const noexcept;
    void setDescription(const std::string &description);

    const std::size_t &memoryFree() const noexcept;
    void setMemoryFree(const std::size_t &memoryFree);

    [[nodiscard]] const std::size_t &memoryTotal() const noexcept;
    void setMemoryTotal(const std::size_t &memoryTotal);

    [[nodiscard]] const std::string &deviceId() const noexcept;
    void setDeviceId(const std::string &deviceId);

    [[nodiscard]] JobGgmlDeviceCaps *caps() noexcept;
    [[nodiscard]] const JobGgmlDeviceCaps *caps() const noexcept;
    void setCaps(JobGgmlDeviceCaps::UPtr caps);

    void setProps(ggml_backend_dev_props other);
    [[nodiscard]] ggml_backend_dev_props props();
    void resetProps();

private:
    [[nodiscard]] static constexpr ggml_backend_dev_props defaultProps() noexcept
    {
        return {
            nullptr,
            nullptr,
            0,
            0,
            GGML_BACKEND_DEVICE_TYPE_CPU,
            nullptr,
            {
                false,
                false,
                false,
                false
            }
        };
    }

    ggml_backend_dev_props      m_props{defaultProps()};

    [[nodiscard]] enum ggml_backend_dev_type type() const noexcept;
    void setGgmlType(enum ggml_backend_dev_type type);
    enum ggml_backend_dev_type  m_ggmlDeviceType{GGML_BACKEND_DEVICE_TYPE_CPU};

    std::string                 m_name{"cpu"};
    std::string                 m_description{"unknown"};
    std::size_t                 m_memoryFree{0};
    std::size_t                 m_memoryTotal{0};
    std::string                 m_deviceId{"unknown"};

    JobGgmlDeviceCaps::UPtr m_caps{JobGgmlDeviceCaps::createUniq(defaultProps().caps)};
};

} // namespace job::ggml