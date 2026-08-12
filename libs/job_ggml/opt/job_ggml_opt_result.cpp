#include "job_ggml_opt_result.h"

#include <limits>
#include <stdexcept>

namespace job::ggml {

JobGgmlOptResult::JobGgmlOptResult() :
    m_result{ggml_opt_result_init()}
{
    if (!m_result) {
        throw std::runtime_error{
            "Failed to initialize the GGML optimization result"
        };
    }
}

JobGgmlOptResult::~JobGgmlOptResult()
{
    if (m_result) {
        ggml_opt_result_free(m_result);
        m_result = nullptr;
    }
}

bool JobGgmlOptResult::isValid() const noexcept
{
    return m_result != nullptr;
}

bool JobGgmlOptResult::isEmpty() const noexcept
{
    return ndata() == 0;
}

std::int64_t JobGgmlOptResult::ndata() const noexcept
{
    if (!m_result)
        return 0;

    std::int64_t value{0};
    ggml_opt_result_ndata(m_result, &value);

    return value;
}

double JobGgmlOptResult::loss(double *uncertainty) const noexcept
{
    if (!m_result) {
        if (uncertainty)
            *uncertainty = std::numeric_limits<double>::quiet_NaN();

        return 0.0;
    }

    double value{0.0};

    /*
     * Upstream documents the uncertainty pointer as optional, but currently
     * dereferences it when no batches have been accumulated. Always provide a
     * local destination and copy it to the caller only when requested.
     */
    double nativeUncertainty{ std::numeric_limits<double>::quiet_NaN() };

    ggml_opt_result_loss(m_result, &value, &nativeUncertainty);

    if (uncertainty)
        *uncertainty = nativeUncertainty;

    return value;
}

std::vector<std::int32_t> JobGgmlOptResult::predictions() const
{
    if (!m_result) {
        throw std::runtime_error{
            "Cannot read predictions from an invalid GGML optimization result"
        };
    }

    const std::int64_t count = ndata();

    if (count <= 0)
        return {};

    const auto maxSize = static_cast<std::uint64_t>(std::vector<std::int32_t>{}.max_size());
    const auto predictionCount = static_cast<std::uint64_t>(count);

    if (predictionCount > maxSize) {
        throw std::length_error{
            "GGML optimization result prediction count exceeds vector capacity"
        };
    }

    std::vector<std::int32_t> values(static_cast<std::size_t>(count));
    ggml_opt_result_pred(m_result, values.data());

    return values;
}

double JobGgmlOptResult::accuracy(double *uncertainty) const noexcept
{
    if (!m_result) {
        if (uncertainty) {
            *uncertainty =
                std::numeric_limits<double>::quiet_NaN();
        }

        return std::numeric_limits<double>::quiet_NaN();
    }

    double value{ std::numeric_limits<double>::quiet_NaN() };
    double nativeUncertainty{ std::numeric_limits<double>::quiet_NaN() };
    ggml_opt_result_accuracy(m_result, &value, &nativeUncertainty);

    if (uncertainty)
        *uncertainty = nativeUncertainty;

    return value;
}

void JobGgmlOptResult::reset() noexcept
{
    if (m_result)
        ggml_opt_result_reset(m_result);
}

ggml_opt_result_t JobGgmlOptResult::result() noexcept
{
    return m_result;
}

const struct ggml_opt_result *JobGgmlOptResult::result() const noexcept
{
    return m_result;
}

} // namespace job::ggml