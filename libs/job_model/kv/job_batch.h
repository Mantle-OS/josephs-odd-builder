#pragma once

#include <cstdint>
#include <memory>

#include <job_token.h>

namespace job::model {

class JobBatch
{
public:
    using Ptr  = std::shared_ptr<JobBatch>;
    using WPtr = std::weak_ptr<JobBatch>;
    using UPtr = std::unique_ptr<JobBatch>;

    JobBatch() = default;
    ~JobBatch() = default;

    JobBatch(const JobBatch &) = default;
    JobBatch &operator=(const JobBatch &) = default;

    JobBatch(JobBatch &&) noexcept = default;
    JobBatch &operator=(JobBatch &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobBatch>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobBatch>();
    }

    [[nodiscard]] int32_t nTokens() const noexcept
    {
        return m_nTokens;
    }

    void setNTokens(int32_t value) noexcept
    {
        m_nTokens = value;
    }

    [[nodiscard]] token::JobToken *token() noexcept
    {
        return m_token;
    }

    [[nodiscard]] const token::JobToken *token() const noexcept
    {
        return m_token;
    }

    void setToken(token::JobToken *value) noexcept
    {
        m_token = value;
    }

    [[nodiscard]] float *embd() noexcept
    {
        return m_embd;
    }

    [[nodiscard]] const float *embd() const noexcept
    {
        return m_embd;
    }

    void setEmbd(float *value) noexcept
    {
        m_embd = value;
    }

    [[nodiscard]] int32_t *pos() noexcept
    {
        return m_pos;
    }

    [[nodiscard]] const int32_t *pos() const noexcept
    {
        return m_pos;
    }

    void setPos(int32_t *value) noexcept
    {
        m_pos = value;
    }

    [[nodiscard]] int32_t *nSeqId() noexcept
    {
        return m_nSeqId;
    }

    [[nodiscard]] const int32_t *nSeqId() const noexcept
    {
        return m_nSeqId;
    }

    void setNSeqId(int32_t *value) noexcept
    {
        m_nSeqId = value;
    }

    [[nodiscard]] int32_t **seqId() noexcept
    {
        return m_seqId;
    }

    [[nodiscard]] int32_t *const *seqId() const noexcept
    {
        return m_seqId;
    }

    void setSeqId(int32_t **value) noexcept
    {
        m_seqId = value;
    }

    [[nodiscard]] int8_t *output() noexcept
    {
        return m_output;
    }

    [[nodiscard]] const int8_t *output() const noexcept
    {
        return m_output;
    }

    void setOutput(int8_t *value) noexcept
    {
        m_output = value;
    }

private:
    int32_t             m_nTokens{0};
    token::JobToken     *m_token{nullptr};
    float               *m_embd{nullptr};
    int32_t             *m_pos{nullptr};
    int32_t             *m_nSeqId{nullptr};
    int32_t             **m_seqId{nullptr};
    int8_t              *m_output{nullptr};
};

} // namespace job::model