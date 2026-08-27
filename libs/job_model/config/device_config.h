#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include <job_base_obj.h>

#include <job_ggml_enums.h>

#include "jobmodel_export.h"

namespace job::model {

class JOBMODEL_EXPORT DeviceConfig : public job::core::BaseObject
{
public:
    using Ptr  = std::shared_ptr<DeviceConfig>;
    using WPtr = std::weak_ptr<DeviceConfig>;
    using UPtr = std::unique_ptr<DeviceConfig>;

    DeviceConfig();
    ~DeviceConfig() = default;

    [[nodiscard]] static Ptr createShared() { return std::make_shared<DeviceConfig>(); }
    [[nodiscard]] static UPtr createUniq()  { return std::make_unique<DeviceConfig>(); }

    DeviceConfig(const DeviceConfig &) = default;
    DeviceConfig &operator=(const DeviceConfig &) = default;
    DeviceConfig(DeviceConfig &&) noexcept = default;
    DeviceConfig &operator=(DeviceConfig &&) noexcept = default;

    [[nodiscard]] bool hasPreferredDeviceKind() const noexcept { return m_preferredDeviceKind.has_value(); }
    [[nodiscard]] std::optional<ggml::JobGgmlDeviceImpl> preferredDeviceKind() const noexcept { return m_preferredDeviceKind; }
    void setPreferredDeviceKind(ggml::JobGgmlDeviceImpl kind) noexcept { m_preferredDeviceKind = kind; }
    void clearPreferredDeviceKind() noexcept { m_preferredDeviceKind.reset(); }

    [[nodiscard]] uint64_t memoryBudgetBytes() const noexcept { return m_memoryBudgetBytes; }
    void setMemoryBudgetBytes(uint64_t bytes) noexcept { m_memoryBudgetBytes = bytes; }

    // CPU thread budget. 0 means "auto".
    [[nodiscard]] uint32_t threadBudget() const noexcept { return m_threadBudget; }
    void setThreadBudget(uint32_t count) noexcept { m_threadBudget = count; }

    [[nodiscard]] bool useMmap() const noexcept { return m_useMmap; }
    void setUseMmap(bool value) noexcept { m_useMmap = value; }

    [[nodiscard]] bool useMLock() const noexcept { return m_useMLock; }
    void setUseMLock(bool value) noexcept { m_useMLock = value; }

    [[nodiscard]] bool noAlloc() const noexcept { return m_noAlloc; }
    void setNoAlloc(bool value) noexcept { m_noAlloc = value; }

    [[nodiscard]] bool isValid() const noexcept;

private:
    std::optional<ggml::JobGgmlDeviceImpl>  m_preferredDeviceKind{ggml::JobGgmlDeviceImpl::Cuda};
    uint64_t                                m_memoryBudgetBytes{0};
    uint32_t                                m_threadBudget{0};
    bool                                    m_useMmap{true};
    bool                                    m_useMLock{false};
    bool                                    m_noAlloc{false};
};

} // namespace job::model