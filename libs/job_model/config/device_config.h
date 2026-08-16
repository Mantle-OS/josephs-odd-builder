#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include <job_ggml_enums.h>

#include "jobmodel_export.h"

namespace job::model {

// Device/runtime budget and placement PREFERENCE for one model instance --
// not a handle to any live device. Fully portable, value-semantic, never
// populated by a reader (GgufModelConfigReader / HfJsonModelConfigReader):
// nothing about "which physical GPU" comes from a model file, it's always
// application-supplied at load time.
//
// Resolving this against an actual JobGgmlDeviceManager -- picking a
// concrete JobGgmlDevice*, and pinning a specific uid when the caller
// wants to (e.g. multi-model / diffusion-pipeline partitioning) -- is
// JobModel::load()'s job, not this class's. This class only ever borrows
// nothing and owns nothing beyond its own plain data.
class JOBMODEL_EXPORT DeviceConfig
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

    // Preferred backend kind (Cuda, Vulkan, Cpu, ...). Absent means "no
    // preference, let the resolver pick" -- e.g. JobModel::load()'s
    // existing "try CUDA, fall back to CPU" logic, now data-driven.
    [[nodiscard]] bool hasPreferredDeviceKind() const noexcept { return m_preferredDeviceKind.has_value(); }
    [[nodiscard]] std::optional<ggml::JobGgmlDeviceImpl> preferredDeviceKind() const noexcept { return m_preferredDeviceKind; }
    void setPreferredDeviceKind(ggml::JobGgmlDeviceImpl kind) noexcept { m_preferredDeviceKind = kind; }
    void clearPreferredDeviceKind() noexcept { m_preferredDeviceKind.reset(); }

    // Ceiling this model instance is allowed to request, in bytes. 0 means
    // "no explicit budget" -- take whatever's available. This is a request
    // this model makes of the resolver, not a report of what the device has.
    [[nodiscard]] uint64_t memoryBudgetBytes() const noexcept { return m_memoryBudgetBytes; }
    void setMemoryBudgetBytes(uint64_t bytes) noexcept { m_memoryBudgetBytes = bytes; }

    // CPU thread budget. 0 means "auto".
    [[nodiscard]] uint32_t threadBudget() const noexcept { return m_threadBudget; }
    void setThreadBudget(uint32_t count) noexcept { m_threadBudget = count; }

    // Memory-map the weight file instead of reading it into a heap buffer.
    // See job_ggml's no_alloc (job_gguf_init_params.h) -- the receiving
    // end this plugs into.
    [[nodiscard]] bool useMmap() const noexcept { return m_useMmap; }
    void setUseMmap(bool value) noexcept { m_useMmap = value; }

    // Pin mmap'd/allocated weight pages resident, refusing to let the OS
    // swap them out under memory pressure.
    [[nodiscard]] bool useMLock() const noexcept { return m_useMLock; }
    void setUseMLock(bool value) noexcept { m_useMLock = value; }

    // Skip allocating a data buffer for loaded tensors -- expected when
    // weight bytes are instead being supplied via useMmap() rather than
    // copied in by job_gguf.
    [[nodiscard]] bool noAlloc() const noexcept { return m_noAlloc; }
    void setNoAlloc(bool value) noexcept { m_noAlloc = value; }

    [[nodiscard]] bool isValid() const noexcept;

private:
    std::optional<ggml::JobGgmlDeviceImpl> m_preferredDeviceKind;
    uint64_t m_memoryBudgetBytes{0};
    uint32_t m_threadBudget{0};
    bool     m_useMmap{true};
    bool     m_useMLock{false};
    bool     m_noAlloc{false};
};

} // namespace job::model