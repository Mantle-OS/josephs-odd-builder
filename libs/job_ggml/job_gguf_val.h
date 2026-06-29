#pragma once
#include <concepts>
#include <cstdint>
#include <string_view>

#include <gguf.h>
#include <endian_utils.h>
namespace job::ggml {
struct JobGgufValue {
    gguf_type type;
    const uint8_t* ptr;   // Pointer into the mmap'd file
    uint64_t len;         // Number of bytes for strings, or number of elements for arrays

    // Usage: uint32_t val = kv.as<uint32_t>();
    template<std::integral T>
    [[nodiscard]] T as() const
    {
        // In a production build, you'd add a check:
        // if (type != ExpectedGgufType<T>()) throw...
        return job::core::byteswap_internal(*reinterpret_cast<const T*>(ptr));
    }

    // Specialization for strings
    [[nodiscard]] std::string_view as_string() const
    {
        return std::string_view(reinterpret_cast<const char*>(ptr), len);
    }
};
}