#include "job_ggml_context_view.h"

#include <stdexcept>

namespace job::ggml {

JobGgmlContextView::JobGgmlContextView(ggml_context *context) :
    m_context{context}
{
    if (!m_context) {
        throw std::invalid_argument{
            "JobGgmlContextView requires a valid ggml_context pointer"
        };
    }
}

bool JobGgmlContextView::isValid() const noexcept
{
    return m_context != nullptr;
}

ggml_context *JobGgmlContextView::context() noexcept
{
    return m_context;
}

const ggml_context *JobGgmlContextView::context() const noexcept
{
    return m_context;
}

std::size_t JobGgmlContextView::usedMemory() const noexcept
{
    return m_context ? ggml_used_mem(m_context) : 0;
}

std::size_t JobGgmlContextView::memorySize() const noexcept
{
    return m_context ? ggml_get_mem_size(m_context) : 0;
}

std::size_t JobGgmlContextView::maxTensorSize() const noexcept
{
    return m_context ? ggml_get_max_tensor_size(m_context) : 0;
}

void *JobGgmlContextView::memoryBuffer() noexcept
{
    return m_context ? ggml_get_mem_buffer(m_context) : nullptr;
}

const void *JobGgmlContextView::memoryBuffer() const noexcept
{
    return m_context ? ggml_get_mem_buffer(m_context) : nullptr;
}

bool JobGgmlContextView::noAlloc() const noexcept
{
    return m_context && ggml_get_no_alloc(m_context);
}

JobGgmlTensor::UPtr JobGgmlContextView::firstTensor()
{
    if (!m_context)
        return nullptr;

    ggml_tensor *nativeTensor = ggml_get_first_tensor(m_context);

    return nativeTensor ? JobGgmlTensor::createUniq(nativeTensor) : nullptr;
}

JobGgmlTensor::UPtr JobGgmlContextView::nextTensor(const JobGgmlTensor &tensor)
{
    if (!m_context || !tensor.isValid())
        return nullptr;

    ggml_tensor *nativeTensor = ggml_get_next_tensor(m_context, const_cast<ggml_tensor*>(tensor.tensor()));
    return nativeTensor ? JobGgmlTensor::createUniq(nativeTensor) : nullptr;
}

JobGgmlTensor::UPtr JobGgmlContextView::tensor(const std::string &name)
{
    if (!m_context || name.empty())
        return nullptr;

    ggml_tensor *nativeTensor = ggml_get_tensor(m_context, name.c_str());

    return nativeTensor ? JobGgmlTensor::createUniq(nativeTensor) : nullptr;
}
} // namespace job::ggml