#pragma once
#include <cstring>
#include <string>
#include <vector>
#include <sodium.h>
namespace job::crypto::utils {

[[nodiscard]] inline static bool base64ToBin(std::vector<unsigned char> &out, const std::string &b64) noexcept
{
    std::size_t const maxLen = (b64.size() * 3) / 4 + 2;
    out.resize(maxLen);
    std::size_t binLen = 0;
    if (sodium_base642bin(out.data(), out.size(), b64.c_str(), b64.size(),
                          nullptr, &binLen, nullptr, sodium_base64_VARIANT_ORIGINAL) != 0)
        return false;
    out.resize(binLen);
    return true;
}


[[nodiscard]] inline static std::string toBase64(const std::vector<unsigned char> &bytes) noexcept
{
    if (bytes.empty()) {
        return {};
    }

    std::size_t const maxB64Len = sodium_base64_ENCODED_LEN(bytes.size(),
                                                            sodium_base64_VARIANT_ORIGINAL
                                                            );
    std::string b64Str(maxB64Len, '\0');

    char* const result = sodium_bin2base64(
        b64Str.data(), b64Str.size(),
        bytes.data(), bytes.size(),
        sodium_base64_VARIANT_ORIGINAL
        );

    if (!result) {
        return {};
    }

    b64Str.resize(std::strlen(b64Str.c_str()));
    return b64Str;
}

} // namespace job::crypto::utils
