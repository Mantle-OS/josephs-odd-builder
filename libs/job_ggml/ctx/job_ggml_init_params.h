#pragma once

#include <cstddef>
#include <memory>

#include <ggml.h>

#include "jobggml_export.h"

namespace job::ggml {

inline constexpr std::size_t kDefaultMetadataPadding = 4096;

class JOBGGML_EXPORT JobGgmlInitParams
{
public:
    using Ptr  = std::shared_ptr<JobGgmlInitParams>;
    using UPtr = std::unique_ptr<JobGgmlInitParams>;

    explicit JobGgmlInitParams(std::size_t mem, void *buffer = nullptr , bool noAlloc = true);
    explicit JobGgmlInitParams(const ggml_init_params &initParams = defaultInitParams());

    ~JobGgmlInitParams() = default;
    //
    [[nodiscard]] static Ptr createShared(std::size_t mem, void *buffer = nullptr , bool noAlloc = true)
    {
        return std::make_shared<JobGgmlInitParams>(ggml_init_params{mem, buffer, noAlloc});
    }
    [[nodiscard]] static Ptr createShared(const ggml_init_params &initParams = defaultInitParams() )
    {
        return std::make_shared<JobGgmlInitParams>(initParams);
    }

    // Meta data and ctx helpers
    [[nodiscard]] static Ptr createMetadata(std::size_t memorySize = std::size_t{4096}, bool noAlloc = true)
    {
        return std::make_shared<JobGgmlInitParams>(memorySize, nullptr, noAlloc);
    }
    [[nodiscard]] static Ptr createMetadataFor(std::size_t tensors,
                                           std::size_t graphSize = GGML_DEFAULT_GRAPH_SIZE,
                                           bool grad = false,
                                           std::size_t pad = kDefaultMetadataPadding,
                                           bool noAlloc = true)
    {

        const auto sz = JobGgmlInitParams::estCtxCost(tensors, graphSize, grad, pad);
        return std::make_shared<JobGgmlInitParams>(sz, nullptr, noAlloc);
    }

    ////// UNIQ
    [[nodiscard]] static UPtr createUniq(std::size_t mem, void *buffer = nullptr , bool noAlloc = true)
    {
        return std::make_unique<JobGgmlInitParams>(ggml_init_params{mem, buffer, noAlloc});
    }
    [[nodiscard]] static UPtr createUniq(const ggml_init_params &initParams = defaultInitParams() )
    {
        return std::make_unique<JobGgmlInitParams>(initParams);
    }
    // Meta data and ctx helpers
    [[nodiscard]] static UPtr createUniqMetadata(std::size_t memorySize = std::size_t{GGML_DEFAULT_GRAPH_SIZE}, bool noAlloc = true)
    {
        return std::make_unique<JobGgmlInitParams>(memorySize, nullptr, noAlloc);
    }
    [[nodiscard]] static UPtr createUniqMetadataFor(std::size_t tensors,
                                            std::size_t graphSize = GGML_DEFAULT_GRAPH_SIZE,
                                            bool grad = false,
                                            std::size_t pad = kDefaultMetadataPadding,
                                            bool noAlloc = true)
    {

        const auto sz = JobGgmlInitParams::estCtxCost(tensors, graphSize, grad, pad);
        return std::make_unique<JobGgmlInitParams>(sz, nullptr, noAlloc);
    }


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

    [[nodiscard]] static std::size_t estCtxCost(std::size_t tensors,
                                                std::size_t graphSize = GGML_DEFAULT_GRAPH_SIZE,
                                                bool grad = false,
                                                std::size_t pad = kDefaultMetadataPadding) noexcept;


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
