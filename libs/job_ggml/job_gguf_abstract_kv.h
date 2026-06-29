#pragma once

#include <memory>
#include <optional>
#include <string>
#include <cstdint>
#include <unordered_map>

#include <ggml.h>
#include <gguf.h>

#include <endian_utils.h>
#include "job_gguf_val.h"

namespace job::ggml {

class JobGgufAbstractKV {
public:
    using Ptr  = std::shared_ptr<JobGgufAbstractKV>;
    using UPtr = std::unique_ptr<JobGgufAbstractKV>;

    virtual ~JobGgufAbstractKV() = default;

    bool load(const uint8_t *&ptr, uint64_t count)
    {
        if(!m_entries.empty())
            m_entries.clear();

        m_entries.reserve(count);

        for (uint64_t i = 0; i < count; ++i) {
            // 1. Read Key (GGUF strings are: length [uint64] + data [char*])
            uint64_t key_len = core::fromLE64(*reinterpret_cast<const uint64_t*>(ptr));
            ptr += 8;

            std::string key(reinterpret_cast<const char*>(ptr), key_len);
            ptr += key_len;

            // 2. Read Value Type
            gguf_type t = static_cast<gguf_type>(core::fromLE32(*reinterpret_cast<const uint32_t*>(ptr)));
            ptr += 4;

            // 3. Store and Advance
            JobGgufValue val{t, ptr, 0};
            if (!advanceAndPopulate(val, ptr))
                return false;

            m_entries[key] = val;
        }
        return true;
    }

    std::size_t count() const { return m_entries.size(); }

    std::optional<JobGgufValue> find(const std::string &key) const
    {
        auto it = m_entries.find(key);
        if (it != m_entries.end()) return it->second;
        return std::nullopt;
    }

private:
    bool advanceAndPopulate(JobGgufValue &val, const uint8_t *&ptr)
    {
        switch (val.type) {
        case GGUF_TYPE_UINT8:
        case GGUF_TYPE_INT8:
        case GGUF_TYPE_BOOL:
            ptr += 1;
            break;
        case GGUF_TYPE_UINT16:
        case GGUF_TYPE_INT16:
            ptr += 2;
            break;
        case GGUF_TYPE_UINT32:
        case GGUF_TYPE_INT32:
        case GGUF_TYPE_FLOAT32:
            ptr += 4;
            break;
        case GGUF_TYPE_UINT64:
        case GGUF_TYPE_INT64:
        case GGUF_TYPE_FLOAT64:
            ptr += 8;
            break;
        case GGUF_TYPE_STRING: {
            val.len = core::fromLE64(*reinterpret_cast<const uint64_t*>(ptr));
            ptr += 8;
            val.ptr = ptr; // Update ptr to the actual string start
            ptr += val.len;
            break;
        }
        case GGUF_TYPE_ARRAY: {
            // Arrays are: type [u32] + count [u64] + data
            // This is recursive/variable, typically we'd skip or handle specifically
            // For now, let's just grab the array type and count
            [[maybe_unused]]gguf_type array_type = static_cast<gguf_type>(core::fromLE32(*reinterpret_cast<const uint32_t*>(ptr)));
            ptr += 4;

            val.len = core::fromLE64(*reinterpret_cast<const uint64_t*>(ptr));
            ptr += 8;
            val.ptr = ptr;
            // Move ptr past the array data (this requires knowing size of array_type)
            // ptr += val.len * sizeOf(array_type);
            return false; // Array logic is architecture specific usually
        }
        default:
            return false;
        }

        return true;
    }

    std::unordered_map<std::string, JobGgufValue> m_entries;
};
} // namespace job::ggml
