#pragma once

#include <vector>
#include <cstddef>
#include <cmath>
#include <limits>
#include <utility>
#include <algorithm>

#include <job_logger.h>

#include "job_thread_pool.h"
#include "utils/job_parallel_for.h"
#include "utils/job_verlet_concepts.h"

namespace job::threads {

enum class ExpansionOrder {
    Monopole = 0,
    Dipole = 1,
    Quadrupole = 2
};

// !!!!!! VERY VERY IMPORANT !!!!!!!!!
// HERE ARE THE SIGS THAT ONE NEEDS TO HAVE
template <typename T_Body, typename T_Vec, typename T_Scalar>
struct FmmKernelTraits {
    // static T_Vec     position(const T_Body &body);
    // static T_Scalar  mass(const T_Body &body);
    // static void      applyForce(T_Body &body, const T_Vec &force);
};

template <FloatScalar T_Scalar>
struct FmmParams {
    int             maxLeafSize     = 32;
    int             maxDepth        = 16;
    T_Scalar        theta           = T_Scalar(0.5);
    ExpansionOrder  expansionOrder  = ExpansionOrder::Dipole;
};


// "Multipole"
template <typename T_Vec, FloatScalar T_Scalar>
struct FmmMultipole {
    T_Scalar mass{0};                           // Total charge/mass
    T_Vec    center{0,0,0};                     // Expansion center (cell center, NOT COM)
    T_Vec    dipole{0,0,0};                     // Dipole moment: d = Σ m_i · (r_i - center)
    T_Scalar diagX{0}, diagY{0}, diagZ{0};      // Diagonal
    T_Scalar diagXY{0}, diagXZ{0}, diagYZ{0};   // Off-diagonal (symmetric tensor)
};

// "Target"
template <typename T_Vec, FloatScalar T_Scalar>
struct FmmLocal {
    T_Vec    field{0,0,0};
    T_Scalar potential{0};
    T_Vec    fieldGradient{0,0,0};  // We'll use this in in Quadrupole ? L2L(using gradients, etc). That’s the real “math dragon” part.
};

template <typename T_Vec, FloatScalar T_Scalar>
struct FmmNode {
    T_Vec                           center;
    T_Scalar                        halfSize;
    int                             depth           = 0;
    int                             index           = -1; // Index in the linear node array
    int                             parent          = -1;
    int                             children[8]     = { -1, -1, -1, -1, -1, -1, -1, -1 };
    std::size_t                     firstParticle   = 0;
    std::size_t                     particleCount   = 0;
    FmmMultipole<T_Vec, T_Scalar>   multipole;              // up
    FmmLocal<T_Vec, T_Scalar>       local;                  // down
    [[nodiscard]] bool isLeaf() const
    {
        return children[0] == -1;
    }
};


template <typename T_Body, typename T_Vec, typename T_Scalar,
         typename T_Traits = FmmKernelTraits<T_Body, T_Vec, T_Scalar>>
class JobFmm {
public:
    using Body   = T_Body;
    using Vec    = T_Vec;
    using Node   = FmmNode<Vec, T_Scalar>;
    using Traits = T_Traits;

    JobFmm(ThreadPool::Ptr pool, FmmParams<T_Scalar> params = {}) :
        m_pool(std::move(pool)),
        m_params(params)
    {
        if (!m_pool)
            JOB_LOG_WARN("[JobFmm] ThreadPool is null; THIS IS BAD REAL BAD !");
    }

    // Build the Tree (Same logic as Barnes-Hut, "adapted"(very badly because I am dumb !!!!!!!!!!!!) for FMM structure)
    void buildTree(std::vector<Body> &bodies)
    {
        if (bodies.empty())
            return;

        // resets
        m_nodes.clear();
        m_indices.resize(bodies.size());
        for (size_t i = 0; i < bodies.size(); ++i)
            m_indices[i] = i;

        // boounding
        Vec minPos = T_Traits::position(bodies[0]);
        Vec maxPos = minPos;

        for (const auto &b : bodies) {
            Vec p = T_Traits::position(b);

            if (p.x < minPos.x)
                minPos.x = p.x;

            if (p.y < minPos.y)
                minPos.y = p.y;

            if (p.z < minPos.z)
                minPos.z = p.z;

            if (p.x > maxPos.x)
                maxPos.x = p.x;

            if (p.y > maxPos.y)
                maxPos.y = p.y;

            if (p.z > maxPos.z)
                maxPos.z = p.z;
        }

        Vec center = (minPos + maxPos) * T_Scalar(0.5);
        Vec size   = (maxPos - minPos) * T_Scalar(0.5);
        T_Scalar halfSize = std::max({size.x, size.y, size.z}) * T_Scalar(1.001);


        Node root;
        root.center = center;
        root.halfSize = halfSize;
        root.depth = 0;
        root.index = 0;
        root.firstParticle = 0;
        root.particleCount = bodies.size();
        m_nodes.push_back(root);

        // recursive
        // FIXME: In a real "HPC" imply, use a loop stack ? but recursion is fine for < 50 depth. which im to ......
        subdivide(0, bodies);

        // cache
        m_levels.assign(m_params.maxDepth + 1, {});
        for (const auto &node : m_nodes)
            m_levels[node.depth].push_back(node.index);
    }

    // vcycle try....
    void compute(std::vector<Body> &bodies)
    {
        if (m_nodes.empty())
            return;

        if (!m_pool) {
            JOB_LOG_WARN("[JobFmm] compute() called with null ThreadPool");
            return;
        }

        // "Upward Pass" (P2M + M2M) IE -> Start from bottom level, move up
        for (int d = m_levels.size() - 1; d >= 0; --d) {
            const auto &levelIndices = m_levels[d];
            if (levelIndices.empty())
                continue;

            parallel_for(*m_pool, size_t{0}, levelIndices.size(), [&](size_t i) {
                int nodeIdx = levelIndices[i];
                Node &node = m_nodes[nodeIdx];

                // Clear old data
                node.multipole = {};
                node.local = {};

                if (node.isLeaf())
                    P2M(node, bodies);
                else
                    M2M(node);
            });
        }
        // M2L ??????
        // For every node, gather influence from "well-separated" colleagues I should do this parallel by level or just all nodes.
        parallel_for(*m_pool, size_t{0}, m_nodes.size(), [&](size_t i) {
            // interact(static_cast<int>(i));
            P2P(0, static_cast<int>(i)); // I NEVER GET HERE WITH Dipole with BREAKPOINTS !!!!!!
        });

        for (int d = 1; d < static_cast<int>(m_levels.size()); ++d) {
            const auto &levelIndices = m_levels[d];
            if (levelIndices.empty())
                continue;

            parallel_for(*m_pool, size_t{0}, levelIndices.size(), [&](size_t i) {
                int nodeIdx = levelIndices[i];
                Node &node = m_nodes[nodeIdx];
                Node &parent = m_nodes[node.parent];
                L2L(node, parent);
            });
        }

        // const auto &leaves = m_nodes; // optimization: keep a separate list of leaves ?
        parallel_for(*m_pool, size_t{0}, m_nodes.size(), [&](size_t i) {
            Node &node = m_nodes[i];
            if (node.isLeaf())
                L2P(node, bodies);
        });

        // Serial: near-field P2P (touches shared bodies)
        for (Node &node : m_nodes) {
            if (node.isLeaf())
                handleNearField(node, bodies);
        }
    }


    const std::vector<Node> &nodes() const noexcept
    {
        return m_nodes;
    }


private:
    void upwardPass(const std::vector<Body> &/*bodies*/)
    {

    }

    void downwardPass(std::vector<Body> &/*bodies*/)
    {

    }


    [[nodiscard]] int createChildNode(int parentIndex, int octant, const Vec &center, T_Scalar halfSize)
    {
        Node &parent = m_nodes[parentIndex];
        Node child;
        child.center   = center;
        child.halfSize = halfSize;
        child.depth    = parent.depth + 1;
        child.parent   = parentIndex;

        int childIdx = static_cast<int>(m_nodes.size());
        m_nodes.push_back(child);

        parent.children[octant] = childIdx;

        return childIdx;
    }

    void computeNodeBoundingBox(const Node &node, const std::vector<Body> &bodies, Vec &centerOut, T_Scalar &halfSizeOut)
    {
        if (node.particleCount == 0) {
            centerOut = node.center;
            halfSizeOut = node.halfSize;
            return;
        }

        std::size_t first = node.firstParticle;
        std::size_t last  = node.firstParticle + node.particleCount;

        Vec minPos = Traits::position(bodies[m_indices[first]]);
        Vec maxPos = minPos;

        for (std::size_t i = first + 1; i < last; ++i) {
            Vec pos = Traits::position(bodies[m_indices[i]]);
            minPos.x = std::min(minPos.x, pos.x);
            minPos.y = std::min(minPos.y, pos.y);
            minPos.z = std::min(minPos.z, pos.z);
            maxPos.x = std::max(maxPos.x, pos.x);
            maxPos.y = std::max(maxPos.y, pos.y);
            maxPos.z = std::max(maxPos.z, pos.z);
        }

        centerOut = (minPos + maxPos) * T_Scalar(0.5);

        T_Scalar extentX = (maxPos.x - minPos.x) * T_Scalar(0.5);
        T_Scalar extentY = (maxPos.y - minPos.y) * T_Scalar(0.5);
        T_Scalar extentZ = (maxPos.z - minPos.z) * T_Scalar(0.5);

        halfSizeOut = std::max({extentX, extentY, extentZ}) * T_Scalar(1.001);
    }



    void subdivide(int nodeIdx, std::vector<Body> &bodies)
    {
        Node &node = m_nodes[nodeIdx];

        // leaf conditions already met ?
        if (node.particleCount <= static_cast<size_t>(m_params.maxLeafSize) ||
            node.depth >= m_params.maxDepth)
            return;

        // particles per octant
        std::size_t counts[8] = {0};
        std::size_t first = node.firstParticle;
        std::size_t last = node.firstParticle + node.particleCount;

        for (std::size_t i = first; i < last; ++i) {
            Vec pos = T_Traits::position(bodies[m_indices[i]]);
            int octant = 0;

            if (pos.x >= node.center.x)
                octant |= 4;

            if (pos.y >= node.center.y)
                octant |= 2;

            if (pos.z >= node.center.z)
                octant |= 1;

            counts[octant]++;
        }

        // prefix sum
        std::size_t offsets[9];
        offsets[0] = first;
        for (int o = 0; o < 8; ++o)
            offsets[o + 1] = offsets[o] + counts[o];

        // Partition indices by octant
        std::vector<std::size_t> temp(m_indices.begin() + first, m_indices.begin() + last);
        std::size_t writePos[8];
        for (int o = 0; o < 8; ++o)
            writePos[o] = offsets[o];

        for (std::size_t i = 0; i < node.particleCount; ++i) {
            std::size_t idx = temp[i];
            Vec pos = T_Traits::position(bodies[idx]);

            int octant = 0;
            if (pos.x >= node.center.x)
                octant |= 4;

            if (pos.y >= node.center.y)
                octant |= 2;

            if (pos.z >= node.center.z)
                octant |= 1;

            m_indices[writePos[octant]++] = idx;
        }

        // "new" (not really new)  children
        T_Scalar childHalfSize = node.halfSize * T_Scalar(0.5);

        for (int octant = 0; octant < 8; ++octant) {
            if (counts[octant] == 0)
                continue;

            Vec childCenter = node.center;
            childCenter.x += (octant & 4) ? childHalfSize : -childHalfSize;
            childCenter.y += (octant & 2) ? childHalfSize : -childHalfSize;
            childCenter.z += (octant & 1) ? childHalfSize : -childHalfSize;

            int childIdx = static_cast<int>(m_nodes.size());

            Node child;
            child.depth = node.depth + 1;
            child.index = childIdx;
            child.parent = nodeIdx;
            child.firstParticle = offsets[octant];
            child.particleCount = counts[octant];

            // Shrink-Wrap!
            // Instead of using the geometric center/size, we fit to content.
            Vec tightCenter;
            T_Scalar tightHalf;
            computeNodeBoundingBox(child, bodies, tightCenter, tightHalf);

            child.center   = tightCenter;
            child.halfSize =  tightHalf; //std::min(childHalfSize, tightHalf);

            m_nodes.push_back(child);
            m_nodes[nodeIdx].children[octant] = childIdx;

            // KEEP GOING !!!!!
            subdivide(childIdx, bodies);
        }
    }

    void interact(int targetIdx)
    {
        // Node &target = m_nodes[targetIdx]; // VERY VERY CONFUSING
        P2P(0, targetIdx);
    }

    void P2P(int sourceIdx, int targetIdx)
    {
        if (sourceIdx == targetIdx)
            return;  // Don't interact with self (handled by P2P) <<< BAM !!!!!!!!

        Node &source = m_nodes[sourceIdx];
        Node &target = m_nodes[targetIdx];

        if (isWellSeparated(source, target)) {
            // "Far enough" use multipole
            M2L(target, source);
        } else {
            // Too close
            if (source.isLeaf() && target.isLeaf()) {
                // defer to P2P ???already handled in handleNearField ?
                return;
            } else if (!source.isLeaf()) {
                // Open source
                for (int child : source.children) {
                    if (child != -1)
                        P2P(child, targetIdx);
                }
            } else {
                // open target if not leaf
                for (int child : target.children) {
                    if (child != -1)
                        P2P(sourceIdx, child);
                }
            }
        }
    }


    // "P2P" calc neighbors VERY BROKEN
    void handleNearField(Node &leaf, std::vector<Body> &bodies)
    {
        computeDirectFromTree(0, leaf, bodies);
    }

    void computeDirectFromTree(int sourceIdx, Node &targetLeaf, std::vector<Body> &bodies)
    {
        Node &source = m_nodes[sourceIdx];

        // If they're well separated, far field (M2L) has already handled them.
        if (isWellSeparated(targetLeaf, source))
            return;

        if (source.isLeaf()) {
            if (source.index == targetLeaf.index) {
                // Self-leaf: classic N^2 in that leaf (i < j)
                for (size_t i = 0; i < targetLeaf.particleCount; ++i) {
                    Body &bi = bodies[m_indices[targetLeaf.firstParticle + i]];
                    for (size_t j = i + 1; j < targetLeaf.particleCount; ++j) {
                        Body &bj = bodies[m_indices[targetLeaf.firstParticle + j]];
                        applyP2P(bi, bj);
                    }
                }
            } else {
                // Only do cross-leaf interactions in one direction
                if (source.index > targetLeaf.index)
                    return; // pair will be handled when that leaf is the target

                // Neighbor leaf: full pair between targetLeaf and source
                for (size_t i = 0; i < targetLeaf.particleCount; ++i) {
                    Body &bi = bodies[m_indices[targetLeaf.firstParticle + i]];
                    for (size_t j = 0; j < source.particleCount; ++j) {
                        Body &bj = bodies[m_indices[source.firstParticle + j]];
                        applyP2P(bi, bj);
                    }
                }
            }
        } else {
            // Source is internal: recurse into its children
            for (int child : source.children) {
                if (child != -1)
                    computeDirectFromTree(child, targetLeaf, bodies);
            }
        }
    }



    /////////////////////////
    // start kernels
    /////////////////////////


    // "Particle to Multipole"
    void P2M(Node &node, const std::vector<Body> &bodies)
    {
        node.multipole.center = node.center;

        T_Scalar totalMass{0};
        T_Vec    dipole{0,0,0};

        for (size_t i = 0; i < node.particleCount; ++i) {
            const Body &body = bodies[m_indices[node.firstParticle + i]];
            T_Scalar mass = T_Traits::mass(body);
            // T_Vec    pos = T_Traits::position(body); // Just keeping aroundf in case Quadrupole uses it. For now dipole only calls this in getting the realitive pos

            totalMass += mass;

            T_Vec rel = T_Traits::position(body) - node.center;
            dipole = dipole + rel * mass;
        }

        node.multipole.mass   = totalMass;
        node.multipole.dipole = dipole; // who cares if this is in Monopole not a big deal.
    }


    // "Multipole to Multipole"
    void M2M(Node &node)
    {
        node.multipole.center = node.center;
        T_Scalar totalMass{0};
        T_Vec    totalDipole{0,0,0};

        for (int childIdx : node.children) {
            if (childIdx == -1)
                continue;

            const Node &child  = m_nodes[childIdx];
            const auto &childM = child.multipole;

            totalMass += childM.mass;

            // translation of multipole expansion
            T_Vec offset          = child.multipole.center - node.center;
            T_Vec translatedDipole = childM.dipole + offset * childM.mass;

            totalDipole = totalDipole + translatedDipole;
        }

        node.multipole.mass   = totalMass;
        node.multipole.dipole = totalDipole;
    }

    // "Multipole to Local"
    void M2L(Node &target, const Node &source)
    {
        if (source.multipole.mass == T_Scalar(0))
            return;

        T_Vec r  = target.center - source.multipole.center;
        T_Scalar r2 = r.lengthSq();
        if (r2 < T_Scalar(1e-10))
            return;

        T_Scalar rlen = std::sqrt(r2);
        T_Scalar invR = T_Scalar(1) / rlen;
        T_Scalar invR3 = invR * invR * invR;

        T_Vec field = r * (source.multipole.mass * invR3);


        // DEBUG: Log the first few M2L calls
        static std::atomic<int> callCount{0};
        int myCall = callCount.fetch_add(1, std::memory_order_relaxed);
        if (myCall < 5) {
            JOB_LOG_INFO("[M2L #{}] source.mass={}, source.dipole=({},{},{}), expansionOrder={}",
                         myCall,
                         source.multipole.mass,
                         source.multipole.dipole.x,
                         source.multipole.dipole.y,
                         source.multipole.dipole.z,
                         static_cast<int>(m_params.expansionOrder));
        }


        if (m_params.expansionOrder >= ExpansionOrder::Dipole) {
            T_Scalar invR5   = invR3 * invR * invR;
            T_Scalar d_dot_r = dotProduct(source.multipole.dipole, r);

            T_Vec fieldDip =
                r * (d_dot_r * T_Scalar(-3) * invR5) +
                source.multipole.dipole * invR3;


            if (myCall < 5) {
                JOB_LOG_INFO("[M2L #{}] fieldMonopole=({},{},{}), fieldDip=({},{},{})",
                             myCall,
                             field.x, field.y, field.z,
                             fieldDip.x, fieldDip.y, fieldDip.z);
            }


            field = field + fieldDip;
        }

        // Note for future self field here is an acceleration-like quantity, not force.
        target.local.field = target.local.field + field;
    }


    // "Local to Local"
    void L2L(Node &child, const Node &parent)
    {
        // True FMM L2L does a little Taylor magic to re-expand the local expansion around the child center (using gradients, etc). That’s the real “math dragon” part.
        child.local.field = child.local.field + parent.local.field;

        // FOR LATER IF even needed should not be.
        // if(m_params.expansionOrder == ExpansionOrder::Dipole || m_params.expansionOrder == ExpansionOrder::Monopole){
        // }else{
        //     // FIXME
        // }
    }

    // "Local to Particle"
    void L2P(Node &node, std::vector<Body> &bodies)
    {
        for (size_t i = 0; i < node.particleCount; ++i) {
            Body &body = bodies[m_indices[node.firstParticle + i]];
            T_Traits::applyForce(body, node.local.field);
        }
    }
    // end kernels


    void applyP2P(Body &a, Body &b)
    {
        T_Vec posA = T_Traits::position(a);
        T_Vec posB = T_Traits::position(b);

        T_Vec r = posB - posA;          // from A to B
        T_Scalar rSq = r.lengthSq();

        // Softening / safety: avoid singularity when very close
        const T_Scalar eps2 = T_Scalar(1e-8);
        if (rSq < eps2)
            rSq = eps2;

        T_Scalar invR  = T_Scalar(1) / std::sqrt(rSq);
        T_Scalar invR3 = invR * invR * invR;

        T_Scalar mA = T_Traits::mass(a);
        T_Scalar mB = T_Traits::mass(b);

        // Acceleration on A due to B: a_A = mB * r / r^3
        T_Vec accelA = r * (mB * invR3);

        // Acceleration on B due to A: a_B = -mA * r / r^3
        T_Vec accelB = r * (-mA * invR3);

        T_Traits::applyForce(a, accelA);
        T_Traits::applyForce(b, accelB);
    }

    [[nodiscard]] bool isWellSeparated(const Node &a, const Node &b) const
    {
        if (a.index == b.index)
            return false;

        T_Vec d = a.center - b.center;
        T_Scalar dist2 = d.lengthSq();

        // Avoid divide by zero if somehow halfSize is 0 and dist ~ 0
        if (dist2 <= T_Scalar(1e-20))
            return false;

        T_Scalar effectiveTheta = m_params.theta;
        if (m_params.expansionOrder == ExpansionOrder::Monopole)
            effectiveTheta *= T_Scalar(0.8); // more conservative

        T_Scalar dist = std::sqrt(dist2);
        T_Scalar size = a.halfSize + b.halfSize;

        return (size / dist) < effectiveTheta;
    }

    T_Scalar dotProduct(const T_Vec &a, const T_Vec &b) const
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    ThreadPool::Ptr                 m_pool;
    FmmParams<T_Scalar>             m_params;
    std::vector<Node>               m_nodes;
    std::vector<size_t>             m_indices;
    std::vector<std::vector<int>>   m_levels;


};

} // namespace job::threads
// CHECKPOINT v1.4
