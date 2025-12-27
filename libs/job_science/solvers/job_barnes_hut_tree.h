#pragma once

#include <cmath>
#include <memory>
#include <array>
#include <functional>
#include <vector>

#include <real_type.h>

#include "job_barns_hut_concept.h"

namespace job::science {

template <typename T_Particle, typename T_Vec, core::FloatScalar T_Scalar>
    requires BarnesHutVector<T_Vec, T_Scalar>

class BarnesHutTree {
public:
    // When node width shrinks below this, stop subdividing and keep
    // multiple particles in a single leaf to avoid infinite recursion.
    static constexpr T_Scalar kMinNodeWidth = T_Scalar(1e-6);

    // Callbacks
    using GetPosFunc = std::function<const T_Vec&(const T_Particle&)>;
    using GetMassFunc = std::function<T_Scalar(const T_Particle&)>;

    // Node Struct
    struct Node {
        T_Vec                                   min_bound;
        T_Vec                                   max_bound;
        T_Scalar                                width{T_Scalar(0)};
        T_Vec                                   center_of_mass{};
        T_Scalar                                total_mass{0};

        // Allow multiple particles in a leaf, especially once width gets very small to avoid pathological subdivision.
        std::vector<const T_Particle*>          particles;

        std::array<std::unique_ptr<Node>, 8>    children;

        [[nodiscard]] bool isLeaf() const noexcept
        {
            for (const auto &child : children)
                if (child)
                    return false;
            return true;
        }

        [[nodiscard]] bool hasParticles() const noexcept
        {
            return !particles.empty();
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
        const T_Vec center = (node->min_bound + node->max_bound) * T_Scalar(0.5);

        // Internal node: just recurse into the appropriate child.
        if (!node->isLeaf()) {
            int octant = getOctant(m_get_pos(*p), center);
            insertRecursive(node->children[octant].get(), p);
            return;
        }

        // Leaf with no particles yet: just store it.
        if (!node->hasParticles()) {
            node->particles.push_back(p);
            return;
        }

        // Leaf already has particles.
        // If the node is already extremely small OR the new particle is (numerically) at the same spot as the existing ones,
        // we *do not* subdivide further. We just keep aggregating in this leaf. This prevents infinite recursion when multiple particles share essentially the same position.
        const T_Vec pos_new = m_get_pos(*p);
        const T_Vec pos_ref = m_get_pos(*node->particles.front());
        const T_Vec diff    = pos_new - pos_ref;

        const bool same_pos = (diff.lengthSq() == T_Scalar(0));
        const bool too_small = (node->width <= kMinNodeWidth);

        if (same_pos || too_small) {
            node->particles.push_back(p);
            return;
        }

        // Otherwise: this leaf has a particle and still has usable size. Promote it to an internal node:
        std::vector<const T_Particle*> existing = std::move(node->particles);
        node->particles.clear();

        subdivide(node); // Create 8 children

        // Reinsert existing particles into children
        for (const T_Particle* ep : existing) {
            int oct = getOctant(m_get_pos(*ep), center);
            insertRecursive(node->children[oct].get(), ep);
        }

        // Insert the new particle
        int oct_new = getOctant(pos_new, center);
        insertRecursive(node->children[oct_new].get(), p);
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
        if (node->hasParticles() && node->isLeaf()) {
            T_Vec    weighted_pos_sum{};
            T_Scalar mass_sum = T_Scalar(0);

            for (const T_Particle* p : node->particles) {
                const T_Scalar m = m_get_mass(*p);
                if (m <= T_Scalar(0))
                    continue;
                mass_sum        += m;
                weighted_pos_sum = weighted_pos_sum + (m_get_pos(*p) * m);
            }

            node->total_mass = mass_sum;
            if (mass_sum > T_Scalar(0))
                node->center_of_mass = weighted_pos_sum * (T_Scalar(1) / mass_sum);
        } else if (!node->isLeaf()) {
            T_Vec    weighted_pos_sum{};
            T_Scalar mass_sum = T_Scalar(0);

            for (auto &child : node->children) {
                if (child) {
                    calculateComRecursive(child.get());
                    if (child->total_mass > T_Scalar(0)) {
                        mass_sum        += child->total_mass;
                        weighted_pos_sum = weighted_pos_sum + (child->center_of_mass * child->total_mass);
                    }
                }
            }

            node->total_mass = mass_sum;
            if (mass_sum > T_Scalar(0))
                node->center_of_mass = weighted_pos_sum * (T_Scalar(1) / mass_sum);

        }
        // else: empty node ⇒ mass 0, COM unused.
    }

    // s/d < theta logic
    T_Vec calculateForceRecursive(const Node *node, const T_Particle &p, T_Scalar theta, T_Scalar G, T_Scalar epsilon_sq) const
    {
        if (!node || node->total_mass == T_Scalar(0))
            return {};

        const T_Vec r_vec = node->center_of_mass - m_get_pos(p);

        // Softened distance
        const T_Scalar d_sq = r_vec.lengthSq() + epsilon_sq;
        const T_Scalar d    = std::sqrt(d_sq);

        const bool is_leaf = node->isLeaf();

        // Leaf with exactly this single particle: skip self-interaction.
        if (is_leaf && node->particles.size() == 1 && node->particles[0] == &p)
            return {};

        // Internal node: apply Barnes–Hut opening criterion.
        if (!is_leaf) {
            const T_Scalar s = node->width;
            if ((s / d) >= theta) {
                // Too close: recursively resolve children instead of treating as lump.
                T_Vec total_force{};
                for (const auto &child : node->children)
                    total_force = total_force +
                                  calculateForceRecursive(child.get(), p, theta, G, epsilon_sq);
                return total_force;
            }
            // else: far enough, approximate as single monopole.
        }
        // Leaf node with 1+ particles (or internal node passing opening test)
        // Approximate them as a single body at center_of_mass with total_mass.
        // WE ARE CALCULATING A FORCE !!!!!!!!!!
        const T_Scalar F_mag = G * m_get_mass(p) * node->total_mass;
        return r_vec * (F_mag / (d * d_sq)); // F_mag / d^3
    }
};

} // namespace job::threads

//CHECKPOINT v1.0
