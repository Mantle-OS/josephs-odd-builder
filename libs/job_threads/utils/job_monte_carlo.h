#pragma once

#include <memory>
#include <cstdint>

#include <split_mix64.h>

#include "job_thread_pool.h"
#include "job_parallel_reduce.h"

#include "job_monte_carlo_concept.h"

namespace job::threads {


// Okay we should have the following as template's or concepts
// Lets take a look at what Monte Carlo really is.
// 1) Predictive Model
// 2) Probability Distribution
// 3) Simulation


// If we break this down we have the following ?
// Intergrations and Mthods ?
enum class MonteCarloAlgo : uint8_t {
    HubbardStratonovich,      // Auxiliary Field
    Boltzmann,                // BioMOCA
    P3M,                      // BioMOCA
    // BioMOCA has sub groups [
            Box([2d,3d]Gauss theorem),
            Ion([size, protein, water, etc](Lennard-Jones and friends))
            EMF
            VMD
            Recording trajectories in binary
    DSMC(Boltzmann , Knudsen),
    DMC(non-equilibrium),
    Ergodicity (Birkhoff, Von Neumann ),
    Genetic,
    KMC(Rejection, NON),
    Morris,
    MLMC,
    Quasi,
    Sobol,
    Temporal,
};

template <std::floating_point T_Scalar>
class MonteCarloEngine {
public:
    explicit MonteCarloEngine(ThreadPool::Ptr pool) :
        m_pool(std::move(pool))
    {
        if (!m_pool) {
            JOB_LOG_ERROR("[MonteCarlo] ThreadPool is null!");
        }
    }

    template <typename Func>
    T_Scalar run1D(std::size_t samples, T_Scalar min, T_Scalar max, Func f, uint64_t seed = 0)
    {
        if (samples == 0)
            return 0.0;
        T_Scalar sum = parallel_reduce_ref(*m_pool, std::size_t(0), samples, T_Scalar(0),
                                       [=](std::size_t i) -> T_Scalar {
                                           job::core::SplitMix64 rng(seed + i);
                                           T_Scalar x = rng.range(min, max);
                                           return f(x);
                                       },
                                       std::plus<T_Scalar>{}
                                       );
        T_Scalar volume = max - min;
        T_Scalar average = sum / static_cast<T_Scalar>(samples);

        return volume * average;
    }



    template <typename Func>
    T_Scalar runND(std::size_t samples,
                         const std::vector<T_Scalar> &minBounds,
                         const std::vector<T_Scalar> &maxBounds,
                         Func f, uint64_t seed = 0)
    {
        std::size_t dims = minBounds.size();
        T_Scalar volume = 1.0;
        for(std::size_t d=0; d<dims; ++d) {
            volume *= (maxBounds[d] - minBounds[d]);
        }

        T_Scalar sum = parallel_reduce(*m_pool, std::size_t(0), samples, T_Scalar(0), [&](std::size_t i) -> T_Scalar {
            job::core::SplitMix64 rng(seed + i);
            std::vector<T_Scalar> x(dims);
            for(std::size_t d=0; d<dims; ++d)
                x[d] = rng.range(minBounds[d], maxBounds[d]);
            return f(x);
        }, std::plus<T_Scalar>{} );

        return volume * (sum / static_cast<T_Scalar>(samples));
    }



private:
    ThreadPool::Ptr m_pool;
};
} // namespace job::threads
