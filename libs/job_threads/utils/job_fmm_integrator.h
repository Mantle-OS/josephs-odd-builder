#pragma once

#include <job_logger.h>


#include "job_thread_pool.h"
#include "utils/job_parallel_for.h"

#include "job_fmm_concepts.h"
#include "job_fmm_kernel.h"
#include "job_fmm_utils.h"

namespace job::threads {

using namespace job::threads;

template <typename T_Body, typename T_Vec, typename T_Scalar, typename T_Traits>
class JobFmmEngine {

public:
    using Node    = FmmNode<T_Vec, T_Scalar>;
    using Coeffs  = FmmCoefficients<T_Vec, T_Scalar>;
    using Kernel  = FmmKernels<T_Vec, T_Scalar>;
    using Bodies  = std::vector<T_Body>;

    friend struct FmmTreeBuilder<T_Body, T_Vec, T_Scalar, T_Traits>;
    using Builder = FmmTreeBuilder<T_Body, T_Vec, T_Scalar, T_Traits>;

    struct Params {
        int         maxLeafSize = 32;
        int         maxDepth = 16;
        T_Scalar    theta = T_Scalar(0.5);
    };

    JobFmmEngine(ThreadPool::Ptr pool, Params params):
        m_pool(std::move(pool)),
        m_params(params)
    {
        if(!m_pool)
            JOB_LOG_WARN("[JobFmmEngine] No Pool What in the hell are you up too....");
    }

    JobFmmEngine(ThreadPool &pool, Params params):
        m_pool(&pool, [](ThreadPool*){}),
        m_params(params)
    {
    }


    void compute(Bodies &bodies)
    {
        if (bodies.empty())
            return;

        typename Builder::Config buildCfg {
            m_params.maxLeafSize,
            m_params.maxDepth
        };

        Builder::build(bodies, buildCfg, m_nodes,
                       m_indices, m_levels, m_maxDepthUsed);

        m_multipoles.assign(m_nodes.size(), Coeffs{});
        m_locals.assign(m_nodes.size(), Coeffs{});

        // Upward: P2M + M2M
        runUpwardPass(bodies);
        // Far-field interactions: M2L on all nodes
        runFarFieldPass();
        // Downward: L2L parent -> child
        runDownwardPass();
        // Near-field: P2P + L2P on leaves
        runNearFieldAndL2P(bodies);
    }

private:
    void runUpwardPass(const Bodies &bodies)
    {
        for (int d = m_maxDepthUsed; d >= 0; --d) {
            const auto &levelNodes = m_levels[d];

            // RUN FOREST RUN !!!!
            parallel_for(*m_pool, size_t{0}, levelNodes.size(), [&](size_t i) {

                int nodeIdx = levelNodes[i];
                Node &node = m_nodes[nodeIdx];
                Coeffs &multipole = m_multipoles[nodeIdx];

                if (node.isLeaf()) {
                    for (size_t k = 0; k < node.particleCount; ++k) {
                        const auto &body = bodies[m_indices[node.firstParticle + k]];
                        Kernel::P2M(multipole, T_Traits::position(body), T_Traits::mass(body), node.center);
                    }
                } else {
                    for (int childIdx : node.children) {
                        if (childIdx != -1) {
                            T_Vec shift = m_nodes[childIdx].center - node.center;
                            Kernel::M2M(multipole, m_multipoles[childIdx], shift);
                        }
                    }
                }

            });
        }
    }


    void runDownwardPass()
    {
        // Propagate locals from root to leaves because that is what we do around here. I guess
        for (int d = 0; d < m_maxDepthUsed; ++d) {
            const auto &levelNodes = m_levels[d];

            parallel_for(*m_pool, size_t{0}, levelNodes.size(), [&](size_t i) {
                int parentIdx = levelNodes[i];
                const Node &parent = m_nodes[parentIdx];
                Coeffs &parentLocal = m_locals[parentIdx];

                for (int childIdx : parent.children) {
                    if (childIdx == -1) continue;

                    Node &child = m_nodes[childIdx];
                    Coeffs &childLocal = m_locals[childIdx];

                    T_Vec shift = child.center - parent.center; // child − parent
                    Kernel::L2L(childLocal, parentLocal, shift);
                }
            });
        }
    }


    void runFarFieldPass()
    {
        // Far-field M2L for every node (internal + leaves)
        parallel_for(*m_pool, size_t{0}, m_nodes.size(), [&](size_t t) {
            int targetIdx = static_cast<int>(t);
            const Node &target = m_nodes[targetIdx];

            int stack[256];
            int stackPtr = 0;
            stack[stackPtr++] = 0; // root as source

            while (stackPtr > 0) {
                int sourceIdx = stack[--stackPtr];
                const Node &source = m_nodes[sourceIdx];

                // We do *not* skip target == source - just treat it as near-field
                if (FmmTopology<T_Vec, T_Scalar>::isWellSeparated(target, source, m_params.theta)) {
                    // FAR FIELD -> M2L
                    T_Vec r = target.center - source.center; // target − source
                    Kernel::M2L(m_locals[targetIdx], m_multipoles[sourceIdx], r);
                    // do not descend
                } else {
                    // NEAR FIELD, handled later via P2P
                    if (!source.isLeaf()) {
                        for (int child : source.children) {
                            if (child != -1 && stackPtr < 256)
                                stack[stackPtr++] = child;
                        }
                    }
                }
            }
        });
    }

    void traverseUnified(int targetIdx, int rootIdx, Bodies &bodies)
    {
        const Node &target = m_nodes[targetIdx];

        int stack[256];
        int stackPtr = 0;
        stack[stackPtr++] = rootIdx;

        while(stackPtr > 0) {
            int sourceIdx = stack[--stackPtr];

            if (targetIdx == sourceIdx)
                continue; // <-- this nukes self-leaf interactions

            const Node &source = m_nodes[sourceIdx];

            if (FmmTopology<T_Vec, T_Scalar>::isWellSeparated(target, source, m_params.theta)) {
                T_Vec r = target.center - source.center;
                Kernel::M2L(m_locals[targetIdx], m_multipoles[sourceIdx], r);
            } else {
                if (source.isLeaf()) {
                    computeP2P(target, source, bodies);
                } else {
                    for (int child : source.children) {
                        if (child != -1) {
                            if(stackPtr < 256)
                                stack[stackPtr++] = child;
                        }
                    }
                }
            }
        }
    }


    void traverseNear(int targetIdx, int rootIdx, Bodies &bodies)
    {
        const Node &target = m_nodes[targetIdx];

        int stack[256];
        int stackPtr = 0;
        stack[stackPtr++] = rootIdx;

        while (stackPtr > 0) {
            int sourceIdx = stack[--stackPtr];
            const Node &source = m_nodes[sourceIdx];

            // Far-field already handled in runFarFieldPass via M2L
            if (FmmTopology<T_Vec, T_Scalar>::isWellSeparated(target, source, m_params.theta))
                continue;

            // Near-field (including self)
            if (source.isLeaf()) {
                computeP2P(target, source, bodies);
            } else {
                for (int child : source.children) {
                    // give me back my curly brace ... you dont need it
                    if (child != -1 && stackPtr < 256)
                        stack[stackPtr++] = child;
                }
            }
        }
    }

    void runNearFieldAndL2P(Bodies &bodies)
    {
        std::vector<int> leaves;
        leaves.reserve(m_nodes.size());
        for (const auto &n : m_nodes)
            if (n.isLeaf())
                leaves.push_back(n.index);

        parallel_for(*m_pool, size_t{0}, leaves.size(), [&](size_t i) {
            int leafIdx = leaves[i];
            Node &leaf = m_nodes[leafIdx];

            // Near-field P2P
            traverseNear(leafIdx, 0, bodies);

            // L2P using fully propagated local expansion
            for (size_t k = 0; k < leaf.particleCount; ++k) {
                auto &body = bodies[m_indices[leaf.firstParticle + k]];
                Kernel::template L2P<T_Body, T_Traits>(body, m_locals[leafIdx], leaf.center);
            }
        });
    }



    void computeP2P(const Node &targetLeaf, const Node &sourceLeaf, Bodies &bodies)
    {
        for (size_t i = 0; i < targetLeaf.particleCount; ++i) {
            auto &targetBody = bodies[m_indices[targetLeaf.firstParticle + i]];
            T_Vec posI = T_Traits::position(targetBody);

            for (size_t j = 0; j < sourceLeaf.particleCount; ++j) {
                if (targetLeaf.index == sourceLeaf.index && i == j)
                    continue;

                const auto &sourceBody = bodies[m_indices[sourceLeaf.firstParticle + j]];
                T_Vec r = T_Traits::position(sourceBody) - posI;
                T_Scalar r2 = r.x*r.x + r.y*r.y + r.z*r.z;

                // in a world where 1e-10 makes everything seem okay
                if (r2 > 1e-10) {
                    T_Scalar invR = T_Scalar(1)/std::sqrt(r2);
                    T_Scalar invR3 = invR * invR * invR;
                    T_Vec f = r * (T_Traits::mass(sourceBody) * invR3);
                    T_Traits::applyForce(targetBody, f);
                }
            }
        }
    }

    ThreadPool::Ptr                 m_pool;
    Params                          m_params;
    std::vector<Node>               m_nodes;
    std::vector<size_t>             m_indices;
    std::vector<std::vector<int>>   m_levels;
    int                             m_maxDepthUsed{0};
    std::vector<Coeffs>             m_multipoles;
    std::vector<Coeffs>             m_locals;
};

} // namespace job::threads
