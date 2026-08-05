#include "job_ggml_tensor.h"

#include <stdexcept>

namespace job::ggml {

JobGgmlTensor::JobGgmlTensor(struct ggml_tensor *tensor) :
    m_tensor{tensor},
    m_extents{JobGgmlTensorExtents::createUniq(tensor)},
    m_view{JobGgmlTensorView::createUniq(tensor)},
    m_operation{JobGgmlTensorOperation::createUniq(tensor)},
    m_data{JobGgmlTensorData::createUniq(tensor)},
    m_layout{JobGgmlTensorLayout::createUniq(tensor)}

{
    /*
     * Each child inspection object validates the borrowed native tensor.
     * If tensor is nullptr, construction fails before a partially initialized
     * JobGgmlTensor can escape.
     */
}

bool JobGgmlTensor::isValid() const noexcept
{
    return m_tensor != nullptr &&
           m_extents &&
           m_extents->isValid() &&
           m_view &&
           m_view->isValid() &&
           m_operation &&
           m_operation->isValid() &&
           m_data &&
           m_data->isValid() &&
           m_layout &&
           m_layout->isValid();
}

struct ggml_tensor *JobGgmlTensor::tensor() noexcept
{
    return m_tensor;
}

const struct ggml_tensor *JobGgmlTensor::tensor() const noexcept
{
    return m_tensor;
}

std::string JobGgmlTensor::name() const
{
    if (!m_tensor)
        return {};

    const char *nativeName = ggml_get_name(m_tensor);

    return nativeName ? std::string{nativeName} : std::string{};
}

void JobGgmlTensor::setName(const std::string &name)
{
    if (!m_tensor) {
        throw std::runtime_error{
            "Cannot name an invalid GGML tensor"
        };
    }

    if (name.empty())
        return;

    if (this->name() == name)
        return;

    ggml_set_name(m_tensor, name.c_str());
}

bool JobGgmlTensor::hasName() const noexcept
{
    if (!m_tensor)
        return false;

    const char *nativeName = ggml_get_name(m_tensor);

    return nativeName && nativeName[0] != '\0';
}

JobGgmlTensorExtents *JobGgmlTensor::extents() noexcept
{
    return m_extents.get();
}

const JobGgmlTensorExtents *JobGgmlTensor::extents() const noexcept
{
    return m_extents.get();
}

JobGgmlTensorView *JobGgmlTensor::view() noexcept
{
    return m_view.get();
}

const JobGgmlTensorView *JobGgmlTensor::view() const noexcept
{
    return m_view.get();
}

JobGgmlTensorOperation *JobGgmlTensor::operation() noexcept
{
    return m_operation.get();
}

const JobGgmlTensorOperation *JobGgmlTensor::operation() const noexcept
{
    return m_operation.get();
}

JobGgmlTensorData *JobGgmlTensor::data() noexcept
{
    return m_data.get();
}

const JobGgmlTensorData *JobGgmlTensor::data() const noexcept
{
    return m_data.get();
}

JobGgmlTensorLayout *JobGgmlTensor::layout() noexcept
{
    return m_layout.get();
}

const JobGgmlTensorLayout *JobGgmlTensor::layout() const noexcept
{
    return m_layout.get();
}

int JobGgmlTensor::rank() const noexcept
{
    return m_extents ? m_extents->rank() : 0;
}

std::int64_t JobGgmlTensor::extent(std::size_t dimension) const noexcept
{
    return m_extents ? m_extents->extent(dimension) : 0;
}

std::size_t JobGgmlTensor::stride(std::size_t dimension) const noexcept
{
    return m_extents ? m_extents->stride(dimension) : 0;
}

std::int64_t JobGgmlTensor::elementCount() const noexcept
{
    return m_extents ? m_extents->elementCount() : 0;
}

std::size_t JobGgmlTensor::byteCount() const noexcept
{
    return m_extents ? m_extents->byteCount() : 0;
}

std::size_t JobGgmlTensor::paddedByteCount() const noexcept
{
    return m_extents ? m_extents->paddedByteCount() : 0;
}

bool JobGgmlTensor::isScalar() const noexcept
{
    return m_extents &&
           m_extents->isScalar();
}

bool JobGgmlTensor::isVector() const noexcept
{
    return m_extents && m_extents->isVector();
}

bool JobGgmlTensor::isMatrix() const noexcept
{
    return m_extents && m_extents->isMatrix();
}

bool JobGgmlTensor::isThreeDimensional() const noexcept
{
    return m_extents && m_extents->isThreeDimensional();
}

bool JobGgmlTensor::isFourDimensional() const noexcept
{
    return m_extents && m_extents->isFourDimensional();
}

JobGgmlTensorLayoutType JobGgmlTensor::layoutType() const noexcept
{
    return m_layout ? m_layout->layoutType() : JobGgmlTensorLayoutType::Unknown;
}

bool JobGgmlTensor::isContiguous() const noexcept
{
    return m_layout && m_layout->isContiguous();
}

bool JobGgmlTensor::isContiguouslyAllocated() const noexcept
{
    return m_layout && m_layout->isContiguouslyAllocated();
}

bool JobGgmlTensor::isTransposed() const noexcept
{
    return m_layout && m_layout->isTransposed();
}

bool JobGgmlTensor::isPermuted() const noexcept
{
    return m_layout && m_layout->isPermuted();
}

bool JobGgmlTensor::isStrided() const noexcept
{
    return m_layout && m_layout->isStrided();
}

JobGgmlType JobGgmlTensor::type() const noexcept
{
    return m_data ? m_data->type() : JobGgmlType::F32;
}

enum ggml_type JobGgmlTensor::ggmlType() const noexcept
{
    return m_data ? m_data->ggmlType() : GGML_TYPE_F32;
}

const char *JobGgmlTensor::typeName() const noexcept
{
    return m_data ? m_data->typeName() : "unknown";
}

bool JobGgmlTensor::isQuantized() const noexcept
{
    return m_data && m_data->isQuantized();
}

bool JobGgmlTensor::hasBuffer() const noexcept
{
    return m_data && m_data->hasBuffer();
}

bool JobGgmlTensor::bufferIsHost() const noexcept
{
    return m_data && m_data->bufferIsHost();
}

bool JobGgmlTensor::hasData() const noexcept
{
    return m_data && m_data->hasData();
}

ggml_backend_buffer_t JobGgmlTensor::buffer() const noexcept
{
    return m_data ? m_data->buffer() : nullptr;
}

void *JobGgmlTensor::dataPointer() noexcept
{
    return m_data ? m_data->data() : nullptr;
}

const void *JobGgmlTensor::dataPointer() const noexcept
{
    return m_data ? m_data->data() : nullptr;
}

JobGgmlOp JobGgmlTensor::tensorOperation() const noexcept
{
    return m_operation ? m_operation->operation() : JobGgmlOp::None;
}

enum ggml_op JobGgmlTensor::ggmlOperation() const noexcept
{
    return m_operation ? m_operation->ggmlOperation() : GGML_OP_NONE;
}

bool JobGgmlTensor::hasOperation() const noexcept
{
    return m_operation && m_operation->hasOperation();
}

std::size_t JobGgmlTensor::sourceCount() const noexcept
{
    return m_operation ? m_operation->sourceCount() : 0;
}

bool JobGgmlTensor::isView() const noexcept
{
    return m_view && m_view->isView();
}

std::size_t JobGgmlTensor::viewOffset() const noexcept
{
    return m_view ? m_view->offset() : 0;
}

struct ggml_tensor *JobGgmlTensor::viewSource() noexcept
{
    return m_view ? m_view->source() : nullptr;
}

const struct ggml_tensor *JobGgmlTensor::viewSource() const noexcept
{
    return m_view ? m_view->source() : nullptr;
}

struct ggml_tensor *JobGgmlTensor::rootViewSource() noexcept
{
    return m_view ? m_view->rootSource() : nullptr;
}

const struct ggml_tensor *JobGgmlTensor::rootViewSource() const noexcept
{
    return m_view ? m_view->rootSource() : nullptr;
}

std::int32_t JobGgmlTensor::flags() const noexcept
{
    return m_data ? m_data->flags() : 0;
}

bool JobGgmlTensor::isInput() const noexcept
{
    return m_data && m_data->isInput();
}

bool JobGgmlTensor::isOutput() const noexcept
{
    return m_data && m_data->isOutput();
}

bool JobGgmlTensor::isParameter() const noexcept
{
    return m_data && m_data->isParameter();
}

bool JobGgmlTensor::isLoss() const noexcept
{
    return m_data && m_data->isLoss();
}

bool JobGgmlTensor::isCompute() const noexcept
{
    return m_data && m_data->isCompute();
}

bool JobGgmlTensor::hasSameShape(const JobGgmlTensor &other) const noexcept
{
    if (!m_extents || !other.m_extents)
        return false;

    return m_extents->hasSameShape(*other.m_extents);
}

bool JobGgmlTensor::hasSameStride(const JobGgmlTensor &other) const noexcept
{
    if (!m_layout || !other.m_layout)
        return false;

    return m_layout->hasSameStride(*other.m_layout);
}

bool JobGgmlTensor::hasSameLayout(const JobGgmlTensor &other) const noexcept
{
    if (!m_layout || !other.m_layout)
        return false;

    return m_layout->hasSameLayout(*other.m_layout);
}

bool JobGgmlTensor::canRepeatTo(const JobGgmlTensor &destination) const noexcept
{
    if (!m_extents || !destination.m_extents)
        return false;


    return m_extents->canRepeatTo(*destination.m_extents);
}

bool JobGgmlTensor::canMultiplyMatricesWith(const JobGgmlTensor &other) const noexcept
{
    if (!m_extents || !other.m_extents)
        return false;

    return m_extents->canMultiplyMatricesWith(*other.m_extents);
}

JobGgmlTensorFiber::UPtr JobGgmlTensor::asFiber()
{
    if (!m_tensor || rank() != 1)
        return nullptr;
    return JobGgmlTensorFiber::createUniq(m_tensor);
}

JobGgmlTensorMatrix::UPtr JobGgmlTensor::asMatrix()
{
    if (!m_tensor || rank() != 2)
        return nullptr;
    return JobGgmlTensorMatrix::createUniq(m_tensor);
}

JobGgmlTensorVolume::UPtr JobGgmlTensor::asVolume()
{
    if (!m_tensor || rank() != 3)
        return nullptr;
    return JobGgmlTensorVolume::createUniq(m_tensor);
}

JobGgmlTensorBatch::UPtr JobGgmlTensor::asBatch()
{
    if (!m_tensor || rank() != 4)
        return nullptr;
    return JobGgmlTensorBatch::createUniq(m_tensor);
}

} // namespace job::ggml