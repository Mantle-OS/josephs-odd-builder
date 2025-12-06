#pragma once
#include <cstdint>
#include <functional> // for std::hash

namespace job::ai::kv {

struct CacheKey {
    uint32_t layerId;
    uint32_t head;

    // Allow comparison for map keys
    bool operator==(const CacheKey& other) const {
        return layerId == other.layerId && head == other.head;
    }
};

} // namespace job::ai::kv

// Inject hash for std::unordered_map support
template<>
struct std::hash<job::ai::kv::CacheKey> {
    std::size_t operator()(const job::ai::kv::CacheKey& k) const {
        // Simple bit shift hash combine
        return (std::hash<uint32_t>()(k.layerId) ^ (std::hash<uint32_t>()(k.head) << 1));
    }
};
