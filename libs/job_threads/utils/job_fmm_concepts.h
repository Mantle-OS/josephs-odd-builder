#pragma once

#include <concepts>

namespace job::threads {

template <typename T>
concept FmmScalar = std::floating_point<T>;

template <typename V, typename S>
concept FmmVec = requires(V v, S s) {
    { v.x } -> std::convertible_to<S>;
    { v.y } -> std::convertible_to<S>;
    { v.z } -> std::convertible_to<S>;
    { v + v } -> std::same_as<V>;
    { v * s } -> std::same_as<V>;
};

template <typename T_Vec, typename T_Scalar>
struct alignas(64) FmmCoefficients {
    // C0
    T_Scalar    monopole{0};

    // C1
    T_Vec       dipole{0,0,0};

    // Quadrupole / Hessian
    T_Scalar    q_xx{0};
    T_Scalar    q_yy{0};
    T_Scalar    q_zz{0};
    T_Scalar    q_xy{0};
    T_Scalar    q_xz{0};
    T_Scalar    q_yz{0};

    // Octupole (Order 3)
    // 10 terms: 3 cubic, 6 mixed-1, 1 mixed-all
    T_Scalar    o_xxx{0};
    T_Scalar    o_yyy{0};
    T_Scalar    o_zzz{0};
    T_Scalar    o_xxy{0};
    T_Scalar    o_xxz{0};
    T_Scalar    o_xyy{0};
    T_Scalar    o_yyz{0};
    T_Scalar    o_xzz{0};
    T_Scalar    o_yzz{0};
    T_Scalar    o_xyz{0};

    void clear()
    {
        monopole = T_Scalar(0);
        dipole = {0, 0, 0};

        q_xx=0; q_yy=0; q_zz=0;
        q_xy=0; q_xz=0; q_yz=0;

        o_xxx=0; o_yyy=0; o_zzz=0;
        o_xxy=0; o_xxz=0; o_xyy=0;
        o_yyz=0; o_xzz=0; o_yzz=0;
        o_xyz=0;
    }

    FmmCoefficients &operator+=(const FmmCoefficients &other)
    {
        monopole += other.monopole;
        dipole = dipole + other.dipole;

        q_xx += other.q_xx; q_yy += other.q_yy; q_zz += other.q_zz;
        q_xy += other.q_xy; q_xz += other.q_xz; q_yz += other.q_yz;

        o_xxx += other.o_xxx; o_yyy += other.o_yyy; o_zzz += other.o_zzz;
        o_xxy += other.o_xxy; o_xxz += other.o_xxz; o_xyy += other.o_xyy;
        o_yyz += other.o_yyz; o_xzz += other.o_xzz; o_yzz += other.o_yzz;
        o_xyz += other.o_xyz;

        return *this;
    }
};

// The Topology
template <typename T_Vec, typename T_Scalar>
struct FmmNode {
    T_Vec       center;
    T_Scalar    halfSize;
    int         depth{0};
    int         index{-1};
    int         parent{-1};
    int         children[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
    std::size_t firstParticle{0};
    std::size_t particleCount{0};

    // Checks ALL children, not just the first one.
    [[nodiscard]] bool isLeaf() const noexcept
    {
        for(int child : children)
            if(child != -1)
                return false;

        return true;
    }
};

} // namespace job::threads
