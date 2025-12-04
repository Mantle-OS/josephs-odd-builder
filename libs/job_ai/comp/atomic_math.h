#pragma once

#include <atomic>
#include <cstdint>

namespace job::ai::comp {

// sum expert outputs into the final buffer without mutexes. opps ....
// !!!! Note !!!! memory_order_relaxed is usually "sufficient" for accumulation if there is a barrier (thread join) at the end.
__attribute__((always_inline))
inline void atomicAdd(float& target, float value)
{
    static_assert(std::atomic_ref<float>::is_always_lock_free,
                  "atomic_ref<float> must be lock-free for atomicAdd");

    std::atomic_ref<float> atom(target);
    float old_val = atom.load(std::memory_order_relaxed);
    float new_val;
    do {
        new_val = old_val + value;
    } while (!atom.compare_exchange_weak(old_val, new_val,
                                         std::memory_order_relaxed,
                                         std::memory_order_relaxed));
}

} // namespace job::ai::comp
