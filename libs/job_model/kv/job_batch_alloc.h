#pragma once

#include <array>
#include <bitset>
#include <cstdint>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include "job_batch.h"
#include "job_ubatch.h"

namespace job::token {
class JobVocab;
}

namespace job::model {

class JobIMem;

#ifndef JOB_MAX_SEQ
#define JOB_MAX_SEQ 256
#endif

// A helper for sanitizing, fulfilling, and splitting a batch.
class JobBatchAlloc
{
public:
    using Ptr  = std::shared_ptr<JobBatchAlloc>;
    using WPtr = std::weak_ptr<JobBatchAlloc>;
    using UPtr = std::unique_ptr<JobBatchAlloc>;

    explicit JobBatchAlloc(uint32_t nPosPerEmbd);
    ~JobBatchAlloc() = default;

    JobBatchAlloc(const JobBatchAlloc &) = default;
    JobBatchAlloc &operator=(const JobBatchAlloc &) = delete;

    JobBatchAlloc(JobBatchAlloc &&) noexcept = default;
    JobBatchAlloc &operator=(JobBatchAlloc &&) noexcept = delete;

    [[nodiscard]] static Ptr createShared(uint32_t nPosPerEmbd)
    {
        return std::make_shared<JobBatchAlloc>(nPosPerEmbd);
    }

    [[nodiscard]] static UPtr createUniq(uint32_t nPosPerEmbd)
    {
        return std::make_unique<JobBatchAlloc>(nPosPerEmbd);
    }

    // Sanitize and auto-generate missing data in the input batch.
    // Memory is optional. If provided, it will be used to check for
    // sequence continuity and to determine positions.
    [[nodiscard]] bool init(
        const JobBatch &batchInp,
        const token::JobVocab &vocab,
        const JobIMem *mem,
        uint32_t nEmbd,
        uint32_t nSeqMax,
        bool outputAll);

    [[nodiscard]] const JobBatch &batch() const noexcept;

    [[nodiscard]] uint32_t nTokens() const noexcept;
    [[nodiscard]] uint32_t nOutputs() const noexcept;
    [[nodiscard]] uint32_t nUsed() const noexcept;

    // The array of output indices in the order they were encountered
    // during the ubatch splitting.
    [[nodiscard]] std::vector<int32_t> &outIds() noexcept;

    // Min/max positions of each sequence in the current ubatch.
    [[nodiscard]] uint32_t seqPosMin(uint32_t seqId) const;
    [[nodiscard]] uint32_t seqPosMax(uint32_t seqId) const;

    // Call once before splitting the batch to reset the internal state.
    void splitReset();

    // Simple split, unknown number of sequence sets of unequal lengths.
    [[nodiscard]] JobUBatch splitSimple(uint32_t nUbatch);

    // Make ubatches of equal-length sequence sets.
    // If sequential == true, the tokens in the ubatch will have increasing
    // sequential sequence ids.
    [[nodiscard]] JobUBatch splitEqual(uint32_t nUbatch, bool sequential);

    // Sequence-set-wise split - each ubatch contains a single sequence-set.
    [[nodiscard]] JobUBatch splitSeq(uint32_t nUbatch);

    // A helper method for creating a well-defined ubatch of tokens.
    // TODO: support embeddings if needed in the future.
    [[nodiscard]] JobUBatch uBatchReserve(uint32_t nSeqTokens, uint32_t nSeqs);

private:
    void clear();

    // Create the next ubatch based on the provided batch indices (idxs)
    // and the number of sequence sets (nSeqs).
    // Returns a JobUBatch with nTokens == 0 if the entire batch was consumed.
    [[nodiscard]] JobUBatch ubatchAdd(
        const std::vector<int32_t> &idxs,
        uint32_t nSeqs,
        bool equalSeqs);

    // For debugging, start with JOB_BATCH_DEBUG=2.
    void uBatchDebug(const JobUBatch &ubatch, int debug);

    using PosSet = std::set<int32_t>;
    using SeqCpl = std::vector<bool>;

    using IdxVec = std::vector<int32_t>;
    using SeqSet = std::bitset<JOB_MAX_SEQ>;

    JobBatch m_batch;

    // Only for debugging purposes.
    const token::JobVocab *m_vocab{nullptr};

    // TODO: this is more of a temporary solution until we have a better way to handle multiple positions per token/embd.
    const uint32_t m_nPosPerEmbd;

    uint32_t m_nEmbd{0};
    uint32_t m_nSeqMax{0};
    uint32_t m_nOutputs{0};

    std::array<int32_t, 1> m_seq_id_0{{0}};

    std::vector<int32_t>   m_pos;
    std::vector<int32_t>   m_n_seqId;
    std::vector<int32_t *> m_seqId;
    std::vector<int32_t>   m_seqIdUnq;
    std::vector<int32_t>   m_seqIdx;
    std::vector<int8_t>    m_output;

    // Helper flag to quickly determine if there are any coupled sequences
    // in the batch.
    bool m_hasCpl{false};

    std::vector<PosSet> m_seqPos;
    std::vector<SeqCpl> m_seqCpl;

    // seqSet[i]: the sequence set of token i.
    std::vector<SeqSet> m_seqSet;

    // The indices at which each sequence set appears.
    std::unordered_map<SeqSet, IdxVec> m_seqSetMap;

    // Batch indices of the output.
    std::vector<int32_t> m_outIds;

    uint32_t m_nUsed{0};

    // m_used[i] indicates if token i has already been used in a previous ubatch.
    std::vector<bool> m_used;

    int m_debug{0};
};

} // namespace job::model