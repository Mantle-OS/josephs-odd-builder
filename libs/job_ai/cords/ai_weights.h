#pragma once
#include <aligned_allocator.h>
namespace job::ai::cords {

using AiWeights = std::vector<float, core::AlignedAllocator<float, 64>>;

}
