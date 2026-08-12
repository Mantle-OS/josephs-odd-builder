#include "job_ggml_tensor_operation.h"

#include <stdexcept>

namespace job::ggml {

JobGgmlTensorOperation::JobGgmlTensorOperation(struct ggml_tensor *tensor) :
    m_tensor{tensor}
{
    if (!m_tensor) {
        throw std::invalid_argument{
            "JobGgmlTensorOperation requires a valid ggml_tensor"
        };
    }
}

bool JobGgmlTensorOperation::isValid() const noexcept
{
    return m_tensor != nullptr;
}

JobGgmlOp JobGgmlTensorOperation::operation() const noexcept
{
    return static_cast<JobGgmlOp>(ggmlOperation());
}

enum ggml_op JobGgmlTensorOperation::ggmlOperation() const noexcept
{
    return m_tensor ? m_tensor->op : GGML_OP_NONE;
}

bool JobGgmlTensorOperation::hasOperation() const noexcept
{
    return ggmlOperation() != GGML_OP_NONE;
}

bool JobGgmlTensorOperation::isOperation(JobGgmlOp operation) const noexcept
{
    return this->operation() == operation;
}

const char *JobGgmlTensorOperation::operationName() const noexcept
{
    if (!m_tensor)
        return "NONE";

    const char *name = ggml_op_name(m_tensor->op);

    return name ? name : "UNKNOWN";
}

const char *JobGgmlTensorOperation::operationSymbol() const noexcept
{
    if (!m_tensor)
        return "";

    const char *symbol = ggml_op_symbol(m_tensor->op);

    return symbol ? symbol : "";
}

std::int32_t JobGgmlTensorOperation::flags() const noexcept
{
    return m_tensor ? m_tensor->flags : 0;
}

std::size_t JobGgmlTensorOperation::sourceCount() const noexcept
{
    if (!m_tensor)
        return 0;

    std::size_t count = 0;

    for (std::size_t i = 0; i < MaxSources; ++i) {
        if (!m_tensor->src[i])
            continue;

        ++count;
    }

    return count;
}

bool JobGgmlTensorOperation::hasSources() const noexcept
{
    return sourceCount() > 0;
}

struct ggml_tensor *JobGgmlTensorOperation::source(std::size_t index) noexcept
{
    if (!validSourceIndex(index))
        return nullptr;

    return m_tensor->src[index];
}

const struct ggml_tensor *JobGgmlTensorOperation::source(std::size_t index) const noexcept
{
    if (!validSourceIndex(index))
        return nullptr;

    return m_tensor->src[index];
}

bool JobGgmlTensorOperation::hasSource(const struct ggml_tensor *tensor) const noexcept
{
    if (!m_tensor || !tensor)
        return false;

    for (std::size_t i = 0; i < MaxSources; ++i) {
        if (m_tensor->src[i] == tensor)
            return true;
    }

    return false;
}

std::array<struct ggml_tensor *, JobGgmlTensorOperation::MaxSources>
JobGgmlTensorOperation::sources() noexcept
{
    std::array<struct ggml_tensor *,MaxSources> ret{};

    // derefence here might cause issues late on JOSEPH COME BACK
    if (!m_tensor)
        return ret;

    for (std::size_t i = 0; i < MaxSources; ++i)
        ret[i] = m_tensor->src[i];

    return ret;
}

std::array<const struct ggml_tensor *, JobGgmlTensorOperation::MaxSources> JobGgmlTensorOperation::sources() const noexcept
{
    std::array<const struct ggml_tensor *,MaxSources> ret{};

    // derefence here might cause issues late on JOSEPH COME BACK
    if (!m_tensor)
        return ret;

    for (std::size_t i = 0; i < MaxSources; ++i)
        ret[i] = m_tensor->src[i];

    return ret;
}

std::int32_t JobGgmlTensorOperation::parameter(std::size_t index) const noexcept
{
    if (!m_tensor || index >= ParameterCount)
        return 0;

    return m_tensor->op_params[index];
}

std::array<std::int32_t, JobGgmlTensorOperation::ParameterCount> JobGgmlTensorOperation::parameters() const noexcept
{
    std::array<
        std::int32_t,
        ParameterCount
        > ret{};

    if (!m_tensor)
        return ret;

    for (std::size_t i = 0; i < ParameterCount; ++i)
        ret[i] = m_tensor->op_params[i];

    return ret;
}

const std::int32_t *JobGgmlTensorOperation::parameterData() const noexcept
{
    return m_tensor ? m_tensor->op_params : nullptr;
}

std::size_t JobGgmlTensorOperation::parameterCount() const noexcept
{
    return ParameterCount;
}

std::size_t JobGgmlTensorOperation::parameterByteSize() const noexcept
{
    return ParameterBytes;
}

JobGgmlUnaryOp JobGgmlTensorOperation::unaryOperation() const noexcept
{
    if (!isUnaryOperation())
        return JobGgmlUnaryOp::Abs;

    return static_cast<JobGgmlUnaryOp>(ggml_get_unary_op(m_tensor));
}

JobGgmlGluOp JobGgmlTensorOperation::gluOperation() const noexcept
{
    if (!isGluOperation())
        return JobGgmlGluOp::ReGlu;

    return static_cast<JobGgmlGluOp>(ggml_get_glu_op(m_tensor));
}

bool JobGgmlTensorOperation::isUnaryOperation() const noexcept
{
    return m_tensor && m_tensor->op == GGML_OP_UNARY;
}

bool JobGgmlTensorOperation::isGluOperation() const noexcept
{
    return m_tensor && m_tensor->op == GGML_OP_GLU;
}

struct ggml_tensor *JobGgmlTensorOperation::tensor() noexcept
{
    return m_tensor;
}

const struct ggml_tensor *JobGgmlTensorOperation::tensor() const noexcept
{
    return m_tensor;
}

bool JobGgmlTensorOperation::validSourceIndex(std::size_t index) const noexcept
{
    return m_tensor &&
           index < MaxSources;
}

} // namespace job::ggml