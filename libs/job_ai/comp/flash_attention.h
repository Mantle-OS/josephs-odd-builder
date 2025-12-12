#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>

#include "simd_provider.h"


#include <job_parallel_for.h>
#include <job_logger.h>

namespace job::ai::comp {

// IMPORANT !! Assumes row-major layout. this implementation is "Single-Head".  I guess later I will "Batched/Multi-head" that wraps this.
// Inputs: Q: [SeqLen, HeadDim] | K: [SeqLen, HeadDim] | V: [SeqLen, HeadDim]
// Output: O: [SeqLen, HeadDim]
inline void flashAttentionForward(
    job::threads::ThreadPool& pool,
    int N,      // seq Length
    int d,      // head dimension (must be <= JOB_AI_HEAD_DIM for this optimized "kernel"(sorry linus not sure why these math folks call it this))
    const float* __restrict__ Q,
    const float* __restrict__ K,
    const float* __restrict__ V,
    float* __restrict__ O,
    float scale = 1.0f) // usually 1/sqrt(d) ???
{
    job::threads::parallel_for(pool, size_t{0}, size_t((N + JOB_AI_BLOCK_ROWS - 1) / JOB_AI_BLOCK_ROWS), [&](size_t br_idx) {

        int i_start = br_idx * JOB_AI_BLOCK_ROWS;
        int i_end = std::min(i_start + JOB_AI_BLOCK_ROWS, N);
        int rows = i_end - i_start;

        // O_local: Accumulates weighted sum of V
        float O_local[JOB_AI_BLOCK_ROWS][JOB_AI_HEAD_DIM] = {{0}};

        // l_local: Running sum of exponentials (denominator)
        float l_local[JOB_AI_BLOCK_ROWS] = {0};

        // m_local: Running max score (for numerical stability)
        float m_local[JOB_AI_BLOCK_ROWS];
        std::fill_n(m_local, JOB_AI_BLOCK_ROWS, -1e9f); // Init with -inf

        // Buffer for Scores (Q * K^T) | Size: [Rows x Cols] = 32 * 128 = 4096 float's = 16KB -> L1
        float S_local[JOB_AI_BLOCK_ROWS][JOB_AI_BLOCK_COLS];

        // iterate over key/value blocks
        for (int j_start = 0; j_start < N; j_start += JOB_AI_BLOCK_COLS) {
            int j_end = std::min(j_start + JOB_AI_BLOCK_COLS, N);
            int cols = j_end - j_start;

            // S = Q_tile * K_tile^T
            // Q tile: [rows, d] | K tile: [cols, d] | S tile: [rows, cols]
            for (int r = 0; r < rows; ++r) {
                const float *q_ptr = Q + (i_start + r) * d;

                for (int c = 0; c < cols; ++c) {
                    const float *k_ptr = K + (j_start + c) * d;

                    // Dot Product (AVX2-friendly loop)
                    float score = 0.0f;
                    for (int x = 0; x < d; ++x)
                        score += q_ptr[x] * k_ptr[x];

                    S_local[r][c] = score * scale;
                }
            }

            // online softmax & accumulation
            for (int r = 0; r < rows; ++r) {
                // Find max in this new chunk
                float m_prev = m_local[r];
                float m_curr = m_prev;

                for (int c = 0; c < cols; ++c)
                    m_curr = std::max(m_curr, S_local[r][c]);

                // calculate correction factors if new max, shrink the old accumulations
                float correction = std::exp(m_prev - m_curr);
                m_local[r] = m_curr;

                // update running denominator || l_new = l_prev * exp(m_prev - m_curr) + sum(exp(S_new - m_curr))
                float l_prev = l_local[r];
                float p_sum = 0.0f;

                // temporary row buffer for p (probabilities) to avoid re-exp-ing
                float P_row[JOB_AI_BLOCK_COLS];

                for (int c = 0; c < cols; ++c) {
                    float p = std::exp(S_local[r][c] - m_curr);
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
                    const float *v_ptr = V + (j_start + c) * d;
                    float p = P_row[c];

                    // vector-scalar mul-add
                    for (int x = 0; x < d; ++x)
                        O_local[r][x] += p * v_ptr[x];
                }
            }
        }

        // final normalization | O = O_accum / l_accum
        for (int r = 0; r < rows; ++r) {
            float inv_l = 1.0f / l_local[r];
            float *o_ptr = O + (i_start + r) * d;

            for (int x = 0; x < d; ++x)
                o_ptr[x] = O_local[r][x] * inv_l;
        }
    });
}

} // namespace job::ai::comp
