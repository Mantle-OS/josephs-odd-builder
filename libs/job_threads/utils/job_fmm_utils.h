#pragma once

#include <vector>
#include <algorithm>
#include <numeric>

#include "job_fmm_concepts.h"

namespace job::threads {

template <typename T_Vec, typename T_Scalar>
struct FmmTopology {
    using Node = FmmNode<T_Vec, T_Scalar>;



   // Important detail: this uses only |d|², not the direction, so it doesn’t care if you write a.center - b.center or b.center - a.center (the norm is symmetric).
   // So isWellSeparated is not imposing any directional convention; it just gives you the “interaction graph” between nodes.
   // The sign conventions all live in how you call the kernels.
    [[nodiscard]] static inline bool isWellSeparated(const Node &a, const Node &b, T_Scalar theta)
    {
        if (a.index == b.index)
            return false;

        T_Vec d = a.center - b.center;
        T_Scalar dist2 = d.x*d.x + d.y*d.y + d.z*d.z;

        // radius sum
        T_Scalar rSum = a.halfSize + b.halfSize;

        // Check: (rSum / dist) < theta
        return (rSum * rSum) < (dist2 * theta * theta);
    }

    template <typename T_Body, typename T_Traits>
    static void computeBounds(const std::vector<T_Body> &bodies, const std::vector<size_t> &indices,
                              size_t start, size_t count, T_Vec &centerOut, T_Scalar &halfSizeOut)
    {
        if (count == 0)
            return;

        T_Vec minP = T_Traits::position(bodies[indices[start]]);
        T_Vec maxP = minP;

        for (size_t i = 1; i < count; ++i) {
            T_Vec p = T_Traits::position(bodies[indices[start + i]]);

            if (p.x < minP.x)
                minP.x = p.x;

            if (p.x > maxP.x)
                maxP.x = p.x;

            if (p.y < minP.y)
                minP.y = p.y;

            if (p.y > maxP.y)
                maxP.y = p.y;

            if (p.z < minP.z)
                minP.z = p.z;

            if (p.z > maxP.z)
                maxP.z = p.z;
        }

        centerOut = (minP + maxP) * T_Scalar(0.5);
        T_Vec sizeVec = (maxP - minP) * T_Scalar(0.5);
        halfSizeOut = std::max({sizeVec.x, sizeVec.y, sizeVec.z}) * T_Scalar(1.001); // 0.1% buffer .... I think
    }
};


// The Constructor
template <typename T_Body, typename T_Vec, typename T_Scalar, typename T_Traits>
struct FmmTreeBuilder {
    using Node   = FmmNode<T_Vec, T_Scalar>;
    using Nodes  = std::vector<Node>;
    using Bodies = std::vector<T_Body>;
    struct Config {
        int maxLeafSize = 32;
        int maxDepth = 16;
    };

    static void build(Bodies &bodies, const Config &cfg, Nodes &nodes, std::vector<size_t> &indices,
                      std::vector<std::vector<int>> &levels, int &maxDepthUsed)
    {
        nodes.clear();
        indices.resize(bodies.size());
        std::iota(indices.begin(), indices.end(), 0);
        levels.clear();
        levels.resize(cfg.maxDepth + 1);
        maxDepthUsed = 0;

        if(bodies.empty())
            return;

        T_Vec rootCenter;
        T_Scalar rootHalf;
        FmmTopology<T_Vec, T_Scalar>::template computeBounds<T_Body, T_Traits>(bodies, indices, 0,
                                                                               bodies.size(), rootCenter, rootHalf);

        Node root;
        root.center = rootCenter;
        root.halfSize = rootHalf;
        root.index = 0;
        root.particleCount = bodies.size();
        root.firstParticle = 0;
        root.depth = 0;

        nodes.push_back(root);
        levels[0].push_back(0);

        // sub %
        std::vector<int> stack = {0};
        while (!stack.empty()) {
            int nodeIdx = stack.back();
            stack.pop_back();

            if (nodes[nodeIdx].particleCount <= static_cast<size_t>(cfg.maxLeafSize) ||
                nodes[nodeIdx].depth >= cfg.maxDepth) {
                continue;
            }

            subdivide(nodeIdx, bodies, nodes, indices, levels, stack);

            if (nodes[nodeIdx].depth + 1 > maxDepthUsed)
                maxDepthUsed = nodes[nodeIdx].depth + 1;
        }
    }

private:
    static void subdivide(int parentIdx, const Bodies &bodies, Nodes &nodes, std::vector<size_t> &indices,
                          std::vector<std::vector<int>> &levels, std::vector<int> &stack)
    {
        Node parentCopy = nodes[parentIdx];

        size_t start = parentCopy.firstParticle;
        size_t count = parentCopy.particleCount;

        std::vector<size_t> bins[8];
        for(int i = 0; i < 8; ++i)
            bins[i].reserve(count / 8);

        for (size_t i = 0; i < count; ++i) {
            size_t pIdx = indices[start + i];
            T_Vec p = T_Traits::position(bodies[pIdx]);

            int octant = 0;
            if (p.x >= parentCopy.center.x)
                octant |= 4;

            if (p.y >= parentCopy.center.y)
                octant |= 2;

            if (p.z >= parentCopy.center.z)
                octant |= 1;

            bins[octant].push_back(pIdx);
        }

        size_t currentWrite = start;
        T_Scalar childHalf = parentCopy.halfSize * T_Scalar(0.5);

        for (int i = 0; i < 8; ++i) {
            if (bins[i].empty())
                continue;

            for (size_t idx : bins[i])
                indices[currentWrite++] = idx;

            Node child;
            child.depth = parentCopy.depth + 1;
            child.parent = parentIdx;
            child.index = static_cast<int>(nodes.size());
            child.firstParticle = currentWrite - bins[i].size();
            child.particleCount = bins[i].size();

            child.center = parentCopy.center;
            child.center.x += (i & 4) ? childHalf : -childHalf;
            child.center.y += (i & 2) ? childHalf : -childHalf;
            child.center.z += (i & 1) ? childHalf : -childHalf;
            child.halfSize = childHalf;

            nodes.push_back(child);
            nodes[parentIdx].children[i] = child.index;
            levels[child.depth].push_back(child.index);
            stack.push_back(child.index);
        }
    }
};

} // namespace job::threads
