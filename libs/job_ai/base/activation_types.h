#pragma once
#include <cstdint>
namespace job::ai::base {
enum class ActivationType : uint8_t {
    Identity = 0,   // f(x) = x
    Sigmoid,        // squashes to (0, 1)
    Tanh,           // squashes to (−1, 1)
    HardTanh,       // piecewise linear cheap approximations
    ///
    ReLU,           // max(0, x)
    LeakyReLU,      // x for x > 0, αx for x ≤ 0 (α small, like 0.01)
    PReLU,          // like Leaky, but α is learned.
    RReLU,          // α random in training.
    ELU,            // exponential on the negative side
    SELU,           // scaled variant designed for self-normalizing networks.
    ///
    GELU,           // gaussian error linear unit
    AproxGELU,      // cheaper tanh-based lol.
    Swish,          // x * sigmoid(βx) (β sometimes = 1)
    HSwish,         // piecewise linear approximation to Swish.
    Mish,           // x * tanh(softplus(x))
    HMish,          // piecewise linears mimicking Mish/Swish.
    Softplus,       // log(1 + exp(x)) (a smooth ReLU).
    ///
    Maxout,         // f(x) = max(x₁, x₂, …, x_k) across groups of channels.
    ///
    GDN             // generalized divisive normalization
};
}
