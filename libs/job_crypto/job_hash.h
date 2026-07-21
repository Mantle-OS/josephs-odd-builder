#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <sodium/crypto_generichash.h>
#include "jobcrypto_export.h"
namespace job::crypto {

class JOBCRYPTO_EXPORT JobHash {
public:
    JobHash() = default;
    ~JobHash() = default;

    [[nodiscard]] static std::vector<unsigned char> hashBuffer(const std::vector<unsigned char> &data,
                                                               std::size_t hashSize = crypto_generichash_BYTES,
                                                               const unsigned char *key = nullptr,
                                                               std::size_t keylen = 0) noexcept;

    [[nodiscard]] static std::vector<unsigned char> hashFile(const std::string &filePath,
                                                             std::size_t hashSize = crypto_generichash_BYTES,
                                                             const unsigned char *key = nullptr,
                                                             std::size_t keylen = 0) noexcept;

private:
    // Solid 1MB continuous read layout optimized for massive files (GGUFs, textures, layers)
    static constexpr std::size_t kChunkSize = 1024 * 1024;
};

} // namespace job::crypto