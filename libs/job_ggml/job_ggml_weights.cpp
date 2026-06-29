#include "job_ggml_weights.h"

#include <ggml.h>
#include <gguf.h>

#include <job_logger.h>
namespace job::ggml {

JobGgmlWeights::JobGgmlWeights(JobGgmlDevice &device) :
    m_device(device)
{
    struct ggml_init_params params{};
    params.mem_size   = 0;      // GGUF controls allocation
    params.mem_buffer = nullptr;
    params.no_alloc   = true;   // GGUF provides memory layout

    m_ctx = ggml_context_ptr{ggml_init(params)};

    if (!m_ctx)
        throw std::runtime_error("JobGgmlWeights: failed to init context");
}

// bool JobGgmlWeights::loadFromGGUF(gguf_context_ptr &ggufCtx) {
//     if (!ggufCtx)
//         return false;

//     const int n_tensors = gguf_get_n_tensors(ggufCtx.get());

//     // 1. We must create the tensors in our context first
//     for (int i = 0; i < n_tensors; ++i) {
//         const char * name = gguf_get_tensor_name(ggufCtx.get(), i);
//         struct ggml_tensor * t = ggml_add_tensor(m_ctx.get(), gguf_get_tensor(ggufCtx.get(), i));
//         ggml_set_name(t, name);
//     }

//     // 2. Map the tensors to the backend buffer (The GGUF data)
//     // We use the device's buffer type to ensure the weights are accessible
//     m_buffer = ggml_backend_alloc_ctx_tensors(m_ctx.get(), m_device.backend());

//     if (!m_buffer) {
//         JOB_LOG_ERROR("Failed to allocate weights buffer on device");
//         return false;
//     }

//     // 3. Fill our lookup map
//     for (int i = 0; i < n_tensors; ++i) {
//         const char *name = gguf_get_tensor_name(ggufCtx.get(), i);
//         m_tensors[name] = ggml_get_tensor(m_ctx.get(), name);
//     }

//     return true;
// }

bool JobGgmlWeights::loadFromGGUF(gguf_context_ptr &ggufCtx)
{
    if (!ggufCtx)
        return false;

    const int n_tensors = gguf_get_n_tensors(ggufCtx.get());

    for (int i = 0; i < n_tensors; ++i) {
        const char *name = gguf_get_tensor_name(ggufCtx.get(), i);
        if (!name)
            continue;

        ggml_tensor *t = ggml_get_tensor(m_ctx.get(), name);
        if (!t)
            continue;

        m_tensors[name] = t;
    }

    return !m_tensors.empty();
}


ggml_tensor *JobGgmlWeights::tensor(const std::string &name) noexcept
{
    auto it = m_tensors.find(name);
    return (it != m_tensors.end()) ? it->second : nullptr;
}

const ggml_tensor *JobGgmlWeights::tensor(const std::string &name) const noexcept
{
    auto it = m_tensors.find(name);
    return (it != m_tensors.end()) ? it->second : nullptr;
}

size_t JobGgmlWeights::tensorCount() const noexcept
{
    return m_tensors.size();
}

std::vector<std::string> JobGgmlWeights::names() const
{
    std::vector<std::string> out;
    out.reserve(m_tensors.size());

    for (const auto &[name, _] : m_tensors)
        out.push_back(name);

    return out;
}

size_t JobGgmlWeights::totalSize() const noexcept
{
    size_t total = 0;

    for (const auto &[_, t] : m_tensors)
        total += ggml_nbytes(t);

    return total;
}

}