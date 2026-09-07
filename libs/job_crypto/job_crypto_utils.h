#pragma once

#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <sodium.h>

namespace job::crypto::utils {

/////////////////////////////////////////
// BASE64 -> BIN
/////////////////////////////////////////

[[nodiscard]] inline static bool base64ToBin(std::vector<unsigned char> &out, std::string_view b64) noexcept
{
    if (b64.empty()) {
        out.clear();
        return true;
    }

    std::size_t const maxLen = (b64.size() * 3) / 4 + 2;
    out.resize(maxLen);

    std::size_t binLen = 0;

    if (sodium_base642bin(
            out.data(),
            out.size(),
            b64.data(),
            b64.size(),
            nullptr,
            &binLen,
            nullptr,
            sodium_base64_VARIANT_ORIGINAL) != 0) {
        out.clear();
        return false;
    }

    out.resize(binLen);
    return true;
}

[[nodiscard]] inline static bool base64ToBin(std::vector<unsigned char> &out, const char *b64) noexcept
{
    if (!b64) {
        out.clear();
        return false;
    }

    return base64ToBin(out, std::string_view{b64});
}

[[nodiscard]] inline static bool base64ToBin(std::vector<unsigned char> &out, const std::string &b64) noexcept
{
    return base64ToBin(out, std::string_view{b64});
}

/////////////////////////////////////////
// BIN -> BASE64
/////////////////////////////////////////

[[nodiscard]] inline static std::string toBase64(const void *data, std::size_t size) noexcept
{
    if (!data && size > 0)
        return {};

    if (size == 0)
        return {};

    std::size_t const maxB64Len = sodium_base64_ENCODED_LEN(
        size,
        sodium_base64_VARIANT_ORIGINAL
        );

    std::string b64Str(maxB64Len, '\0');

    char *const result = sodium_bin2base64(
        b64Str.data(),
        b64Str.size(),
        static_cast<const unsigned char *>(data),
        size,
        sodium_base64_VARIANT_ORIGINAL
        );

    if (!result)
        return {};

    b64Str.resize(std::strlen(b64Str.c_str()));
    return b64Str;
}

[[nodiscard]] inline static std::string toBase64(std::string_view data) noexcept
{
    return toBase64(data.data(), data.size());
}

[[nodiscard]] inline static std::string toBase64(const char *data) noexcept
{
    if (!data)
        return {};

    return toBase64(std::string_view{data});
}

[[nodiscard]] inline static std::string toBase64(const std::string &data) noexcept
{
    return toBase64(std::string_view{data});
}

[[nodiscard]] inline static std::string toBase64(const std::vector<unsigned char> &data) noexcept
{
    return toBase64(data.data(), data.size());
}

template<typename T>
    requires requires(const T &value) {
        value.data();
        value.size();
    } && (sizeof(std::remove_cv_t<std::remove_pointer_t<decltype(std::declval<const T &>().data())>>) == 1)
[[nodiscard]] inline static std::string toBase64(const T &data) noexcept
{
    return toBase64(data.data(), data.size());
}

/////////////////////////////////////////
// HEX -> BIN
/////////////////////////////////////////

[[nodiscard]] inline static bool hexToBin(std::vector<unsigned char> &out, std::string_view hex) noexcept
{
    if (hex.empty()) {
        out.clear();
        return true;
    }

    out.resize(hex.size() / 2 + 1);

    std::size_t binLen = 0;

    if (sodium_hex2bin(
            out.data(),
            out.size(),
            hex.data(),
            hex.size(),
            nullptr,
            &binLen,
            nullptr) != 0) {
        out.clear();
        return false;
    }

    out.resize(binLen);
    return true;
}

[[nodiscard]] inline static bool hexToBin(std::vector<unsigned char> &out, const char *hex) noexcept
{
    if (!hex) {
        out.clear();
        return false;
    }

    return hexToBin(out, std::string_view{hex});
}

[[nodiscard]] inline static bool hexToBin(std::vector<unsigned char> &out, const std::string &hex) noexcept
{
    return hexToBin(out, std::string_view{hex});
}

[[nodiscard]] inline static bool hexToBin(std::string &out, std::string_view hex) noexcept
{
    if (hex.empty()) {
        out.clear();
        return true;
    }

    out.resize(hex.size() / 2 + 1);

    std::size_t binLen = 0;

    if (sodium_hex2bin(
            reinterpret_cast<unsigned char *>(out.data()),
            out.size(),
            hex.data(),
            hex.size(),
            nullptr,
            &binLen,
            nullptr) != 0) {
        out.clear();
        return false;
    }

    out.resize(binLen);
    return true;
}

[[nodiscard]] inline static bool hexToBin(std::string &out, const char *hex) noexcept
{
    if (!hex) {
        out.clear();
        return false;
    }

    return hexToBin(out, std::string_view{hex});
}

[[nodiscard]] inline static bool hexToBin(std::string &out, const std::string &hex) noexcept
{
    return hexToBin(out, std::string_view{hex});
}

/////////////////////////////////////////
// BIN -> HEX
/////////////////////////////////////////

[[nodiscard]] inline static std::string toHex(const void *data, std::size_t size) noexcept
{
    if (!data && size > 0)
        return {};

    if (size == 0)
        return {};

    std::string result(size * 2 + 1, '\0');

    sodium_bin2hex(
        result.data(),
        result.size(),
        static_cast<const unsigned char *>(data),
        size
        );

    result.resize(size * 2);
    return result;
}

[[nodiscard]] inline static std::string toHex(std::string_view data) noexcept
{
    return toHex(data.data(), data.size());
}

[[nodiscard]] inline static std::string toHex(const char *data) noexcept
{
    if (!data)
        return {};

    return toHex(std::string_view{data});
}

[[nodiscard]] inline static std::string toHex(const std::string &data) noexcept
{
    return toHex(std::string_view{data});
}

[[nodiscard]] inline static std::string toHex(const std::vector<unsigned char> &data) noexcept
{
    return toHex(data.data(), data.size());
}

template<typename T>
    requires requires(const T &value) {
        value.data();
        value.size();
    } && (sizeof(std::remove_cv_t<std::remove_pointer_t<decltype(std::declval<const T &>().data())>>) == 1)
[[nodiscard]] inline static std::string toHex(const T &data) noexcept
{
    return toHex(data.data(), data.size());
}

} // namespace job::crypto::utils