#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>

#include <job_parallel_for.h>
#include <job_logger.h>

// Assuming these are defined elsewhere, otherwise define defaults:
#ifndef JOB_AI_BLOCK_ROWS
#define JOB_AI_BLOCK_ROWS 64
#endif
#ifndef JOB_AI_BLOCK_COLS
#define JOB_AI_BLOCK_COLS 128
#endif

// Threshold for stack allocation.
// 64 rows * 256 dim * 4 bytes = 64KB.
// This covers 99% of your "everything else" cases without touching the heap.
#ifndef JOB_AI_MAX_STACK_DIM
#define JOB_AI_MAX_STACK_DIM 256
#endif

namespace job::ai::comp {
inline void flashForward(int N, // seq Length
                         int d, // head dimension (Dynamic!)
                         const float* __restrict__ Q,
                         const float* __restrict__ K,
                         const float* __restrict__ V,
                         float* __restrict__ O,
                         int br_idx,
                         float scale)
{
    int i_start = br_idx * JOB_AI_BLOCK_ROWS;
    int i_end = std::min(i_start + JOB_AI_BLOCK_ROWS, N);
    int rows = i_end - i_start;

    //Dynamic Accumulator Allocation ---
    // Goal: O_local [rows x d]
    // Strategy: Try stack first (fast), fallback to heap (safe)

    float stack_buf[JOB_AI_BLOCK_ROWS * JOB_AI_MAX_STACK_DIM];
    std::vector<float> heap_buf; // Keep this in scope!
    float* O_local_flat;

    if (d <= JOB_AI_MAX_STACK_DIM) {
        O_local_flat = stack_buf;
    } else {
        // "Resize or do something" -> We resize.
        // This handles your char/motif embeddings if they are huge (e.g. 256/512)
        heap_buf.resize(rows * d);
        O_local_flat = heap_buf.data();
    }

    // Initialize Accumulator to 0
    std::memset(O_local_flat, 0, rows * d * sizeof(float));


    // l_local: Running sum of exponentials (denominator)
    float l_local[JOB_AI_BLOCK_ROWS] = {0};

    // m_local: Running max score
    float m_local[JOB_AI_BLOCK_ROWS];
    std::fill_n(m_local, JOB_AI_BLOCK_ROWS, -1e9f);

    // Buffer for Scores (Q * K^T)
    // This is size [Rows x Cols], independent of 'd'. Safe to keep fixed.
    float S_local[JOB_AI_BLOCK_ROWS][JOB_AI_BLOCK_COLS];

    for (int j_start = 0; j_start < N; j_start += JOB_AI_BLOCK_COLS) {
        int j_end = std::min(j_start + JOB_AI_BLOCK_COLS, N);
        int cols = j_end - j_start;

        // Compute Scores Tile: S = Q_tile * K_tile^T
        for (int r = 0; r < rows; ++r) {
            const float *q_ptr = Q + (i_start + r) * d;

            for (int c = 0; c < cols; ++c) {
                const float *k_ptr = K + (j_start + c) * d;

                // Dot Product
                // Note: If you have SIMD::dot(ptr, ptr, size), use it here!
                float score = 0.0f;
                for (int x = 0; x < d; ++x)
                    score += q_ptr[x] * k_ptr[x];

                S_local[r][c] = score * scale;
            }
        }

        // Online Softmax Update
        for (int r = 0; r < rows; ++r) {
            // Access the correct row in our flat O_local buffer
            float *o_local_row = O_local_flat + (r * d);

            float m_prev = m_local[r];
            float m_curr = m_prev;

            // Find max
            for (int c = 0; c < cols; ++c)
                m_curr = std::max(m_curr, S_local[r][c]);

            // Correction factor
            float correction = std::exp(m_prev - m_curr);
            m_local[r] = m_curr;

            // Update denominator
            float l_prev = l_local[r];
            float p_sum = 0.0f;

            // Temp P buffer // I hate all this but whatever ....... FIXME LATER
            float P_row[JOB_AI_BLOCK_COLS];
            for (int c = 0; c < cols; ++c) {
                float p = std::exp(S_local[r][c] - m_curr);
                P_row[c] = p;
                p_sum += p;
            }

            l_local[r] = (l_prev * correction) + p_sum;

            // Update Output Accumulator
            // Rescale existing O_local
            for (int x = 0; x < d; ++x)
                o_local_row[x] *= correction;

            // Accumulate P * V_new
            for (int c = 0; c < cols; ++c) {
                const float *v_ptr = V + (j_start + c) * d;
                float p = P_row[c];

                // Vector-Scalar FMA
                for (int x = 0; x < d; ++x)
                    o_local_row[x] += p * v_ptr[x];
            }
        }
    }


    for (int r = 0; r < rows; ++r) {
        float* o_local_row = O_local_flat + (r * d);

        float inv_l = (l_local[r] == 0.0f) ? 0.0f : (1.0f / l_local[r]);
        float *o_ptr = O + (i_start + r) * d;

        for (int x = 0; x < d; ++x)
            o_ptr[x] = o_local_row[x] * inv_l;
    }
}
// Single Threaded Wrapper
inline void flashAttentionForward(int N,
                                  int d,
                                  const float* __restrict__ Q,
                                  const float* __restrict__ K,
                                  const float* __restrict__ V,
                                  float* __restrict__ O,
                                  float scale)
{
    size_t num_blocks = (N + JOB_AI_BLOCK_ROWS - 1) / JOB_AI_BLOCK_ROWS;
    for(size_t i = 0; i < num_blocks; ++i)
        flashForward(N, d, Q, K, V, O, i, scale);
}

// Parallel Wrapper
inline void flashParallelAttentionForward(job::threads::ThreadPool &pool,
                                          int N,
                                          int d,
                                          const float* __restrict__ Q,
                                          const float* __restrict__ K,
                                          const float* __restrict__ V,
                                          float* __restrict__ O,
                                          float scale)
{
    size_t num_blocks = (N + JOB_AI_BLOCK_ROWS - 1) / JOB_AI_BLOCK_ROWS;

    job::threads::parallel_for(pool, size_t{0}, num_blocks, [&](size_t br_idx) {
        flashForward(N, d, Q, K, V, O, br_idx, scale);
    });
}

} // namespace job::ai::comp
