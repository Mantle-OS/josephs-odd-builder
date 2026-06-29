#include "job_ggml_model.h"

namespace  job::ggml {

bool JobGgmlModel::loadGGUF(const std::string &path, JobGgmlDevice &device)
{
    m_ggufCtx.reset();

    gguf_init_params params{};
    params.no_alloc = false;

    m_ggufCtx = gguf_context_ptr{
        gguf_init_from_file(path.c_str(), params)
    };

    if (!m_ggufCtx) {
        return false;
    }

    // ------------------------------------------------------------
    // Helper: GGUF string lookup (key -> id -> value)
    // ------------------------------------------------------------
    auto getStr = [&](const char *key) -> const char * {
        int64_t id = gguf_find_key(m_ggufCtx.get(), key);
        if (id < 0)
            return nullptr;

        return gguf_get_val_str(m_ggufCtx.get(), id);
    };

    [[maybe_unused]] auto getInt = [&](const char *key) -> int64_t {
        int64_t id = gguf_find_key(m_ggufCtx.get(), key);
        if (id < 0)
            return 0;

        return gguf_get_val_i64(m_ggufCtx.get(), id);
    };

    // ------------------------------------------------------------
    // Core metadata
    // ------------------------------------------------------------
    if (const char *name = getStr("general.name")) {
        m_name = name;
    }

    if (const char *arch = getStr("general.architecture")) {
        m_architecture = arch;
    }

    // ------------------------------------------------------------
    // Full metadata dump (safe + future-proof)
    // ------------------------------------------------------------
    const int n_kv = gguf_get_n_kv(m_ggufCtx.get());

    for (int i = 0; i < n_kv; ++i) {
        const char *key = gguf_get_key(m_ggufCtx.get(), i);
        if (!key)
            continue;

        int64_t val_type = gguf_get_kv_type(m_ggufCtx.get(), i);

        // Only handle strings for now (extend later if needed)
        if (val_type == GGUF_TYPE_STRING) {
            const char *val = gguf_get_val_str(m_ggufCtx.get(), i);
            if (val) {
                m_metadata[key] = val;
            }
        }
    }

    // ------------------------------------------------------------
    // Weights (device-aware initialization)
    // ------------------------------------------------------------
    m_weights = std::make_unique<JobGgmlWeights>(device);

    if (!m_weights->loadFromGGUF(m_ggufCtx)) {
        return false;
    }

    return true;
}

const std::string &JobGgmlModel::name() const noexcept
{
    return m_name;
}

const std::string &JobGgmlModel::architecture() const noexcept
{
    return m_architecture;
}

std::string JobGgmlModel::metadata(const std::string &key) const
{
    auto it = m_metadata.find(key);
    if (it != m_metadata.end())
        return it->second;

    return {};
}

JobGgmlWeights &JobGgmlModel::weights() noexcept
{
    return *m_weights;
}

const JobGgmlWeights &JobGgmlModel::weights() const noexcept
{
    return *m_weights;
}

gguf_context *JobGgmlModel::ggufCtx() noexcept
{
    return m_ggufCtx.get();
}
}