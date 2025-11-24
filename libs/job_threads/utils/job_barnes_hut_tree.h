#pragma once

#include <cmath>
#include <memory>
#include <array>
#include <functional>

#include "utils/job_verlet_concepts.h"

namespace job::threads {

template <typename T_Particle, typename T_Vec, FloatScalar T_Scalar>
    requires VecOps<T_Vec, T_Scalar>

class BarnesHutTree {
public:
    // Callbacks
    using GetPosFunc = std::function<const T_Vec&(const T_Particle&)>;
    using GetMassFunc = std::function<T_Scalar(const T_Particle&)>;

    // Node Struct
    struct Node {
        T_Vec               min_bound;
        T_Vec               max_bound;
        T_Scalar            width;
        T_Vec               center_of_mass;
        T_Scalar            total_mass{0};
        const T_Particle    *particle{nullptr};
        // FIXME (LATER) use the enums somehow or is it always 8 ? I thought that this could be 4 or 8
        std::array<std::unique_ptr<Node>, 8> children;
        [[nodiscard]] bool isLeaf() const noexcept
        {
            // just checking nullptr is too brittle for me maybe later add bool hasChildren()
            for (const auto &child : children)
                if (child)
                    return false;
            return true;
        }
    };


    BarnesHutTree(const T_Vec &min, const T_Vec &max, GetPosFunc get_pos, GetMassFunc get_mass ) :
        m_get_pos(get_pos),
        m_get_mass(get_mass)
    {
        m_root = std::make_unique<Node>();
        m_root->min_bound = min;
        m_root->max_bound = max;
        m_root->width = (max.x - min.x);
    }

    void insert(const T_Particle *p)
    {
        insertRecursive(m_root.get(), p);
    }

    void calculateCenterOfMass()
    {
        calculateComRecursive(m_root.get());
    }


    T_Vec calculateForce(const T_Particle &p, T_Scalar theta, T_Scalar G, T_Scalar epsilon_sq) const
    {
        return calculateForceRecursive(m_root.get(), p, theta, G, epsilon_sq);
    }

private:
    std::unique_ptr<Node> m_root;
    GetPosFunc m_get_pos;
    GetMassFunc m_get_mass;

    [[nodiscard]] int getOctant(const T_Vec &pos, const T_Vec &center) const noexcept
    {
        int octant = 0;
        if (pos.x >= center.x)
            octant |= 1;

        if (pos.y >= center.y)
            octant |= 2;

        if (pos.z >= center.z)
            octant |= 4;

        return octant;
    }

    void insertRecursive(Node *node, const T_Particle *p)
    {
        if (!node->isLeaf()) {
            int octant = getOctant(m_get_pos(*p), (node->min_bound + node->max_bound) * T_Scalar(0.5));
            insertRecursive(node->children[octant].get(), p);
            return;
        }

        if (node->particle == nullptr) {
            node->particle = p;
            return;
        }

        const T_Particle *existing_p = node->particle;
        node->particle = nullptr; // This node is now an internal node

        subdivide(node); // Create 8 children

        int octant_existing = getOctant(m_get_pos(*existing_p), (node->min_bound + node->max_bound) * T_Scalar(0.5));
        insertRecursive(node->children[octant_existing].get(), existing_p);

        int octant_new = getOctant(m_get_pos(*p), (node->min_bound + node->max_bound) * 0.5f);
        insertRecursive(node->children[octant_new].get(), p);
    }

    void subdivide(Node *node)
    {
        T_Vec center = (node->min_bound + node->max_bound) * T_Scalar(0.5);
        T_Vec half_size = (center - node->min_bound);

        for (int i = 0; i < 8; ++i) {
            T_Vec new_min = center;

            if ((i & 1) == 0)
                new_min.x -= half_size.x;

            if ((i & 2) == 0)
                new_min.y -= half_size.y;

            if ((i & 4) == 0)
                new_min.z -= half_size.z;

            T_Vec new_max = new_min + half_size;

            node->children[i] = std::make_unique<Node>();
            node->children[i]->min_bound = new_min;
            node->children[i]->max_bound = new_max;
            node->children[i]->width = half_size.x;
        }
    }

    // Post  traversal -> calculate CoM
    void calculateComRecursive(Node *node)
    {
        // Check if this is a leaf (has a particle, no children)
        if (node->particle != nullptr) {
            node->total_mass = m_get_mass(*node->particle);
            node->center_of_mass = m_get_pos(*node->particle);
        } else if (!node->isLeaf()) { // Check if this is an internal node
            T_Vec weighted_pos_sum = {};
            T_Scalar mass_sum = 0;

            for (auto &child : node->children) {
                if (child) {
                    calculateComRecursive(child.get());
                    mass_sum += child->total_mass;
                    weighted_pos_sum = weighted_pos_sum + (child->center_of_mass * child->total_mass);
                }
            }

            node->total_mass = mass_sum;
            if (mass_sum > 0)
                node->center_of_mass = weighted_pos_sum * (T_Scalar(1) / mass_sum);
        }
        // else: it's an empty node (no particle, no children), so mass remains 0
    }

    // s/d < theta logic
    T_Vec calculateForceRecursive(const Node *node, const T_Particle &p, T_Scalar theta, T_Scalar G, T_Scalar epsilon_sq) const
    {
        if (!node || node->total_mass == 0)
            return {};

        T_Vec r_vec = node->center_of_mass - m_get_pos(p);

        // Softened distance
        T_Scalar d_sq = r_vec.lengthSq() + epsilon_sq;
        T_Scalar d = std::sqrt(d_sq);

        // Check if node is a leaf
        if (node->particle != nullptr) {
            if (node->particle == &p)
                return {};
        }
        // Not a leaf, check s/d
        else {
            T_Scalar s = node->width;
            if ((s / d) < theta) {
                // Far enough: approximate this node as a single "big body" at its COM.
            } else {
                // Too close, recurse into children
                T_Vec total_force = {};
                for (const auto &child : node->children)
                    total_force = total_force + calculateForceRecursive(child.get(), p, theta, G, epsilon_sq);

                return total_force;
            }
        }

        // WE ARE CALCULATING A FORCE !!!!!!!!!!
        T_Scalar F_mag = G * m_get_mass(p) * node->total_mass;
        return r_vec * (F_mag / (d * d_sq)); // F_mag / d^3
    }
};

} // namespace job::threads

//CHECKPOINT v1.0
