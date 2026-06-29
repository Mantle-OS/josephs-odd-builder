#pragma once
#include <memory>
#include <string>
#include <cstdint>
#include <vector>
#include <unordered_map>

#include <ggml.h>
#include <endian_utils.h>

namespace job::ggml {

/**
 * Represents the individual metadata for one tensor within the GGUF file.
 */
struct JobGgufTensorEntry {
    std::string           name;
    ggml_type             type;
    uint64_t              offset;
    std::vector<uint64_t> dims;

    // Helper to get total number of elements
    uint64_t elements() const {
        uint64_t n = 1;
        for (auto d : dims) n *= d;
        return n;
    }
};

class JobGgufAbstractTensorInfo {
public:
    using Ptr = std::shared_ptr<JobGgufAbstractTensorInfo>;
    virtual ~JobGgufAbstractTensorInfo() = default;

    /**
     * Parses the Tensor Info section.
     * ptr is advanced through the memory as we read.
     */
    bool load(const uint8_t *&ptr, uint64_t count) {
        m_count = static_cast<size_t>(count);
        m_tensors.reserve(m_count);

        for (uint64_t i = 0; i < count; ++i) {
            JobGgufTensorEntry entry;

            // 1. Read Name (GGUF String)
            uint64_t name_len = core::fromLE64(*reinterpret_cast<const uint64_t*>(ptr));
            ptr += 8;
            entry.name.assign(reinterpret_cast<const char*>(ptr), name_len);
            ptr += name_len;

            // 2. Read Number of Dimensions
            uint32_t n_dims = core::fromLE32(*reinterpret_cast<const uint32_t*>(ptr));
            ptr += 4;

            // 3. Read Dimensions (array of uint64)
            entry.dims.reserve(n_dims);
            for (uint32_t d = 0; d < n_dims; ++d) {
                entry.dims.push_back(core::fromLE64(*reinterpret_cast<const uint64_t*>(ptr)));
                ptr += 8;
            }

            // 4. Read Type (ggml_type)
            uint32_t type_val = core::fromLE32(*reinterpret_cast<const uint32_t*>(ptr));
            entry.type = static_cast<ggml_type>(type_val);
            ptr += 4;

            // 5. Read Offset (relative to the start of the data binary blob)
            entry.offset = core::fromLE64(*reinterpret_cast<const uint64_t*>(ptr));
            ptr += 8;

            m_tensors.push_back(std::move(entry));
        }

        return true;
    }

    // Accessors
    const std::vector<JobGgufTensorEntry> &tensors() const
    {
        return m_tensors;
    }
    size_t count() const
    {
        return m_count;
    }

    const JobGgufTensorEntry *find(const std::string &name) const
    {
        for (const auto& t : m_tensors) {
            if (t.name == name) return &t;
        }
        return nullptr;
    }

private:
    size_t m_count{0};
    std::vector<JobGgufTensorEntry> m_tensors;
};

} // namespace job::ggml