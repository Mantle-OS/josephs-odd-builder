#pragma once
#include <cstdint>
namespace job::ai::cords {
struct AttentionShape {
    uint32_t batch{0};
    uint32_t seq{0};
    uint32_t dim{0};
    uint32_t numHeads{0};
};
}
