#pragma once

#include <cstddef>
#include <memory>

#include <ggml.h>

#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlInitParams
{
public:
    using Ptr  = std::shared_ptr<JobGgmlInitParams>;
    using UPtr = std::unique_ptr<JobGgmlInitParams>;

    explicit JobGgmlInitParams(const ggml_init_params &initParams = defaultInitParams());
    // explicit JobGgmlInitParams(std::size_t mem, void *buffer = nullptr , bool noAlloc = true);
    // explicit JobGgmlInitParams(std::verctor<JobGgmlTensor> tensors , std::size_t graphSize = GGML_DEFAULT_GRAPH_SIZE, bool gradients = false);

    ~JobGgmlInitParams() = default;

    [[nodiscard]] static Ptr createShared(const ggml_init_params &initParams = defaultInitParams() ) { return std::make_shared<JobGgmlInitParams>(initParams); }
    [[nodiscard]] static UPtr createUniq(const ggml_init_params &initParams = defaultInitParams() ) { return std::make_unique<JobGgmlInitParams>(initParams);}

    JobGgmlInitParams(const JobGgmlInitParams &) = delete;
    JobGgmlInitParams &operator=(const JobGgmlInitParams &) = delete;
    JobGgmlInitParams(JobGgmlInitParams &&) = delete;
    JobGgmlInitParams &operator=(JobGgmlInitParams &&) = delete;

    [[nodiscard]] std::size_t memorySize() const noexcept;
    void setMemorySize(std::size_t memorySize) noexcept;

    [[nodiscard]] void *memoryBuffer() const noexcept;
    void setMemoryBuffer(void *memoryBuffer) noexcept;

    [[nodiscard]] bool noAlloc() const noexcept;
    void setNoAlloc(bool noAlloc) noexcept;

    void setInitParams(const ggml_init_params &other) noexcept;
    [[nodiscard]] ggml_init_params initParams() noexcept;
    void resetInitParams() noexcept;

private:
    [[nodiscard]] static constexpr ggml_init_params defaultInitParams() noexcept
    {
        return {
            0,
            nullptr,
            false
        };
    }

    ggml_init_params m_initParams{defaultInitParams()};
    std::size_t      m_memorySize{0};
    void            *m_memoryBuffer{nullptr}; // Borrowed; ownership remains with the caller.
    bool             m_noAlloc{false};
};

} // namespace job::ggml
