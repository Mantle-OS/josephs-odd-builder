#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include <ggml.h>

#include "job_ggml_tensor_batch.h"
#include "job_ggml_tensor_data.h"
#include "job_ggml_tensor_extents.h"
#include "job_ggml_tensor_fiber.h"
#include "job_ggml_tensor_layout.h"
#include "job_ggml_tensor_matrix.h"
#include "job_ggml_tensor_operation.h"
#include "job_ggml_tensor_view.h"
#include "job_ggml_tensor_volume.h"
#include "job_ggml_enums.h"

#include "jobggml_export.h"

namespace job::ggml {
class JobGgmlContext;
class JOBGGML_EXPORT JobGgmlTensor
{
public:
    using Ptr  = std::shared_ptr<JobGgmlTensor>;
    using WPtr = std::weak_ptr<JobGgmlTensor>;
    using UPtr = std::unique_ptr<JobGgmlTensor>;

    explicit JobGgmlTensor(struct ggml_tensor *tensor);
    ~JobGgmlTensor() = default;

    [[nodiscard]] static Ptr createShared(struct ggml_tensor *tensor) { return std::make_shared<JobGgmlTensor>( tensor ); }
    [[nodiscard]] static Ptr createSharedNamedTensor2d(JobGgmlContext &context,
                                                  const std::string &name,
                                                  JobGgmlType type = job::ggml::JobGgmlType::F32,
                                                  std::int64_t ne0 = 8,
                                                  std::int64_t ne1 = 4);


    [[nodiscard]] static UPtr createUniq(struct ggml_tensor *tensor) { return std::make_unique<JobGgmlTensor>(tensor); }
    [[nodiscard]] static UPtr createUniqNamedTensor2d(JobGgmlContext &context,
                                           const std::string &name,
                                           JobGgmlType type = job::ggml::JobGgmlType::F32,
                                           std::int64_t ne0 = 8,
                                           std::int64_t ne1 = 4);

    JobGgmlTensor(const JobGgmlTensor &) = delete;
    JobGgmlTensor &operator=(const JobGgmlTensor &) = delete;
    JobGgmlTensor(JobGgmlTensor &&) = delete;
    JobGgmlTensor &operator=(JobGgmlTensor &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;

    // Native tensor
    [[nodiscard]] struct ggml_tensor *tensor() noexcept;
    [[nodiscard]] const struct ggml_tensor *tensor() const noexcept;

    // Identity
    [[nodiscard]] std::string name() const;
    void setName(const std::string &name);
    [[nodiscard]] bool hasName() const noexcept;

    // Core inspection objects
    [[nodiscard]] JobGgmlTensorExtents *extents() noexcept;
    [[nodiscard]] const JobGgmlTensorExtents *extents() const noexcept;

    [[nodiscard]] JobGgmlTensorView *view() noexcept;
    [[nodiscard]] const JobGgmlTensorView *view() const noexcept;

    [[nodiscard]] JobGgmlTensorOperation *operation() noexcept;
    [[nodiscard]] const JobGgmlTensorOperation *operation() const noexcept;

    [[nodiscard]] JobGgmlTensorData *data() noexcept;
    [[nodiscard]] const JobGgmlTensorData *data() const noexcept;

    [[nodiscard]] JobGgmlTensorLayout *layout() noexcept;
    [[nodiscard]] const JobGgmlTensorLayout *layout() const noexcept;

    // Shape shortcuts
    [[nodiscard]] int rank() const noexcept;

    [[nodiscard]] std::int64_t extent(std::size_t dimension) const noexcept;

    [[nodiscard]] std::size_t stride(std::size_t dimension) const noexcept;

    [[nodiscard]] std::int64_t elementCount() const noexcept;
    [[nodiscard]] std::size_t byteCount() const noexcept;
    [[nodiscard]] std::size_t paddedByteCount() const noexcept;

    [[nodiscard]] bool isScalar() const noexcept;
    [[nodiscard]] bool isVector() const noexcept;
    [[nodiscard]] bool isMatrix() const noexcept;
    [[nodiscard]] bool isThreeDimensional() const noexcept;
    [[nodiscard]] bool isFourDimensional() const noexcept;

    // Layout shortcuts
    [[nodiscard]] JobGgmlTensorLayoutType layoutType() const noexcept;

    [[nodiscard]] bool isContiguous() const noexcept;
    [[nodiscard]] bool isContiguouslyAllocated() const noexcept;
    [[nodiscard]] bool isTransposed() const noexcept;
    [[nodiscard]] bool isPermuted() const noexcept;
    [[nodiscard]] bool isStrided() const noexcept;

    // Type and storage shortcuts
    [[nodiscard]] JobGgmlType type() const noexcept;

    [[nodiscard]] const char *typeName() const noexcept;

    [[nodiscard]] bool isQuantized() const noexcept;
    [[nodiscard]] bool hasBuffer() const noexcept;
    [[nodiscard]] bool bufferIsHost() const noexcept;
    [[nodiscard]] bool hasData() const noexcept;

    [[nodiscard]] ggml_backend_buffer_t buffer() const noexcept;

    [[nodiscard]] void *dataPointer() noexcept;
    [[nodiscard]] const void *dataPointer() const noexcept;

    // Operation shortcuts
    [[nodiscard]] JobGgmlOp tensorOperation() const noexcept;

    [[nodiscard]] bool hasOperation() const noexcept;
    [[nodiscard]] std::size_t sourceCount() const noexcept;

    // View shortcuts
    [[nodiscard]] bool isView() const noexcept;
    [[nodiscard]] std::size_t viewOffset() const noexcept;

    [[nodiscard]] struct ggml_tensor *viewSource() noexcept;
    [[nodiscard]] const struct ggml_tensor *viewSource() const noexcept;

    [[nodiscard]] struct ggml_tensor *rootViewSource() noexcept;
    [[nodiscard]] const struct ggml_tensor *rootViewSource() const noexcept;

    // Flags
    [[nodiscard]] std::int32_t flags() const noexcept;

    [[nodiscard]] bool isInput() const noexcept;
    [[nodiscard]] bool isOutput() const noexcept;
    [[nodiscard]] bool isParameter() const noexcept;
    [[nodiscard]] bool isLoss() const noexcept;
    [[nodiscard]] bool isCompute() const noexcept;

    // Shape and layout comparison
    [[nodiscard]] bool hasSameShape(const JobGgmlTensor &other) const noexcept;
    [[nodiscard]] bool hasSameStride(const JobGgmlTensor &other) const noexcept;
    [[nodiscard]] bool hasSameLayout(const JobGgmlTensor &other) const noexcept;
    [[nodiscard]] bool canRepeatTo(const JobGgmlTensor &destination) const noexcept;
    [[nodiscard]] bool canMultiplyMatricesWith(const JobGgmlTensor &other) const noexcept;

    // Rank-specific inspection wrappers
    [[nodiscard]] JobGgmlTensorFiber::UPtr asFiber();
    [[nodiscard]] JobGgmlTensorMatrix::UPtr asMatrix();
    [[nodiscard]] JobGgmlTensorVolume::UPtr asVolume();
    [[nodiscard]] JobGgmlTensorBatch::UPtr asBatch();

    [[nodiscard]] enum ggml_op ggmlOperation() const noexcept;
    [[nodiscard]] enum ggml_type ggmlType() const noexcept;
private:
    struct ggml_tensor *m_tensor{nullptr}; // Borrowed from the owning GGML context.

    JobGgmlTensorExtents::UPtr   m_extents;
    JobGgmlTensorView::UPtr      m_view;
    JobGgmlTensorOperation::UPtr m_operation;
    JobGgmlTensorData::UPtr      m_data;
    JobGgmlTensorLayout::UPtr    m_layout;
};

} // namespace job::ggml