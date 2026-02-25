#pragma once
#include <memory>
#include <atomic>
#include "genome.h"
namespace job::ai::learn {
class ILearn {
public:
    using Ptr   = std::shared_ptr<ILearn>;
    using UPtr  = std::unique_ptr<ILearn>;
    virtual ~ILearn() = default;

    // The Core Loop:
    // Load Genome (Flywheel)
    // Run Simulation (Physics + Inference)
    // Update the m_fitness
    // Return m_fitness Higher is better at 100%
    [[nodiscard]] virtual float learn(const evo::Genome &genome)  = 0;

    // Optional: Returns the input dimension required by this environment
    [[nodiscard]] virtual uint32_t inputDimension() const noexcept = 0;

    // Optional: Returns the output/action dimension required
    [[nodiscard]] virtual uint32_t outputDimension() const noexcept = 0;

    virtual void onTokenTime([[maybe_unused]]uint64_t generation, [[maybe_unused]]uint64_t seed) {}

    float constexpr fitness() const noexcept{return m_fitness;}
    bool  constexpr done() const noexcept{ return m_done.load(std::memory_order_acquire);}

protected:
    float              m_fitness = 0.0f;
    std::atomic<bool>   m_done{false};
};
}
