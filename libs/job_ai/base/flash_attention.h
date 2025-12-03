#pragma once

#include "real_type.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <immintrin.h>

#include <job_parallel_for.h>
#include <job_logger.h>

namespace job::ai::base {


// 32 rows * 64 dim * 4 bytes = 8KB. => L1 cache
constexpr int kBlockRows = 32;

// Laptop
constexpr int kBlockSize = 128;

// Server
// constexpr int kBlockSize = 256;


// 128 cols * 64 dim * 4 bytes = 32KB. Fits in L1/L2.
constexpr int kBlockCols = 128;

// "standard" transformer size
constexpr int kHeadDim = 64;




// Note: Assumes row-major layout. this implementation is "Single-Head".  I guess later I will "Batched/Multi-head" that wraps this.
// Inputs: Q: [SeqLen, HeadDim] | K: [SeqLen, HeadDim] | V: [SeqLen, HeadDim]
// Output: O: [SeqLen, HeadDim]
inline void flashAttentionForward(
    job::threads::ThreadPool& pool,
    int N,      // seq Length
    int d,      // head dimension (must be <= kHeadDim for this optimized "kernel"(sorry linus not sure why these math folks call it this))
    const core::real_t* __restrict__ Q,
    const core::real_t* __restrict__ K,
    const core::real_t* __restrict__ V,
    core::real_t* __restrict__ O,
    core::real_t scale = 1.0f) // usually 1/sqrt(d) ???
{
    job::threads::parallel_for(pool, size_t{0}, size_t((N + kBlockRows - 1) / kBlockRows), [&](size_t br_idx) {

        int i_start = br_idx * kBlockRows;
        int i_end = std::min(i_start + kBlockRows, N);
        int rows = i_end - i_start;

        // registers
        // O_local: Accumulates weighted sum of V
        core::real_t O_local[kBlockRows][kHeadDim] = {{0}};

        // l_local: Running sum of exponentials (denominator)
        core::real_t l_local[kBlockRows] = {0};

        // m_local: Running max score (for numerical stability)
        core::real_t m_local[kBlockRows];
        std::fill_n(m_local, kBlockRows, -1e9f); // Init with -inf

        // Buffer for Scores (Q * K^T) | Size: [Rows x Cols] = 32 * 128 = 4096 float's = 16KB -> L1
        core::real_t S_local[kBlockRows][kBlockCols];

        // iterate over key/value blocks
        for (int j_start = 0; j_start < N; j_start += kBlockCols) {
            int j_end = std::min(j_start + kBlockCols, N);
            int cols = j_end - j_start;

            // S = Q_tile * K_tile^T
            // Q tile: [rows, d] | K tile: [cols, d] | S tile: [rows, cols]
            for (int r = 0; r < rows; ++r) {
                const core::real_t* q_ptr = Q + (i_start + r) * d;

                for (int c = 0; c < cols; ++c) {
                    const core::real_t* k_ptr = K + (j_start + c) * d;

                    // Dot Product (AVX2-friendly loop)
                    core::real_t score = 0.0f;
                    for (int x = 0; x < d; ++x)
                        score += q_ptr[x] * k_ptr[x];

                    S_local[r][c] = score * scale;
                }
            }

            // online softmax & accumulation
            for (int r = 0; r < rows; ++r) {
                // Find max in this new chunk
                core::real_t m_prev = m_local[r];
                core::real_t m_curr = m_prev;

                for (int c = 0; c < cols; ++c)
                    m_curr = std::max(m_curr, S_local[r][c]);

                // calculate correction factors if new max, shrink the old accumulations
                core::real_t correction = std::exp(m_prev - m_curr);
                m_local[r] = m_curr;

                // update running denominator || l_new = l_prev * exp(m_prev - m_curr) + sum(exp(S_new - m_curr))
                core::real_t l_prev = l_local[r];
                core::real_t p_sum = 0.0f;

                // temporary row buffer for p (probabilities) to avoid re-exp-ing
                core::real_t P_row[kBlockCols];

                for (int c = 0; c < cols; ++c) {
                    core::real_t p = std::exp(S_local[r][c] - m_curr);
                    P_row[c] = p;
                    p_sum += p;
                }

                l_local[r] = (l_prev * correction) + p_sum;

                // update output accumulator
                // O_new = O_prev * correction + P * V | First, apply correction to existing O
                for (int x = 0; x < d; ++x)
                    O_local[r][x] *= correction;

                // accumulate new weighted values
                for (int c = 0; c < cols; ++c) {
                    const core::real_t* v_ptr = V + (j_start + c) * d;
                    core::real_t p = P_row[c];

                    // vector-scalar mul-add
                    for (int x = 0; x < d; ++x)
                        O_local[r][x] += p * v_ptr[x];
                }
            }
        }

        // final normalization | O = O_accum / l_accum
        for (int r = 0; r < rows; ++r) {
            core::real_t inv_l = 1.0f / l_local[r];
            core::real_t* o_ptr = O + (i_start + r) * d;

            for (int x = 0; x < d; ++x)
                o_ptr[x] = O_local[r][x] * inv_l;
        }
    });
}

} // namespace job::ai::base
