
#if 0
#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <limits>

#include <job_parallel_for.h>
#include <job_logger.h>
#include <simd_provider.h>
#include <simd_for.h>

// Assuming these are defined elsewhere, otherwise define defaults:
#ifndef JOB_AI_BLOCK_ROWS
#define JOB_AI_BLOCK_ROWS 64
#endif
#ifndef JOB_AI_BLOCK_COLS
#define JOB_AI_BLOCK_COLS 128
#endif

// Threshold for stack allocation.
#ifndef JOB_AI_MAX_STACK_DIM
#define JOB_AI_MAX_STACK_DIM 256
#endif

namespace job::ai::comp {

using namespace job::simd;

// Helper to sum all lanes in a register
__attribute__((always_inline))
inline float hsum_ps(f32 x) {
    alignas(sizeof(float) * SIMD::width()) float tmp[SIMD::width()];
    SIMD::mov(tmp, x);
    float s = 0.0f;
    for (int i = 0; i < SIMD::width(); ++i)
        s += tmp[i];
    return s;
}

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

    // 1. Dynamic Accumulator Allocation
    float stack_buf[JOB_AI_BLOCK_ROWS * JOB_AI_MAX_STACK_DIM];
    std::vector<float> heap_buf;
    float* O_local_flat;

    if (d <= JOB_AI_MAX_STACK_DIM) {
        O_local_flat = stack_buf;
    } else {
        heap_buf.resize(rows * d);
        O_local_flat = heap_buf.data();
    }

    // Initialize Accumulator to 0
    std::memset(O_local_flat, 0, rows * d * sizeof(float));

    float l_local[JOB_AI_BLOCK_ROWS] = {0};
    float m_local[JOB_AI_BLOCK_ROWS];
    std::fill_n(m_local, JOB_AI_BLOCK_ROWS, -1e9f); // -Inf

    float S_local[JOB_AI_BLOCK_ROWS][JOB_AI_BLOCK_COLS];

    // ============================================================
    //  Main Tiling Loop
    // ============================================================
    for (int j_start = 0; j_start < N; j_start += JOB_AI_BLOCK_COLS) {
        int j_end = std::min(j_start + JOB_AI_BLOCK_COLS, N);
        int cols = j_end - j_start;

        // A. Compute Scores Tile: S = Q_tile * K_tile^T
        for (int r = 0; r < rows; ++r) {
            const float *q_ptr = Q + (i_start + r) * d;

            for (int c = 0; c < cols; ++c) {
                const float *k_ptr = K + (j_start + c) * d;

                // [SIMD DOT PRODUCT]
                // 2x Unroll is usually the sweet spot for AVX2 FMA latency vs Register Pressure
                f32 v0 = SIMD::zero();
                f32 v1 = SIMD::zero();

                int x = 0;
                // Main vector loop unrolled 2x
                for (; x + 2 * SIMD::width() <= d; x += 2 * SIMD::width()) {
                    v0 = SIMD::mul_plus(SIMD::pull(q_ptr + x), SIMD::pull(k_ptr + x), v0);
                    v1 = SIMD::mul_plus(SIMD::pull(q_ptr + x + SIMD::width()), SIMD::pull(k_ptr + x + SIMD::width()), v1);
                }

                v0 = SIMD::add(v0, v1);

                // Handle remaining vectors
                for (; x + SIMD::width() <= d; x += SIMD::width())
                    v0 = SIMD::mul_plus(SIMD::pull(q_ptr + x), SIMD::pull(k_ptr + x), v0);


                float s_val = hsum_ps(v0);
                for (; x < d; ++x)
                    s_val += q_ptr[x] * k_ptr[x];

                S_local[r][c] = s_val * scale;
            }
        }

        // B. Online Softmax Update & Output Accumulation
        for (int r = 0; r < rows; ++r) {
            float *o_local_row = O_local_flat + (r * d);
            float m_prev = m_local[r];
            float m_curr = m_prev;

            // Find max (Scalar is fine)
            for (int c = 0; c < cols; ++c)
                m_curr = std::max(m_curr, S_local[r][c]);

            float correction = std::exp(m_prev - m_curr); // COUGH COUGH HEAVEY

            m_local[r] = m_curr;

            float P_row[JOB_AI_BLOCK_COLS];

            // [SIMD EXP - FAST]
            // Using exp (Estrin) to maintain accureracy
            f32 v_p_sum = SIMD::zero();
            float p_sum = 0.0f;
            auto v_m_curr = SIMD::set1(m_curr);

            simd_for(cols,
                     [&](size_t c) {
                         auto v_s = SIMD::pull(&S_local[r][c]);
                         // exp_fast is the key here!
                         auto v_p = SIMD::exp(SIMD::sub(v_s, v_m_curr));
                         SIMD::mov(&P_row[c], v_p);
                         v_p_sum = SIMD::add(v_p_sum, v_p);
                     },
                     [&](size_t c) {
                        // SIMD::exp
                         float p = std::exp(S_local[r][c] - m_curr);
                         P_row[c] = p;
                         p_sum += p;
                     }
                     );
            p_sum += hsum_ps(v_p_sum);

            float l_prev = l_local[r];
            l_local[r] = (l_prev * correction) + p_sum;

            // [SIMD RESCALE O]
            // O *= correction
            if (correction != 1.0f) { // Slight optimization branch
                auto v_correction = SIMD::set1(correction);
                simd_for(d,
                         [&](size_t x) {
                             auto ptr = o_local_row + x;
                             SIMD::mov(ptr, SIMD::mul(SIMD::pull(ptr), v_correction));
                         },
                         [&](size_t x) { o_local_row[x] *= correction; }
                         );
            }

            // [SIMD ACCUMULATE O += P * V]
            for (int c = 0; c < cols; ++c) {
                const float *v_ptr = V + (j_start + c) * d;
                float p = P_row[c];
                auto v_p = SIMD::set1(p);

                // Manual unroll here to ensure no lambda overhead in the HOTTEST loop
                int x = 0;
                for (; x + SIMD::width() <= d; x += SIMD::width()) {
                    auto vo = SIMD::pull(o_local_row + x);
                    auto vv = SIMD::pull(v_ptr + x);
                    SIMD::mov(o_local_row + x, SIMD::mul_plus(v_p, vv, vo));
                }
                for (; x < d; ++x) o_local_row[x] += p * v_ptr[x];
            }
        }
    }

    // 2. Final Writeback
    for (int r = 0; r < rows; ++r) {
        float* o_local_row = O_local_flat + (r * d);
        float* o_ptr = O + (i_start + r) * d;

        float inv_l = (l_local[r] == 0.0f) ? 0.0f : (1.0f / l_local[r]);
        auto v_inv = SIMD::set1(inv_l);

        simd_for(d,
                 [&](size_t x) {
                     SIMD::mov(o_ptr + x, SIMD::mul(SIMD::pull(o_local_row + x), v_inv));
                 },
                 [&](size_t x) { o_ptr[x] = o_local_row[x] * inv_l; }
                 );
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

#endif



///OLD CODE THAT IS FASTER

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



// ADD KERNELS

#include <simd_provider.h>

namespace job::ai::comp {
using namespace job::simd;


__attribute__((always_inline))
inline float dot(const float* __restrict__ a,
                 const float* __restrict__ b,
                 size_t n)
{
    constexpr int K = SIMD::width();
    size_t i = 0;

    // Accumulators
    f32 sum0 = SIMD::zero();
    f32 sum1 = SIMD::zero();
    f32 sum2 = SIMD::zero();
    f32 sum3 = SIMD::zero();

    // 1. Unrolled Vector Loop (Process 4 * K floats per iter)
    // For AVX2 (K=8), this processes 32 floats per iteration.
    for (; i + 4 * K <= n; i += 4 * K) {
        // Load A
        auto a0 = SIMD::pull(a + i + 0 * K);
        auto a1 = SIMD::pull(a + i + 1 * K);
        auto a2 = SIMD::pull(a + i + 2 * K);
        auto a3 = SIMD::pull(a + i + 3 * K);

        // Load B
        auto b0 = SIMD::pull(b + i + 0 * K);
        auto b1 = SIMD::pull(b + i + 1 * K);
        auto b2 = SIMD::pull(b + i + 2 * K);
        auto b3 = SIMD::pull(b + i + 3 * K);

        // FMA
        sum0 = SIMD::mul_plus(a0, b0, sum0);
        sum1 = SIMD::mul_plus(a1, b1, sum1);
        sum2 = SIMD::mul_plus(a2, b2, sum2);
        sum3 = SIMD::mul_plus(a3, b3, sum3);
    }

    // Reduce Accumulators 4 -> 1
    sum0 = SIMD::add(sum0, sum1);
    sum2 = SIMD::add(sum2, sum3);
    sum0 = SIMD::add(sum0, sum2);

    // 2. Remaining Vectors (Process 1 * K floats per iter)
    for (; i + K <= n; i += K) {
        auto va = SIMD::pull(a + i);
        auto vb = SIMD::pull(b + i);
        sum0 = SIMD::mul_plus(va, vb, sum0);
    }

    // 3. Horizontal reduction
    float total = hsum(sum0);

    // 4. Scalar Tail
    for (; i < n; ++i) {
        total += a[i] * b[i];
    }

    return total;
}

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
        // This handles my char/motif embeddings if they are huge (e.g. 256/512)
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
                S_local[r][c] = dot(q_ptr, k_ptr, d) * scale;
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
        float *o_local_row = O_local_flat + (r * d);

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
