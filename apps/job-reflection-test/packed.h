#pragma once

#include <cstdint>
#include <meta>
#include <contracts>
#include <iostream>

enum class LayerType : uint8_t {
    Input       = 0,    // Input.
    Dense       = 1,    // Standard Linear
    SparseMoE   = 2,    // Mixture of Experts
    Attention   = 3,    // Self/Cross Attention
    Embedding   = 4,    // Token -> Vector
    LayerNorm   = 5,    // Normalization
    RMSNorm     = 6,    // LLaMA style norm
    Residual    = 7,    // Add
    LinearLoRA  = 8,    // Add
    Output      = 9,    // Logits
    Abstract     = 254  // Unknown slash bad layer type
};


enum class ActivationType : uint8_t {
    Identity    = 0,   // f(x) = x
    Sigmoid     = 1,   // squashes to (0, 1)
    Tanh        = 2,   // squashes to (−1, 1)
    HardTanh    = 3,   // piecewise linear cheap approximations
    ///
    ReLU        = 4,    // max(0, x)
    LeakyReLU   = 5,    // x for x > 0, αx for x ≤ 0 (α small, like 0.01)
    PReLU       = 6,    // like Leaky, but α is learned.
    RReLU       = 7,    // α random in training.
    ELU         = 8,    // exponential on the negative side
    SELU        = 9,    // scaled variant designed for self-normalizing networks.
    ///
    GELU        = 10,   // gaussian error linear unit
    AproxGELU   = 11,   // Tanh-based approximation of GELU (faster)
    Swish       = 12,   // x * sigmoid(βx) (β sometimes = 1)
    HSwish      = 13,   // piecewise linear approximation to Swish.
    Mish        = 14,   // x * tanh(softplus(x))
    HMish       = 15,   // piecewise linears mimicking Mish/Swish.
    Softplus    = 16,   // log(1 + exp(x)) (a smooth ReLU).
    ///
    Maxout      = 17,   // f(x) = max(x₁, x₂, …, x_k) across groups of channels.
    //
    GDN         = 18    // generalized divisive normalization
};

struct Packed {
    LayerType      type{LayerType::Dense};
    ActivationType activation{ActivationType::Identity};
    std::uint8_t   _pad[2]{};
    std::uint32_t  inputs{0};
    std::uint32_t  outputs{0};
    std::uint32_t  weightOffset{0};
    std::uint32_t  weightCount{0};
    std::uint32_t  biasOffset{0};
    std::uint32_t  biasCount{0};
    std::uint32_t  auxiliaryData{0};

    static consteval auto reflectedMembers()
    {
        constexpr auto ctx = std::meta::access_context::unchecked();
        return std::define_static_array(std::meta::nonstatic_data_members_of(^^Packed, ctx));
    }

    static consteval std::size_t reflectedMemberCount()
    {
        return reflectedMembers().size();
    }
};

static_assert(sizeof(Packed) == 32);
static_assert(Packed::reflectedMemberCount() == 10);

struct Serializer
{
    static bool save(const Packed &g, const std::string &filename)
    {
        std::ofstream out(filename, std::ios::binary);
        if (!out)
            return false;

        out.write(reinterpret_cast<const char *>(&g), sizeof(Packed));
        return out.good();
    }

    static Packed load(const std::string &filename)
    {
        Packed g{};

        std::ifstream in(filename, std::ios::binary);
        if (!in)
            return {};

        in.read(reinterpret_cast<char *>(&g), sizeof(Packed));
        if (!in || in.gcount() != sizeof(Packed))
            return {};

        return g;
    }
};

template <typename T>
void dumpPacked(const T &obj)
{
    template for (constexpr auto member : T::reflectedMembers()) {
        std::cout << std::meta::identifier_of(member) << ": ";

        using V = std::remove_cvref_t<decltype(obj.[:member:])>;

        if constexpr (std::is_enum_v<V>) {
            using U = std::underlying_type_t<V>;
            std::cout << static_cast<std::uint64_t>(static_cast<U>(obj.[:member:]));
        } else if constexpr (std::is_array_v<V>) {
            std::cout << '[';
            bool first = true;
            for (const auto &value : obj.[:member:]) {
                if (!first)
                    std::cout << ", ";
                std::cout << static_cast<std::uint64_t>(value);
                first = false;
            }
            std::cout << ']';
        } else {
            std::cout << obj.[:member:];
        }

        std::cout << '\n';
    }
}