#include "job_ggml_tensor_view.h"

#include <stdexcept>

namespace job::ggml {
JobGgmlTensorView::JobGgmlTensorView(struct ggml_tensor *tensor) :
    m_tensor{tensor}
{
    if (!m_tensor) {
        throw std::invalid_argument{
            "JobGgmlTensorView requires a valid ggml_tensor"
        };
    }
}

bool JobGgmlTensorView::isValid() const noexcept
{
    return m_tensor != nullptr;
}

bool JobGgmlTensorView::isView() const noexcept
{
    return m_tensor && ggml_is_view(m_tensor);
}

struct ggml_tensor *JobGgmlTensorView::source() noexcept
{
    return m_tensor ? m_tensor->view_src : nullptr;
}

const struct ggml_tensor *JobGgmlTensorView::source() const noexcept
{
    return m_tensor ? m_tensor->view_src : nullptr;
}

std::size_t JobGgmlTensorView::offset() const noexcept
{
    return m_tensor ? m_tensor->view_offs : 0;
}

struct ggml_tensor *JobGgmlTensorView::rootSource() noexcept
{
    if (!m_tensor)
        return nullptr;

    struct ggml_tensor *current = m_tensor;
    std::size_t currentDepth = 0;

    while (current->view_src &&
           currentDepth < MaxViewDepth) {
        current = current->view_src;
        ++currentDepth;
    }

    return current;
}

const struct ggml_tensor *JobGgmlTensorView::rootSource() const noexcept
{
    if (!m_tensor)
        return nullptr;

    const struct ggml_tensor *current = m_tensor;
    std::size_t currentDepth = 0;

    while (current->view_src &&
           currentDepth < MaxViewDepth) {
        current = current->view_src;
        ++currentDepth;
    }

    return current;
}

std::size_t JobGgmlTensorView::depth() const noexcept
{
    if (!m_tensor)
        return 0;

    const struct ggml_tensor *current = m_tensor;
    std::size_t currentDepth = 0;

    while (current->view_src && currentDepth < MaxViewDepth) {
        current = current->view_src;
        ++currentDepth;
    }

    return currentDepth;
}

bool JobGgmlTensorView::hasSource(const struct ggml_tensor *tensor) const noexcept
{
    if (!m_tensor || !tensor)
        return false;

    const struct ggml_tensor *current = m_tensor->view_src;
    std::size_t currentDepth = 0;

    while (current && currentDepth < MaxViewDepth) {
        if (current == tensor)
            return true;

        current = current->view_src;
        ++currentDepth;
    }

    return false;
}

struct ggml_tensor *JobGgmlTensorView::tensor() noexcept
{
    return m_tensor;
}

const struct ggml_tensor *JobGgmlTensorView::tensor() const noexcept
{
    return m_tensor;
}

} // namespace job::ggml