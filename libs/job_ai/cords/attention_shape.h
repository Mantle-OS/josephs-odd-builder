#pragma once
#include <cstdint>
namespace job::ai::cords {
struct AttentionShape {
    uint32_t batch;
    uint32_t seq;
    uint32_t dim;
    uint32_t numHeads;
};
}
