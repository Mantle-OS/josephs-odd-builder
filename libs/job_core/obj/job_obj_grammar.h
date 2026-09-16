#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace job::core {

// ----------------------------------------
// Basic types.
// Do not use these lazily outside the object grammar.  They may conflict conceptually with domain-specific aliases such as job::simd types.
// careful joseph lol .....
// ----------------------------------------
using i8            = std::int8_t;
using i16           = std::int16_t;
using i32           = std::int32_t;
using i64           = std::int64_t;

using u8            = std::uint8_t;
using u16           = std::uint16_t;
using u32           = std::uint32_t;
using u64           = std::uint64_t;

using usize         = std::size_t;

using byte          = std::byte;
using bytes         = std::vector<byte>;

using str           = std::string;
using str_view		= std::string_view;

template <typename T>
using opt		= std::optional<T>;


// ----------------------------------------
// Pointer
// ----------------------------------------
template <typename T>
using Ptr		= std::shared_ptr<T>;

template <typename T>
using WPtr		= std::weak_ptr<T>;

template <typename T>
using UPtr		= std::unique_ptr<T>;


// ----------------------------------------
// Containers
// ----------------------------------------
template <typename T>
using vec		= std::vector<T>;

template <typename T>
using list		= std::vector<T>;

template <typename T, std::size_t N>
using array		= std::array<T, N>;

template <typename T>
using linklist	= std::list<T>;

template <typename T>
using set		= std::set<T>;

template <typename T>
using uset		= std::unordered_set<T>;

template <typename Key, typename Value>
using map		= std::map<Key, Value>;

template <typename Key, typename Value>
using umap		= std::unordered_map<Key, Value>;

// ----------------------------------------
// Internal naming Do NOT let this turn into a massive list .....
// ----------------------------------------
using pub_key		= std::array<byte, 32>;
using pri_key		= std::array<byte, 64>;
using hash          = std::array<byte, 32>;
using signature		= std::array<byte, 64>;
using salt          = std::array<byte, 16>;

} // namespace job::core