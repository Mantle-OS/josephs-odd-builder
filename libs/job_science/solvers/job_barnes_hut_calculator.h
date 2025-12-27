#pragma once

#include <limits>
#include <algorithm>

#include <job_thread_pool.h>
#include <utils/job_parallel_for.h>
#include <utils/job_parallel_reduce.h>

#include "job_barns_hut_concept.h"
#include "job_barnes_hut_tree.h"


namespace job::science {

template <typename T_Particle, typename T_Vec, core::FloatScalar T_Scalar>
    requires BarnesHutVector<T_Vec, T_Scalar>

class BarnesHutForceCalculator {
public:
    using GetPosFunc = std::function<const T_Vec&(const T_Particle&)>;
    using GetMassFunc = std::function<T_Scalar(const T_Particle&)>;
    using Tree = BarnesHutTree<T_Particle, T_Vec, T_Scalar>;

    /**
     * pool The thread pool for parallel steps.
     * get_pos Accessor for particle position.
     * get_mass Accessor for particle mass.
     * theta Accuracy parameter (e.g., 0.5).
     * G Gravitational constant.
     * epsilon_sq Softening factor (squared) to prevent singularities.
     */
    BarnesHutForceCalculator(threads::ThreadPool::Ptr pool, GetPosFunc get_pos, GetMassFunc get_mass,
                             T_Scalar theta = 0.5, T_Scalar G = 6.674e-11, T_Scalar epsilon_sq = 1e-6) :
        m_pool(pool),
        m_get_pos(get_pos),
        m_get_mass(get_mass),
        m_theta(theta),
        m_G(G),
        m_epsilon_sq(epsilon_sq)
    {

    }

    void calculate_forces(const std::vector<T_Particle>& particles, std::vector<T_Vec>& out_forces)
    {
        if (particles.empty()) {
            out_forces.clear();
            return;
        }

        if (!m_pool) {
            JOB_LOG_ERROR("[BarnesHutForceCalculator] ThreadPool is null; returning zero forces.");
            out_forces.assign(particles.size(), T_Vec{});
            return;
        }

        out_forces.resize(particles.size());


        auto bounds = findBoundingBox(particles);
        auto tree = std::make_unique<Tree>(bounds.min, bounds.max, m_get_pos, m_get_mass);
        for (const auto& p : particles)
            tree->insert(&p);

        tree->calculateCenterOfMass();

        parallel_for(*m_pool, (size_t)0, particles.size(), [&](size_t i) {
            out_forces[i] = tree->calculateForce(particles[i], m_theta, m_G, m_epsilon_sq);
        });
    }

private:
    struct BoundingBox {
        T_Vec min;
        T_Vec max;
    };

    // Need this for the bounding box init :>( sad....
    static constexpr T_Scalar T_Scalar_MAX = std::numeric_limits<T_Scalar>::max();

    BoundingBox findBoundingBox(const std::vector<T_Particle> &particles)
    {
        // bounding box of one particle
        auto map_fn = [&](const T_Particle& p) -> BoundingBox {
            return { m_get_pos(p), m_get_pos(p) };
        };

        // two boxes -> merge
        auto reduce_fn = [](BoundingBox a, BoundingBox b) -> BoundingBox {
            return {
                {std::min(a.min.x, b.min.x), std::min(a.min.y, b.min.y), std::min(a.min.z, b.min.z)},
                {std::max(a.max.x, b.max.x), std::max(a.max.y, b.max.y), std::max(a.max.z, b.max.z)}
            };
        };

        BoundingBox init_box{
            { T_Scalar_MAX,  T_Scalar_MAX,  T_Scalar_MAX },
            {-T_Scalar_MAX, -T_Scalar_MAX, -T_Scalar_MAX }
        };
        BoundingBox total_box = parallel_reduce(*m_pool, particles.begin(), particles.end(), init_box, map_fn, reduce_fn);

        // box -> cube -> padding
        T_Scalar max_dim = 0;
        max_dim = std::max(max_dim, total_box.max.x - total_box.min.x);
        max_dim = std::max(max_dim, total_box.max.y - total_box.min.y);
        max_dim = std::max(max_dim, total_box.max.z - total_box.min.z);

        if (max_dim <= std::numeric_limits<T_Scalar>::epsilon()) {
            max_dim = T_Scalar(1.0); // Default size
        }

        // 1% padding
        max_dim *= T_Scalar(1.01);

        T_Vec center    = (total_box.min + total_box.max) * T_Scalar(0.5);
        T_Vec half_size = {max_dim * T_Scalar(0.5), max_dim * T_Scalar(0.5), max_dim * T_Scalar(0.5)};

        total_box.min = center - half_size;
        total_box.max = center + half_size;

        return total_box;
    }

    threads::ThreadPool::Ptr    m_pool;
    GetPosFunc                  m_get_pos;
    GetMassFunc                 m_get_mass;
    T_Scalar                    m_theta;
    T_Scalar                    m_G;
    T_Scalar                    m_epsilon_sq;
};

} // namespace job::threads
// CHECKPOINT v1.0
