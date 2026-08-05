#pragma once

#include <cstdint>

#include "jobggml_export.h"

namespace job::ggml {

/*
 * Semantic tensor shapes
 *
 * These structures describe the logical meaning of tensor dimensions.
 * They do not describe GGML's physical dimension order.
 *
 * Example:
 *
 *     Semantic: [B, S, D]
 *     GGML:     [D, S, B]
 *
 * The conversion between semantic and native GGML ordering belongs in
 * JobGgmlTensorExtents or JobGgmlTensorLayout.
 */

// [B, S]
//
// Common uses:
//   - token IDs
//   - attention masks
//   - position IDs
struct JOBGGML_EXPORT JobGgmlBSShape
{
    std::int64_t batch{0};
    std::int64_t sequence{0};

    [[nodiscard]] bool isValid() const noexcept
    {
        return batch > 0 && sequence > 0;
    }

    [[nodiscard]] std::int64_t elementCount() const noexcept
    {
        return isValid()? batch * sequence : 0;
    }

    [[nodiscard]] bool operator==(const JobGgmlBSShape &other) const noexcept = default;
};

// [B, S, D]
//
// Common uses:
//   - hidden states
//   - token embeddings
//   - projected transformer activations
struct JOBGGML_EXPORT JobGgmlBSDShape
{
    std::int64_t batch{0};
    std::int64_t sequence{0};
    std::int64_t dimension{0};

    [[nodiscard]] bool isValid() const noexcept
    {
        return batch > 0 &&
               sequence > 0 &&
               dimension > 0;
    }

    [[nodiscard]] std::int64_t elementCount() const noexcept
    {
        return isValid() ? batch * sequence * dimension : 0;
    }

    [[nodiscard]] std::int64_t vectorsPerBatch() const noexcept
    {
        return isValid() ? sequence : 0;
    }

    [[nodiscard]] bool operator==(const JobGgmlBSDShape &other) const noexcept = default;
};

// [B, S, H, Dh]
//
// Common uses:
//   - split query heads
//   - split key heads
//   - split value heads
//
// Dmodel = H * Dh
struct JOBGGML_EXPORT JobGgmlBSHDShape
{
    std::int64_t batch{0};
    std::int64_t sequence{0};
    std::int64_t heads{0};
    std::int64_t headDimension{0};

    [[nodiscard]] bool isValid() const noexcept
    {
        return batch > 0 &&
               sequence > 0 &&
               heads > 0 &&
               headDimension > 0;
    }

    [[nodiscard]] std::int64_t modelDimension() const noexcept
    {
        return heads > 0 && headDimension > 0 ? heads * headDimension : 0;
    }

    [[nodiscard]] std::int64_t elementCount() const noexcept
    {
        return isValid() ? batch * sequence * heads * headDimension : 0;
    }

    [[nodiscard]] bool operator==(const JobGgmlBSHDShape &other) const noexcept = default;
};

// [B, H, S, Dh]
//
// Same total element count as BSHD, but sequence and head axes
// are transposed. This is common during attention computation.
struct JOBGGML_EXPORT JobGgmlBHSDShape
{
    std::int64_t batch{0};
    std::int64_t heads{0};
    std::int64_t sequence{0};
    std::int64_t headDimension{0};

    [[nodiscard]] bool isValid() const noexcept
    {
        return batch > 0 &&
               heads > 0 &&
               sequence > 0 &&
               headDimension > 0;
    }

    [[nodiscard]] std::int64_t modelDimension() const noexcept
    {
        return heads > 0 && headDimension > 0 ? heads * headDimension : 0;
    }

    [[nodiscard]] std::int64_t elementCount() const noexcept
    {
        return isValid() ? batch * heads * sequence * headDimension : 0;
    }

    [[nodiscard]] bool operator==(const JobGgmlBHSDShape &other) const noexcept = default;
};

// [B, H, Sq, Sk]
//
// Common use:
//   - attention score matrices
//
// Sq may equal Sk for self-attention, but they can differ for
// cross-attention or cached key/value sequences.
struct JOBGGML_EXPORT JobGgmlBHSSShape
{
    std::int64_t batch{0};
    std::int64_t heads{0};
    std::int64_t querySequence{0};
    std::int64_t keySequence{0};

    [[nodiscard]] bool isValid() const noexcept
    {
        return batch > 0 &&
               heads > 0 &&
               querySequence > 0 &&
               keySequence > 0;
    }

    [[nodiscard]] bool isSquareAttention() const noexcept
    {
        return querySequence > 0 && querySequence == keySequence;
    }

    [[nodiscard]] std::int64_t elementCount() const noexcept
    {
        return isValid() ? batch * heads * querySequence * keySequence : 0;
    }

    [[nodiscard]] bool operator==( const JobGgmlBHSSShape &other ) const noexcept = default;
};

// [V, D]
//
// Common uses:
//   - token embedding tables
//   - output projection tables
//
// V = vocabulary size
// D = embedding dimension
struct JOBGGML_EXPORT JobGgmlVDShape
{
    std::int64_t vocabulary{0};
    std::int64_t dimension{0};

    [[nodiscard]] bool isValid() const noexcept
    {
        return vocabulary > 0 && dimension > 0;
    }

    [[nodiscard]] std::int64_t elementCount() const noexcept
    {
        return isValid() ? vocabulary * dimension : 0;
    }

    [[nodiscard]] bool operator==( const JobGgmlVDShape &other ) const noexcept = default;
};

// [Dout, Din]
//
// Common use:
//   - linear-layer weight matrices
struct JOBGGML_EXPORT JobGgmlLinearShape
{
    std::int64_t outputDimension{0};
    std::int64_t inputDimension{0};

    [[nodiscard]] bool isValid() const noexcept
    {
        return outputDimension > 0 && inputDimension > 0;
    }

    [[nodiscard]] std::int64_t elementCount() const noexcept
    {
        return isValid() ? outputDimension * inputDimension : 0;
    }

    [[nodiscard]] bool isSquare() const noexcept
    {
        return outputDimension > 0 && outputDimension == inputDimension;
    }

    [[nodiscard]] bool operator==(const JobGgmlLinearShape &other) const noexcept = default;
};

// [B, C, H, W]
//
// Common uses:
//   - image batches
//   - convolution feature maps
//   - diffusion latents
struct JOBGGML_EXPORT JobGgmlBCHWShape
{
    std::int64_t batch{0};
    std::int64_t channels{0};
    std::int64_t height{0};
    std::int64_t width{0};

    [[nodiscard]] bool isValid() const noexcept
    {
        return batch > 0 &&
               channels > 0 &&
               height > 0 &&
               width > 0;
    }

    [[nodiscard]] std::int64_t spatialSize() const noexcept
    {
        return height > 0 && width > 0 ? height * width : 0;
    }

    [[nodiscard]] std::int64_t elementsPerBatch() const noexcept
    {
        const std::int64_t spatial = spatialSize();

        return channels > 0 && spatial > 0 ? channels * spatial : 0;
    }

    [[nodiscard]] std::int64_t elementCount() const noexcept
    {
        const std::int64_t perBatch = elementsPerBatch();

        return batch > 0 && perBatch > 0 ? batch * perBatch : 0;
    }

    [[nodiscard]] bool operator==(const JobGgmlBCHWShape &other) const noexcept = default;
};

} // namespace job::ggml