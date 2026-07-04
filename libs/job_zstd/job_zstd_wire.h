#pragma once

#include <cstdint>
#include <string>
#include <istream>
#include <ostream>

// Job's own minimal little-endian wire format, used only for the parts of
// the archive stream that qzstd never wrote (V2 directory entries). This is
// NOT QDataStream-compatible and was never meant to be -- the legacy
// QDataStream decode path lives entirely inside JobZstdDecompressor, kept
// separate on purpose so the two formats can't bleed into each other.
namespace job::zstd::utils {

constexpr std::size_t kMaxWireStringLength = 65536;

void writeU8(std::ostream &out, std::uint8_t value);
void writeU64(std::ostream &out, std::uint64_t value);
void writeString(std::ostream &out, const std::string &value);

// Each returns false on stream failure or unexpected EOF, leaving `value`
// unspecified. Check the stream, not just the return, if you need to know why.
bool readU8(std::istream &in, std::uint8_t &value);
bool readU64(std::istream &in, std::uint64_t &value);
bool readString(std::istream &in, std::string &value);

} // namespace job::zstd::utils