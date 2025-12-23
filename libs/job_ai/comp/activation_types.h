#pragma once
#include <cstdint>
namespace job::ai::comp {
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
} // namespace job::ai::comp
