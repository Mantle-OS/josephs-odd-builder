#pragma once

#include <algorithm>
#include <vector>

#include <job_thread_pool.h>
#include <utils/job_parallel_for.h>
#include "job_stencil_boundary.h"

namespace job::science {

template <typename T>
class GridReader2D {
public:
    GridReader2D(const T *data, int width, int height) :
        m_data(data),
        m_width(width),
        m_height(height)
    {
    }

    // Fast but unsafe
    [[nodiscard]] const T &operator()(int x, int y) const
    {
        return m_data[y * m_width + x];
    }

    // Safe
    [[nodiscard]] T at(int x, int y, BoundaryMode mode) const
    {
        switch (mode) {
        case BoundaryMode::Clamp:
            x = std::clamp(x, 0, m_width - 1);
            y = std::clamp(y, 0, m_height - 1);
            break;
        case BoundaryMode::Wrap:
            x = (x % m_width + m_width) % m_width;
            y = (y % m_height + m_height) % m_height;
            break;
        case BoundaryMode::FixedZero:
            if (x < 0 || x >= m_width || y < 0 || y >= m_height)
                return T{};
            break;
        }
        return m_data[y * m_width + x];
    }

    [[nodiscard]] int width() const
    {
        return m_width;
    }

    [[nodiscard]] int height() const
    {
        return m_height;
    }

private:
    const T *m_data;
    int m_width;
    int m_height;
};

template <typename T_Cell>
class JobStencilGrid2D {
public:

    JobStencilGrid2D(threads::ThreadPool &pool, int width, int height, bool parallel = true, T_Cell init = T_Cell{}) :
        m_pool(pool),
        m_width(std::max(1, width)),
        m_height(std::max(1, height)),
        m_parallel(parallel),
        m_buffers{std::vector<T_Cell>(static_cast<size_t>(m_width) * m_height, init),
                  std::vector<T_Cell>(static_cast<size_t>(m_width) * m_height, init)}
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

    // SIG T_Cell(int x, int y, GridReader2D<T_Cell> view);
    template <typename Kernel>
    void step(Kernel &&kernel)
    {
        const T_Cell *readPtr = m_buffers[m_front].data();
        T_Cell *writePtr      = m_buffers[1 - m_front].data();

        GridReader2D<T_Cell> reader(readPtr, m_width, m_height);

        std::size_t total = static_cast<std::size_t>(m_width) * m_height;

        auto exec = [&](std::size_t idx) {
            int y = static_cast<int>(idx / m_width);
            int x = static_cast<int>(idx % m_width);
            writePtr[idx] = kernel(x, y, reader);
        };

        if(m_parallel){
            parallel_for(m_pool, std::size_t{0}, total, exec);
        }else{
            for (std::size_t i = 0; i < total; ++i)
                exec(i);
        }
        // Swappy swappy
        m_front = 1 - m_front;
    }

    // N simulations
    template <typename Kernel>
    void step_n(std::size_t iterations, Kernel &&kernel)
    {
        for (std::size_t i = 0; i < iterations; ++i)
            step(kernel);
    }

    // For initial seeding ONLY
    // WARNING: Not thread safe if simulation is running.
    void set(int x, int y, T_Cell val)
    {
        if (x >= 0 && x < m_width && y >= 0 && y < m_height)
            m_buffers[m_front][y * m_width + x] = val;
    }

private:
    threads::ThreadPool     &m_pool;
    int                     m_width;
    int                     m_height;
    bool                    m_parallel{true};
    std::vector<T_Cell>     m_buffers[2];
    int                     m_front{0};
};

} // namespace job::threads

