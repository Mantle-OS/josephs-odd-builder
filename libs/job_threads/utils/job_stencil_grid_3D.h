#pragma once

#include <algorithm>
#include <vector>

#include "job_thread_pool.h"
#include "utils/job_stencil_boundary.h"
#include "utils/job_parallel_for.h"

namespace job::threads {

template <typename T>
class GridReader3D {
public:
    GridReader3D(const T *data, int width, int height, int depth) :
        m_data(data),
        m_width(width),
        m_height(height),
        m_depth(depth),
        m_strideXY(width * height)
    {
    }

    [[nodiscard]] const T &operator()(int x, int y, int z) const
    {
        return m_data[z * m_strideXY + y * m_width + x];
    }

    [[nodiscard]] T at(int x, int y, int z, BoundaryMode mode) const
    {
        switch (mode) {
        case BoundaryMode::Clamp:
            x = std::clamp(x, 0, m_width - 1);
            y = std::clamp(y, 0, m_height - 1);
            z = std::clamp(z, 0, m_depth - 1);
            break;
        case BoundaryMode::Wrap:
            x = (x % m_width + m_width) % m_width;
            y = (y % m_height + m_height) % m_height;
            z = (z % m_depth + m_depth) % m_depth;
            break;
        case BoundaryMode::FixedZero:
            if (x < 0 || x >= m_width || y < 0 || y >= m_height || z < 0 || z >= m_depth)
                return T{};
            break;
        }
        return m_data[z * m_strideXY + y * m_width + x];
    }

    [[nodiscard]] int width() const
    {
        return m_width;
    }

    [[nodiscard]] int height() const
    {
        return m_height;
    }

    [[nodiscard]] int depth() const
    {
        return m_depth;
    }

private:
    const T *m_data;
    int m_width;
    int m_height;
    int m_depth;
    int m_strideXY;
};


template <typename T_Cell>
class JobStencilGrid3D {
public:
    JobStencilGrid3D(ThreadPool &pool, int width, int height, int depth, T_Cell init = T_Cell{}) :
        m_pool(pool),
        m_width(std::max(1, width)),
        m_height(std::max(1, height)),
        m_depth(std::max(1, depth)),
        m_strideXY(m_width * m_height),
        m_buffers{std::vector<T_Cell>(static_cast<size_t>(m_width) * m_height * m_depth, init),
                  std::vector<T_Cell>(static_cast<size_t>(m_width) * m_height * m_depth, init)}
    {
    }

    T_Cell *data()
    {
        return m_buffers[m_front].data();
    }

    const T_Cell *data() const
    {
        return m_buffers[m_front].data();
    }

    [[nodiscard]] int width() const noexcept
    {
        return m_width;
    }

    [[nodiscard]] int height() const noexcept
    {
        return m_height;
    }

    [[nodiscard]] int depth() const noexcept
    {
        return m_depth;
    }

    // SIG T_Cell(int x, int y, int z, GridReader3D<T_Cell> view);
    template <typename Kernel>
    void step(Kernel &&kernel)
    {
        const T_Cell *readPtr = m_buffers[m_front].data();
        T_Cell *writePtr = m_buffers[1 - m_front].data();

        GridReader3D<T_Cell> reader(readPtr, m_width, m_height, m_depth);
        std::size_t total = static_cast<std::size_t>(m_strideXY) * m_depth;

        parallel_for(m_pool, std::size_t{0}, total, [&](std::size_t idx) {
            // Decode 1D index to 3D coordinates
            int z = static_cast<int>(idx / m_strideXY);
            int rem = static_cast<int>(idx % m_strideXY);
            int y = rem / m_width;
            int x = rem % m_width;

            writePtr[idx] = kernel(x, y, z, reader);
        });

        m_front = 1 - m_front;
    }

    template <typename Kernel>
    void step_n(std::size_t iterations, Kernel &&kernel)
    {
        for (std::size_t i = 0; i < iterations; ++i)
            step(kernel);
    }

    void set(int x, int y, int z, T_Cell val)
    {
        if (x >= 0 && x < m_width && y >= 0 && y < m_height && z >= 0 && z < m_depth)
            m_buffers[m_front][z * m_strideXY + y * m_width + x] = val;
    }

private:
    ThreadPool              &m_pool;
    int                     m_width;
    int                     m_height;
    int                     m_depth;
    int                     m_strideXY;
    std::vector<T_Cell>     m_buffers[2];
    int                     m_front{0};
};

} // namespace job::threads
// CHECKPOINT: v1.1
