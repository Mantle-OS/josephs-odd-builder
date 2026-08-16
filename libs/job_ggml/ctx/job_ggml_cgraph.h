#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <ggml.h>


#include "job_ggml_tensor.h"
#include "jobggml_export.h"

namespace job::ggml {
class JobGgmlContext;
// class JobGgmlTensor;
class JOBGGML_EXPORT JobGgmlCGraph
{
public:
    using Ptr  = std::shared_ptr<JobGgmlCGraph>;
    using UPtr = std::unique_ptr<JobGgmlCGraph>;

    explicit JobGgmlCGraph(ggml_cgraph *graph);
    ~JobGgmlCGraph() = default;

    [[nodiscard]] static Ptr createShared(ggml_cgraph *graph) { return std::make_shared<JobGgmlCGraph>(graph); }
    [[nodiscard]] static UPtr createUniq(ggml_cgraph *graph) { return std::make_unique<JobGgmlCGraph>(graph); }

    JobGgmlCGraph(const JobGgmlCGraph &) = delete;
    JobGgmlCGraph &operator=(const JobGgmlCGraph &) = delete;
    JobGgmlCGraph(JobGgmlCGraph &&) = delete;
    JobGgmlCGraph &operator=(JobGgmlCGraph &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] int size() const noexcept;
    [[nodiscard]] int nodeCount() const noexcept;

    /*
     * [[FORWARD_PORT]] JOSEPH: KEEP AN EYE UPSTREAM
     * GGML does not currently expose ggml_graph_n_leafs().
     * Keep tracking upstream before adding leafCount().
     */

    [[nodiscard]] JobGgmlTensor::UPtr node(int index) const;
    [[nodiscard]] std::vector<JobGgmlTensor::UPtr> nodes() const;

    [[nodiscard]] JobGgmlTensor::UPtr tensor(const std::string &name) const;

    [[nodiscard]] JobGgmlTensor::UPtr gradient(const JobGgmlTensor &node) const;

    [[nodiscard]] JobGgmlTensor::UPtr gradientAccumulator(const JobGgmlTensor &node) const;

    [[nodiscard]] JobGgmlTensor::UPtr buildForwardSelect(const std::vector<JobGgmlTensor *> &tensors, int index);

    void buildForwardExpand(JobGgmlTensor &tensor);
    void buildBackwardExpand(JobGgmlContext &context, const std::vector<JobGgmlTensor*> &gradientAccumulators = {});

    void addNode(JobGgmlTensor &tensor);

    void reset() noexcept;
    void clear() noexcept;

    void print() const noexcept;

    void dumpDot(const std::string &fileName, const JobGgmlCGraph *forwardGraph = nullptr) const;

    [[nodiscard]] ggml_cgraph *graph() noexcept;
    [[nodiscard]] const ggml_cgraph *graph() const noexcept;

    [[nodiscard]] static std::size_t overhead() noexcept;

    [[nodiscard]] static std::size_t overheadCustom(std::size_t size, bool gradients) noexcept;

private:
    ggml_cgraph *m_graph{nullptr}; // Borrowed from the owning GGML context.
};

} // namespace job::ggml