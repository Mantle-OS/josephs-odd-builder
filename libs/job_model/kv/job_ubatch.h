#pragma once

#include <cstdint>
#include <memory>
#include "job_batch_data.h"

namespace job::model {

class JobUBatch
{
public:
    using Ptr  = std::shared_ptr<JobUBatch>;
    using WPtr = std::weak_ptr<JobUBatch>;
    using UPtr = std::unique_ptr<JobUBatch>;

    enum class PosType : uint32_t
    {
        Seq = 0,
        Rope2D,
        Rope3D,
        Unknown
    };

    JobUBatch() = default;
    ~JobUBatch() = default;

    JobUBatch(const JobUBatch &) = default;
    JobUBatch &operator=(const JobUBatch &) = default;

    JobUBatch(JobUBatch &&) noexcept = default;
    JobUBatch &operator=(JobUBatch &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobUBatch>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobUBatch>();
    }

    [[nodiscard]] bool equalSeqs() const noexcept
    {
        return m_isEqualSeqs != 0;
    }

    void setEqualSeqs(bool value) noexcept
    {
        m_isEqualSeqs = value ? 1U : 0U;
    }

    [[nodiscard]] bool pos2d() const noexcept
    {
        return m_posType == PosType::Rope2D;
    }

    [[nodiscard]] PosType posType() const noexcept
    {
        return m_posType;
    }

    void setPosType(PosType value) noexcept
    {
        m_posType = value;
    }

    [[nodiscard]] uint32_t nTokens() const noexcept
    {
        return m_nTokens;
    }

    void setNTokens(uint32_t value) noexcept
    {
        m_nTokens = value;
    }

    [[nodiscard]] uint32_t nSeqTokens() const noexcept
    {
        return m_nSeqTokens;
    }

    void setNSeqTokens(uint32_t value) noexcept
    {
        m_nSeqTokens = value;
    }

    [[nodiscard]] uint32_t nSeqs() const noexcept
    {
        return m_nSeqs;
    }

    void setNSeqs(uint32_t value) noexcept
    {
        m_nSeqs = value;
    }

    [[nodiscard]] uint32_t nSeqsUnq() const noexcept
    {
        return m_nSeqsUnq;
    }

    void setNSeqsUnq(uint32_t value) noexcept
    {
        m_nSeqsUnq = value;
    }

    [[nodiscard]] uint32_t nPos() const noexcept
    {
        return m_nPos;
    }

    void setNPos(uint32_t value) noexcept
    {
        m_nPos = value;
    }

    [[nodiscard]] int32_t *tokens() noexcept
    {
        return m_tokens;
    }

    [[nodiscard]] const int32_t *tokens() const noexcept
    {
        return m_tokens;
    }

    void setTokens(int32_t *value) noexcept
    {
        m_tokens = value;
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

    [[nodiscard]] int32_t *totalSeqId() noexcept
    {
        return m_totalSeqId;
    }

    [[nodiscard]] const int32_t *totalSeqId() const noexcept
    {
        return m_totalSeqId;
    }

    void setTotalSeqId(int32_t *value) noexcept
    {
        m_totalSeqId = value;
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

    [[nodiscard]] int32_t *seqIdUnq() noexcept
    {
        return m_seqIdUnq;
    }

    [[nodiscard]] const int32_t *seqIdUnq() const noexcept
    {
        return m_seqIdUnq;
    }

    void setSeqIdUnq(int32_t *value) noexcept
    {
        m_seqIdUnq = value;
    }

    [[nodiscard]] int32_t *seqIdx() noexcept
    {
        return m_seqIdx;
    }

    [[nodiscard]] const int32_t *seqIdx() const noexcept
    {
        return m_seqIdx;
    }

    void setSeqIdx(int32_t *value) noexcept
    {
        m_seqIdx = value;
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

    [[nodiscard]] const BatchData::Ptr &batchData() const noexcept
    {
        return data;
    }

    void setBatchData(BatchData::Ptr value) noexcept
    {
        // SHARED !!!!!!
        data = value;
    }

private:
    uint32_t m_isEqualSeqs{0};
    uint32_t m_nTokens{0};
    uint32_t m_nSeqTokens{0};
    uint32_t m_nSeqs{0};
    uint32_t m_nSeqsUnq{0};
    uint32_t m_nPos{0};

    PosType m_posType{PosType::Unknown};

    //                             size                    | idx | val
    int32_t  *m_tokens{nullptr};     // [n_tokens]         | i   | id, token
    float    *m_embd{nullptr};       // [n_embd, n_tokens] | i   | embd
    int32_t  *m_pos{nullptr};        // [n_tokens*n_pos]   | i   | pos
    int32_t  *m_totalSeqId{nullptr}; // [n_tokens]         | i   | -
    int32_t **m_seqId{nullptr};      // [n_tokens]         | s   | s0, s1, seq_id
    int32_t  *m_seqIdUnq{nullptr};   // [n_seqs_unq]       | s   | seq_id
    int32_t  *m_seqIdx{nullptr};     // [MAX_SEQ]          | -   | seq_idx
    int8_t   *m_output{nullptr};     // [n_tokens]         | i   | -

    // The pointers above point to this data if set.
    // Otherwise they point to external, non-owning data.
    BatchData::Ptr data = nullptr; // SHARED
};

} // namespace job::model