#pragma once

#include "job_fmm_concepts.h"
#include <cmath>

namespace job::science {

// SHIFT SHIFT SHIFT  __glibcxx_shiftSHIFT  SHIFT SHIFT SHIFT SHIFT SHIFT __glibcxx_shift
template <typename T_Vec, typename T_Scalar>
struct FmmKernels {
    using Coeffs = FmmCoefficients<T_Vec, T_Scalar>;

    static void P2M(Coeffs &multipole, const T_Vec &particlePos, T_Scalar mass, const T_Vec &leafCenter)
    {
        // monopole
        multipole.monopole += mass;

        // dipole
        T_Vec r = particlePos - leafCenter;
        multipole.dipole = multipole.dipole + (r * mass);

        // quadrupole
        T_Scalar rx2 = r.x * r.x;
        T_Scalar ry2 = r.y * r.y;
        T_Scalar rz2 = r.z * r.z;

        multipole.q_xx += mass * rx2;
        multipole.q_yy += mass * ry2;
        multipole.q_zz += mass * rz2;
        multipole.q_xy += mass * r.x * r.y;
        multipole.q_xz += mass * r.x * r.z;
        multipole.q_yz += mass * r.y * r.z;

        // octupole
        multipole.o_xxx += mass * rx2 * r.x;
        multipole.o_yyy += mass * ry2 * r.y;
        multipole.o_zzz += mass * rz2 * r.z;

        multipole.o_xxy += mass * rx2 * r.y;
        multipole.o_xxz += mass * rx2 * r.z;
        multipole.o_xyy += mass * r.x * ry2;
        multipole.o_yyz += mass * ry2 * r.z;
        multipole.o_xzz += mass * r.x * rz2;
        multipole.o_yzz += mass * r.y * rz2;

        multipole.o_xyz += mass * r.x * r.y * r.z;
    }

    static void M2M(Coeffs &parent, const Coeffs &child, const T_Vec &shiftVec)
    {
        // mass/monopole
        parent.monopole += child.monopole;

        // dipole
        T_Vec shiftMoment = shiftVec * child.monopole;
        parent.dipole = parent.dipole + child.dipole + shiftMoment;

        // quadrupolet
        auto shiftQ = [&](T_Scalar &parentQ, T_Scalar childQ, T_Scalar termD, T_Scalar termM) {
            parentQ += childQ + termD + termM;
        };

        shiftQ(parent.q_xx, child.q_xx, 2*shiftVec.x*child.dipole.x, child.monopole*shiftVec.x*shiftVec.x);
        shiftQ(parent.q_yy, child.q_yy, 2*shiftVec.y*child.dipole.y, child.monopole*shiftVec.y*shiftVec.y);
        shiftQ(parent.q_zz, child.q_zz, 2*shiftVec.z*child.dipole.z, child.monopole*shiftVec.z*shiftVec.z);

        shiftQ(parent.q_xy, child.q_xy, shiftVec.x*child.dipole.y + shiftVec.y*child.dipole.x, child.monopole*shiftVec.x*shiftVec.y);
        shiftQ(parent.q_xz, child.q_xz, shiftVec.x*child.dipole.z + shiftVec.z*child.dipole.x, child.monopole*shiftVec.x*shiftVec.z);
        shiftQ(parent.q_yz, child.q_yz, shiftVec.y*child.dipole.z + shiftVec.z*child.dipole.y, child.monopole*shiftVec.y*shiftVec.z);

        // octupole "shift" .......... MY HEAD .............
        auto shiftO = [&](T_Scalar &parentO, T_Scalar childO, T_Scalar dQ_sum, T_Scalar ddD_sum, T_Scalar mddd) {
            parentO += childO + dQ_sum + ddD_sum + mddd;
        };

        T_Scalar dx2 = shiftVec.x*shiftVec.x;
        T_Scalar dy2 = shiftVec.y*shiftVec.y;
        T_Scalar dz2 = shiftVec.z*shiftVec.z;

        // Oxxx
        shiftO(parent.o_xxx, child.o_xxx,
               3*shiftVec.x*child.q_xx,
               3*dx2*child.dipole.x,
               child.monopole*dx2*shiftVec.x);

        // Oyyy
        shiftO(parent.o_yyy, child.o_yyy,
               3*shiftVec.y*child.q_yy,
               3*dy2*child.dipole.y,
               child.monopole*dy2*shiftVec.y);

        // Ozzz
        shiftO(parent.o_zzz, child.o_zzz,
               3*shiftVec.z*child.q_zz,
               3*dz2*child.dipole.z,
               child.monopole*dz2*shiftVec.z);

        // Oxxy
        shiftO(parent.o_xxy, child.o_xxy,
               2*shiftVec.x*child.q_xy + shiftVec.y*child.q_xx,
               dx2*child.dipole.y + 2*shiftVec.x*shiftVec.y*child.dipole.x,
               child.monopole*dx2*shiftVec.y);

        // Oxxz
        shiftO(parent.o_xxz, child.o_xxz,
               2*shiftVec.x*child.q_xz + shiftVec.z*child.q_xx,
               dx2*child.dipole.z + 2*shiftVec.x*shiftVec.z*child.dipole.x,
               child.monopole*dx2*shiftVec.z);

        // Oxyy
        shiftO(parent.o_xyy, child.o_xyy,
               2*shiftVec.y*child.q_xy + shiftVec.x*child.q_yy,
               dy2*child.dipole.x + 2*shiftVec.x*shiftVec.y*child.dipole.y,
               child.monopole*shiftVec.x*dy2);

        // Oyyz
        shiftO(parent.o_yyz, child.o_yyz,
               2*shiftVec.y*child.q_yz + shiftVec.z*child.q_yy,
               dy2*child.dipole.z + 2*shiftVec.y*shiftVec.z*child.dipole.y,
               child.monopole*dy2*shiftVec.z);

        // Oxzz
        shiftO(parent.o_xzz, child.o_xzz,
               2*shiftVec.z*child.q_xz + shiftVec.x*child.q_zz,
               dz2*child.dipole.x + 2*shiftVec.x*shiftVec.z*child.dipole.z,
               child.monopole*shiftVec.x*dz2);

        // Oyzz
        shiftO(parent.o_yzz, child.o_yzz,
               2*shiftVec.z*child.q_yz + shiftVec.y*child.q_zz,
               dz2*child.dipole.y + 2*shiftVec.y*shiftVec.z*child.dipole.z,
               child.monopole*shiftVec.y*dz2);

        // Oxyz
        shiftO(parent.o_xyz, child.o_xyz,
               shiftVec.x*child.q_yz + shiftVec.y*child.q_xz + shiftVec.z*child.q_xy,
               shiftVec.x*shiftVec.y*child.dipole.z + shiftVec.x*shiftVec.z*child.dipole.y + shiftVec.y*shiftVec.z*child.dipole.x,
               child.monopole*shiftVec.x*shiftVec.y*shiftVec.z);
    }

    static void M2L(Coeffs &local, const Coeffs &mass, const T_Vec &shiftVec)
    {
        T_Scalar r2 = shiftVec.x*shiftVec.x + shiftVec.y*shiftVec.y + shiftVec.z*shiftVec.z;
        if (r2 < T_Scalar(1e-10))
            return;

        T_Scalar invR = T_Scalar(1)/std::sqrt(r2);
        T_Scalar invR2 = invR * invR;
        T_Scalar invR3 = invR2 * invR;
        T_Scalar invR5 = invR3 * invR2;
        T_Scalar invR7 = invR5 * invR2;
        T_Scalar invR9 = invR7 * invR2;

        // monopole
        local.monopole += -mass.monopole * invR;

        // gradiant
        T_Vec gradM = shiftVec * (mass.monopole * invR3);
        local.dipole = local.dipole + gradM;

        T_Scalar m_invR3 = mass.monopole * invR3;
        T_Scalar m_3_invR5 = mass.monopole * T_Scalar(3) * invR5;

        local.q_xx += m_invR3 - m_3_invR5 * shiftVec.x * shiftVec.x;
        local.q_yy += m_invR3 - m_3_invR5 * shiftVec.y * shiftVec.y;
        local.q_zz += m_invR3 - m_3_invR5 * shiftVec.z * shiftVec.z;
        local.q_xy += -m_3_invR5 * shiftVec.x * shiftVec.y;
        local.q_xz += -m_3_invR5 * shiftVec.x * shiftVec.z;
        local.q_yz += -m_3_invR5 * shiftVec.y * shiftVec.z;

        // dipole
        T_Scalar d_dot_r = mass.dipole.x*shiftVec.x + mass.dipole.y*shiftVec.y + mass.dipole.z*shiftVec.z;
        local.monopole += -(d_dot_r * invR3);

        T_Vec termD1 = shiftVec * (d_dot_r * T_Scalar(3) * invR5);
        T_Vec termD2 = mass.dipole * (-invR3);
        local.dipole = local.dipole + termD1 + termD2;

        // dipole -> Hessian
        // if you are confused here are note that should clear it up :D
        // H_ij += 3 * ( (D.r)*delta_ij + d_i*r_j + d_j*r_i ) * invR5 - 15 * (D.r)*r_i*r_j * invR7
        // T_Scalar d_dot_r_invR5_3 = d_dot_r * T_Scalar(3) * invR5; // Unused atm
        T_Scalar d_dot_r_invR7_15 = d_dot_r * T_Scalar(15) * invR7;

        auto addHessianD = [&](T_Scalar &h, T_Scalar ri, T_Scalar rj, T_Scalar di, T_Scalar dj, bool diag) {
            T_Scalar term1 = (diag ? d_dot_r : T_Scalar(0)) + di*rj + dj*ri;
            h += term1 * T_Scalar(3) * invR5 - ri*rj * d_dot_r_invR7_15;
        };

        addHessianD(local.q_xx, shiftVec.x, shiftVec.x, mass.dipole.x, mass.dipole.x, true);
        addHessianD(local.q_yy, shiftVec.y, shiftVec.y, mass.dipole.y, mass.dipole.y, true);
        addHessianD(local.q_zz, shiftVec.z, shiftVec.z, mass.dipole.z, mass.dipole.z, true);
        addHessianD(local.q_xy, shiftVec.x, shiftVec.y, mass.dipole.x, mass.dipole.y, false);
        addHessianD(local.q_xz, shiftVec.x, shiftVec.z, mass.dipole.x, mass.dipole.z, false);
        addHessianD(local.q_yz, shiftVec.y, shiftVec.z, mass.dipole.y, mass.dipole.z, false);

        // quadrupole -> Local (Pot, Grad)
        T_Scalar trQ = mass.q_xx + mass.q_yy + mass.q_zz;
        T_Scalar rQr = shiftVec.x*(mass.q_xx*shiftVec.x + mass.q_xy*shiftVec.y + mass.q_xz*shiftVec.z) +
                       shiftVec.y*(mass.q_xy*shiftVec.x + mass.q_yy*shiftVec.y + mass.q_yz*shiftVec.z) +
                       shiftVec.z*(mass.q_xz*shiftVec.x + mass.q_yz*shiftVec.y + mass.q_zz*shiftVec.z);

        local.monopole += T_Scalar(-0.5) * (T_Scalar(3) * rQr * invR5 - trQ * invR3);

        T_Vec Qr;
        Qr.x = mass.q_xx*shiftVec.x + mass.q_xy*shiftVec.y + mass.q_xz*shiftVec.z;
        Qr.y = mass.q_xy*shiftVec.x + mass.q_yy*shiftVec.y + mass.q_yz*shiftVec.z;
        Qr.z = mass.q_xz*shiftVec.x + mass.q_yz*shiftVec.y + mass.q_zz*shiftVec.z;

        T_Vec termQ1 = Qr * (T_Scalar(3) * invR5);
        T_Vec termQ2 = shiftVec * (T_Scalar(7.5) * rQr * invR7);
        T_Vec termQ3 = shiftVec * (T_Scalar(1.5) * trQ * invR5);

        local.dipole = local.dipole - termQ1 + termQ2 - termQ3;
        //////////

        // octupole
        // The Next nightmare HELP ME .... MY HEAD and FINGERS !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        T_Scalar rOr =
            mass.o_xxx*shiftVec.x*shiftVec.x*shiftVec.x +
            mass.o_yyy*shiftVec.y*shiftVec.y*shiftVec.y +
            mass.o_zzz*shiftVec.z*shiftVec.z*shiftVec.z +
            T_Scalar(3) * (mass.o_xxy*shiftVec.x*shiftVec.x*shiftVec.y + mass.o_xxz*shiftVec.x*shiftVec.x*shiftVec.z +
                           mass.o_xyy*shiftVec.x*shiftVec.y*shiftVec.y + mass.o_yyz*shiftVec.y*shiftVec.y*shiftVec.z +
                           mass.o_xzz*shiftVec.x*shiftVec.z*shiftVec.z + mass.o_yzz*shiftVec.y*shiftVec.z*shiftVec.z) +
            T_Scalar(6)*mass.o_xyz*shiftVec.x*shiftVec.y*shiftVec.z;

        // trace vector
        T_Vec traceVec;
        traceVec.x = mass.o_xxx + mass.o_xyy + mass.o_xzz;
        traceVec.y = mass.o_yyy + mass.o_xxy + mass.o_yzz;
        traceVec.z = mass.o_zzz + mass.o_xxz + mass.o_yyz;

        T_Scalar r_dot_V = traceVec.x*shiftVec.x + traceVec.y*shiftVec.y + traceVec.z*shiftVec.z;

        local.monopole += -(T_Scalar(2.5) * rOr * invR7 - T_Scalar(0.5) * r_dot_V * invR5);

        // gradient calculation
        T_Vec gradientVec;
        T_Scalar xx=shiftVec.x*shiftVec.x; T_Scalar yy=shiftVec.y*shiftVec.y; T_Scalar zz=shiftVec.z*shiftVec.z;
        T_Scalar xy=shiftVec.x*shiftVec.y; T_Scalar xz=shiftVec.x*shiftVec.z; T_Scalar yz=shiftVec.y*shiftVec.z;

        gradientVec.x = mass.o_xxx*xx + mass.o_xyy*yy + mass.o_xzz*zz + T_Scalar(2)*(mass.o_xxy*xy + mass.o_xxz*xz + mass.o_xyz*yz);
        gradientVec.y = mass.o_yyy*yy + mass.o_xxy*xx + mass.o_yzz*zz + T_Scalar(2)*(mass.o_xyy*xy + mass.o_yyz*yz + mass.o_xyz*xz);
        gradientVec.z = mass.o_zzz*zz + mass.o_xxz*xx + mass.o_yyz*yy + T_Scalar(2)*(mass.o_xzz*xz + mass.o_yzz*yz + mass.o_xyz*xy);

        // This seems "straight forward and easy to understand"
        T_Vec g1 = gradientVec * (T_Scalar(7.5) * invR7);
        T_Vec g2 = shiftVec * (rOr * T_Scalar(-17.5) * invR9);
        T_Vec g3 = traceVec * (T_Scalar(-0.5) * invR5);
        T_Vec g4 = shiftVec * (r_dot_V * T_Scalar(2.5) * invR7);

        // gradient !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        local.dipole = local.dipole + g1 + g2 + g3 + g4;
    }

    static void L2L(Coeffs &localChild, const Coeffs &localParent, const T_Vec &shift)
    {
        // shift potential (C0)
        T_Scalar gradDotShift = localParent.dipole.x * shift.x +
                                localParent.dipole.y * shift.y +
                                localParent.dipole.z * shift.z;

        T_Scalar shiftQshift = shift.x*(localParent.q_xx*shift.x + localParent.q_xy*shift.y + localParent.q_xz*shift.z) +
                               shift.y*(localParent.q_xy*shift.x + localParent.q_yy*shift.y + localParent.q_yz*shift.z) +
                               shift.z*(localParent.q_xz*shift.x + localParent.q_yz*shift.y + localParent.q_zz*shift.z);

        // order 3 potential shift term: 1/6 * Sum O_ijk s_i s_j s_k
        // reuse the contraction logic from M2L but for shift vector 's'
        T_Scalar sOs =
            localParent.o_xxx*shift.x*shift.x*shift.x + localParent.o_yyy*shift.y*shift.y*shift.y + localParent.o_zzz*shift.z*shift.z*shift.z +
            T_Scalar(3)*(localParent.o_xxy*shift.x*shift.x*shift.y + localParent.o_xxz*shift.x*shift.x*shift.z +
                           localParent.o_xyy*shift.x*shift.y*shift.y + localParent.o_yyz*shift.y*shift.y*shift.z +
                           localParent.o_xzz*shift.x*shift.z*shift.z + localParent.o_yzz*shift.y*shift.z*shift.z) +
            T_Scalar(6)*localParent.o_xyz*shift.x*shift.y*shift.z;

        localChild.monopole += localParent.monopole + gradDotShift + T_Scalar(0.5) * shiftQshift + (sOs / T_Scalar(6.0));

        // shift gradient (C1)
        // G_child(0) = G_parent(shift)
        // order 0: C1 -> order 1: Q . shift -> order 2: 0.5 * O . shift^2 (Vector contraction)
        T_Vec Qs;
        Qs.x = localParent.q_xx * shift.x + localParent.q_xy * shift.y + localParent.q_xz * shift.z;
        Qs.y = localParent.q_xy * shift.x + localParent.q_yy * shift.y + localParent.q_yz * shift.z;
        Qs.z = localParent.q_xz * shift.x + localParent.q_yz * shift.y + localParent.q_zz * shift.z;

        T_Vec Oss; // O . shift . shift (Vector)
        // contraction similar to L2P force calc
        T_Scalar sx2=shift.x*shift.x; T_Scalar sy2=shift.y*shift.y; T_Scalar sz2=shift.z*shift.z;
        T_Scalar sxy=shift.x*shift.y; T_Scalar sxz=shift.x*shift.z; T_Scalar syz=shift.y*shift.z;

        Oss.x = localParent.o_xxx*sx2 + localParent.o_xyy*sy2 + localParent.o_xzz*sz2 + T_Scalar(2)*(localParent.o_xxy*sxy + localParent.o_xxz*sxz + localParent.o_xyz*syz);
        Oss.y = localParent.o_yyy*sy2 + localParent.o_xxy*sx2 + localParent.o_yzz*sz2 + T_Scalar(2)*(localParent.o_xyy*sxy + localParent.o_yyz*syz + localParent.o_xyz*sxz);
        Oss.z = localParent.o_zzz*sz2 + localParent.o_xxz*sx2 + localParent.o_yyz*sy2 + T_Scalar(2)*(localParent.o_xzz*sxz + localParent.o_yzz*syz + localParent.o_xyz*sxy);

        localChild.dipole = localChild.dipole + localParent.dipole + Qs + Oss * T_Scalar(0.5);

        // shift Hessian (Q)
        // Q_child(0) = Q_parent(shift)
        // order 0: Q -> order 1: O . shift (tensor contraction O_ijk s_k)

        // Qxx' = Qxx + Oxxk s_k
        auto shiftH = [&](T_Scalar& h, T_Scalar q, T_Scalar ox, T_Scalar oy, T_Scalar oz) {
            h += q + ox*shift.x + oy*shift.y + oz*shift.z;
        };

        shiftH(localChild.q_xx, localParent.q_xx, localParent.o_xxx, localParent.o_xxy, localParent.o_xxz);
        shiftH(localChild.q_yy, localParent.q_yy, localParent.o_xyy, localParent.o_yyy, localParent.o_yyz);
        shiftH(localChild.q_zz, localParent.q_zz, localParent.o_xzz, localParent.o_yzz, localParent.o_zzz);
        shiftH(localChild.q_xy, localParent.q_xy, localParent.o_xxy, localParent.o_xyy, localParent.o_xyz);
        shiftH(localChild.q_xz, localParent.q_xz, localParent.o_xxz, localParent.o_xyz, localParent.o_xzz);
        shiftH(localChild.q_yz, localParent.q_yz, localParent.o_xyz, localParent.o_yyz, localParent.o_yzz);

        // shift octupole (O)
        // O_child = O_parent (Constant for P=3 cutoff)
        localChild.o_xxx += localParent.o_xxx;
        localChild.o_yyy += localParent.o_yyy;
        localChild.o_zzz += localParent.o_zzz;
        localChild.o_xxy += localParent.o_xxy;
        localChild.o_xxz += localParent.o_xxz;
        localChild.o_xyy += localParent.o_xyy;
        localChild.o_yyz += localParent.o_yyz;
        localChild.o_xzz += localParent.o_xzz;
        localChild.o_yzz += localParent.o_yzz;
        localChild.o_xyz += localParent.o_xyz;
    }

    template <typename T_Body, typename T_Traits>
    static void L2P(T_Body &body, const Coeffs &localLeaf, const T_Vec &leafCenter)
    {
        T_Vec pos = T_Traits::position(body);
        T_Vec dx = pos - leafCenter;

        // gradient Contribution
        // Force = -Gradient
        T_Vec force = localLeaf.dipole * T_Scalar(-1);

        // Hessian contribution
        // Force -= Q * dx
        T_Scalar qx = localLeaf.q_xx * dx.x + localLeaf.q_xy * dx.y + localLeaf.q_xz * dx.z;
        T_Scalar qy = localLeaf.q_xy * dx.x + localLeaf.q_yy * dx.y + localLeaf.q_yz * dx.z;
        T_Scalar qz = localLeaf.q_xz * dx.x + localLeaf.q_yz * dx.y + localLeaf.q_zz * dx.z;

        force.x -= qx;
        force.y -= qy;
        force.z -= qz;

        // octupole contribution
        // Force -= 0.5 * (O contract dx dx)
        T_Scalar xx = dx.x*dx.x; T_Scalar yy = dx.y*dx.y; T_Scalar zz = dx.z*dx.z;
        T_Scalar xy = dx.x*dx.y; T_Scalar xz = dx.x*dx.z; T_Scalar yz = dx.y*dx.z;

        T_Vec octForce;
        octForce.x = localLeaf.o_xxx*xx + localLeaf.o_xyy*yy + localLeaf.o_xzz*zz +
                     T_Scalar(2)*(localLeaf.o_xxy*xy + localLeaf.o_xxz*xz + localLeaf.o_xyz*yz);

        octForce.y = localLeaf.o_yyy*yy + localLeaf.o_xxy*xx + localLeaf.o_yzz*zz +
                     T_Scalar(2)*(localLeaf.o_xyy*xy + localLeaf.o_yyz*yz + localLeaf.o_xyz*xz);

        octForce.z = localLeaf.o_zzz*zz + localLeaf.o_xxz*xx + localLeaf.o_yyz*yy +
                     T_Scalar(2)*(localLeaf.o_xzz*xz + localLeaf.o_yzz*yz + localLeaf.o_xyz*xy);

        T_Vec octCorrection = octForce * T_Scalar(-0.5);
        force = force + octCorrection;

        T_Traits::applyForce(body, force);
    }
};

} // namespace job::science
