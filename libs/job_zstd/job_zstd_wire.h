#pragma once

#include <cstdint>
#include <string>
#include <istream>
#include <ostream>

#include "jobzstd_export.h"

// Job's own minimal little-endian wire format, used only for the parts of
// the archive stream that qzstd never wrote (V2 directory entries). This is
// NOT QDataStream-compatible and was never meant to be
namespace job::zstd::utils {

constexpr std::size_t kMaxWireStringLength = 65536;

JOBZSTD_EXPORT void writeU8(std::ostream &out, std::uint8_t value);
JOBZSTD_EXPORT void writeU64(std::ostream &out, std::uint64_t value);
JOBZSTD_EXPORT void writeString(std::ostream &out, const std::string &value);

// Each returns false on stream failure or unexpected EOF, leaving `value`
// unspecified. Check the stream, not just the return, if you need to know why.
bool JOBZSTD_EXPORT readU8(std::istream &in, std::uint8_t &value);
bool JOBZSTD_EXPORT readU64(std::istream &in, std::uint64_t &value);
bool JOBZSTD_EXPORT readString(std::istream &in, std::string &value);

} // namespace job::zstd::utils