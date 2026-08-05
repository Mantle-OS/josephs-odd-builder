#pragma once

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include <ggml-alloc.h>

#include "job_ggml_backend_buffer_type.h"
#include "job_ggml_cgraph.h"
#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlGraphAllocator
{
public:
    using Ptr  = std::shared_ptr<JobGgmlGraphAllocator>;
    using WPtr = std::weak_ptr<JobGgmlGraphAllocator>;
    using UPtr = std::unique_ptr<JobGgmlGraphAllocator>;

    explicit JobGgmlGraphAllocator(JobGgmlBackendBufferType::Ptr bufferType);

    explicit JobGgmlGraphAllocator(std::vector<JobGgmlBackendBufferType::Ptr> bufferTypes);

    ~JobGgmlGraphAllocator();

    [[nodiscard]] static Ptr createShared( JobGgmlBackendBufferType::Ptr bufferType ) { return std::make_shared<JobGgmlGraphAllocator>( std::move(bufferType) ); }
    [[nodiscard]] static Ptr createShared( std::vector<JobGgmlBackendBufferType::Ptr> bufferTypes ) { return std::make_shared<JobGgmlGraphAllocator>( std::move(bufferTypes) ); }

    [[nodiscard]] static UPtr createUniq( JobGgmlBackendBufferType::Ptr bufferType ) { return std::make_unique<JobGgmlGraphAllocator>( std::move(bufferType) ); }
    [[nodiscard]] static UPtr createUniq( std::vector<JobGgmlBackendBufferType::Ptr> bufferTypes ) { return std::make_unique<JobGgmlGraphAllocator>( std::move(bufferTypes) ); }

    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] ggml_gallocr_t graphAllocator() const noexcept;

    [[nodiscard]] std::size_t bufferCount() const noexcept;

    [[nodiscard]] JobGgmlBackendBufferType::Ptr bufferType(std::size_t index) const noexcept;

    [[nodiscard]] bool reserve( JobGgmlCGraph &graph);
    void reserveSize(JobGgmlCGraph &graph, const std::vector<int> &nodeBufferIds, const std::vector<int> &leafBufferIds, std::vector<std::size_t> &sizes );

    [[nodiscard]] bool reserve(JobGgmlCGraph &graph, const std::vector<int> &nodeBufferIds, const std::vector<int> &leafBufferIds);

    [[nodiscard]] bool allocateGraph(JobGgmlCGraph &graph);

    [[nodiscard]] std::size_t bufferSize(int bufferId) const noexcept;

    void reset(JobGgmlBackendBufferType::Ptr bufferType);
    void reset(std::vector<JobGgmlBackendBufferType::Ptr> bufferTypes);

    JobGgmlGraphAllocator(const JobGgmlGraphAllocator &) = delete;
    JobGgmlGraphAllocator &operator=(const JobGgmlGraphAllocator &) = delete;
    JobGgmlGraphAllocator(JobGgmlGraphAllocator &&) = delete;
    JobGgmlGraphAllocator &operator=(JobGgmlGraphAllocator &&) = delete;

private:
    void clear() noexcept;
    void initialize(std::vector<JobGgmlBackendBufferType::Ptr> bufferTypes);
    [[nodiscard]] bool validateAssignments(const JobGgmlCGraph &graph, const std::vector<int> &nodeBufferIds, const std::vector<int> &leafBufferIds) const noexcept;

    std::vector<JobGgmlBackendBufferType::Ptr> m_bufferTypes;
    ggml_gallocr_t                             m_graphAllocator{nullptr};
};

} // namespace job::ggml