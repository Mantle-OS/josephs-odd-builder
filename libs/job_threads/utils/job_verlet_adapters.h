#pragma once

#include <vector>
#include <type_traits>
#include <utility>

namespace job::threads {

// static_asserts in dependent's contexts
template <class...> struct always_false : std::false_type {};

// Adapts a "force calculator" into an "acceleration calculator" expected by !!!YOUR!!! integrator.
// Accepted ForceFn signatures (choose one):
//   1) std::vector<T_Vec> fn(const std::vector<T_Particle>& particles);
//   2) void fn(const std::vector<T_Particle>& particles, std::vector<T_Vec>& out_forces);
// get_mass(p) must return a scalar convertible to whatever scalar !!!!YOUR!!!! T_Vec multiplies by.
// get_acc(p) must return a T_Vec& of the acceleration field to write into.

template <typename T_Particle, typename T_Vec, typename ForceFn, typename GetMass, typename GetAcc>

auto make_force_to_accel_adapter(ForceFn&& force_fn, GetMass&& get_mass, GetAcc&& get_acc)
{
    return [force = std::forward<ForceFn>(force_fn), gm = std::forward<GetMass>(get_mass), ga = std::forward<GetAcc>(get_acc)](std::vector<T_Particle>& ps) mutable
    {
        std::vector<T_Vec> F;
        if constexpr (std::is_invocable_r_v<std::vector<T_Vec>, ForceFn,
                                            const std::vector<T_Particle>&>) {
            // returns vector<T_Vec>
            F = force(ps);
        } else if constexpr (std::is_invocable_r_v<void, ForceFn,
                                                   const std::vector<T_Particle>&,
                                                   std::vector<T_Vec>&>){
            F.resize(ps.size());
            // fills out forces (out_forces)
            force(ps, F);
        } else {
            static_assert(always_false<ForceFn>::value, "ForceFn must be either vector<T_Vec>(ps) or void(ps, out_forces)");
        }

        // Convert forces to accelerations and write via get_acc
        for (std::size_t i = 0; i < ps.size(); ++i) {
            auto &p = ps[i];
            auto &acc = ga(const_cast<T_Particle&>(p));
            auto m = gm(p);

            // 1/m with correct scalar type ... I think
            auto  inv = decltype(m)(1) / m;

            // T_Vec * scalar ->> T_Vec
            acc = F[i] * inv;
        }
    };
}

} // namespace job::threads
// CHECKPOINT v1.1
