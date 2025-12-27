#pragma once

#include <vector>
#include <type_traits>
#include <utility>

namespace job::science {

// static_assert helper
template <class...> struct always_false : std::false_type {};

template <typename T_Particle,
         typename T_Vec,
         typename ForceFn,
         typename GetMass,
         typename GetAcc>
auto makeForceToAccelAdapter(ForceFn &&force_fn, GetMass &&get_mass, GetAcc &&get_acc)
{
    return [force = std::forward<ForceFn>(force_fn),
            gm = std::forward<GetMass>(get_mass),
            ga = std::forward<GetAcc>(get_acc),
            forceBuff = std::vector<T_Vec>{} ](std::vector<T_Particle> &ps) mutable
    {
        // calculate forces
        if constexpr (std::is_invocable_r_v<std::vector<T_Vec>, ForceFn, const std::vector<T_Particle>&>) {
            // Function allocates and returns (Slower, but supported)
            forceBuff = force(ps);
        } else if constexpr (std::is_invocable_r_v<void, ForceFn, const std::vector<T_Particle>&, std::vector<T_Vec>&>) {
            // Function fills buffer (Zero-Alloc Fast Path)
            if (forceBuff.size() != ps.size())
                forceBuff.resize(ps.size());
            force(ps, forceBuff);
        } else {
            static_assert(always_false<ForceFn>::value, "ForceFn must be either vector<T_Vec>(ps) or void(ps, out_forces)");
        }

        // apply F=ma
        const size_t n = ps.size();
        for (std::size_t i = 0; i < n; ++i) {
            auto &particle      = ps[i];
            auto &acceleration  = ga(particle);   // acceleration ref
            auto mass           = gm(particle);   // mass

            // ensure we don't divide by zero or int truncation
            using Scalar = std::remove_reference_t<decltype(mass)>;
            auto inv = Scalar(1) / mass;

            acceleration = forceBuff[i] * inv;
        }
    };
}

}
