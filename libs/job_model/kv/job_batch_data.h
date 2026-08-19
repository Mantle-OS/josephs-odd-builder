#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace job::model {

class BatchData
{
public:
    using Ptr  = std::shared_ptr<BatchData>;
    using WPtr = std::weak_ptr<BatchData>;
    using UPtr = std::unique_ptr<BatchData>;

    BatchData() = default;
    ~BatchData() = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<BatchData>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<BatchData>();
    }

    BatchData(const BatchData &) = default;
    BatchData &operator=(const BatchData &) = default;
    BatchData(BatchData &&) noexcept = default;
    BatchData &operator=(BatchData &&) noexcept = default;

    [[nodiscard]] std::vector<int32_t> &token() noexcept
    {
        return m_token;
    }

    [[nodiscard]] const std::vector<int32_t> &token() const noexcept
    {
        return m_token;
    }

    [[nodiscard]] std::vector<float> &embd() noexcept
    {
        return m_embd;
    }

    [[nodiscard]] const std::vector<float> &embd() const noexcept
    {
        return m_embd;
    }

    [[nodiscard]] std::vector<int32_t> &pos() noexcept
    {
        return m_pos;
    }

    [[nodiscard]] const std::vector<int32_t> &pos() const noexcept
    {
        return m_pos;
    }

    [[nodiscard]] std::vector<int32_t> &nSeqId() noexcept
    {
        return m_nSeqId;
    }

    [[nodiscard]] const std::vector<int32_t> &nSeqId() const noexcept
    {
        return m_nSeqId;
    }

    [[nodiscard]] std::vector<int32_t *> &seqId() noexcept
    {
        return m_seqId;
    }

    [[nodiscard]] const std::vector<int32_t *> &seqId() const noexcept
    {
        return m_seqId;
    }

    [[nodiscard]] std::vector<int32_t> &seqIdUnq() noexcept
    {
        return m_seqIdUnq;
    }

    [[nodiscard]] const std::vector<int32_t> &seqIdUnq() const noexcept
    {
        return m_seqIdUnq;
    }

    [[nodiscard]] std::vector<int32_t> &seqIdx() noexcept
    {
        return m_seqIdx;
    }

    [[nodiscard]] const std::vector<int32_t> &seqIdx() const noexcept
    {
        return m_seqIdx;
    }

    [[nodiscard]] std::vector<int8_t> &output() noexcept
    {
        return m_output;
    }

    [[nodiscard]] const std::vector<int8_t> &output() const noexcept
    {
        return m_output;
    }

    [[nodiscard]] std::vector<int32_t> &seqIdData() noexcept
    {
        return m_seqIdData;
    }

    [[nodiscard]] const std::vector<int32_t> &seqIdData() const noexcept
    {
        return m_seqIdData;
    }

private:
    std::vector<int32_t>   m_token;
    std::vector<float>     m_embd;
    std::vector<int32_t>   m_pos;
    std::vector<int32_t>   m_nSeqId;
    std::vector<int32_t *> m_seqId;      // these point into the seqIdData below
    std::vector<int32_t>   m_seqIdUnq;
    std::vector<int32_t>   m_seqIdx;
    std::vector<int8_t>    m_output;
    std::vector<int32_t>   m_seqIdData;
};

} // namespace job::model