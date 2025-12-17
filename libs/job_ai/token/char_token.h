#pragma once

#include <cstdint>
#include <span>

#include <utils/utf8_decoder.h>

#include "itoken.h"
#include "byte_lattice_kernel.h"

namespace job::ai::token {

class CharToken final : public IToken {
public:
    // Bytes -> atoms (normalized UTF-8 bytes).
    [[nodiscard]] std::size_t encode(std::span<const uint8_t> input,
                                     std::span<ByteLattice> output,
                                     float mass = 1.0f) override
    {
        uint8_t tmp[256 * 3] = {};
        std::size_t outAtoms = 0;
        std::size_t i = 0;

        while (i < input.size() && outAtoms < output.size()) {
            const std::size_t inRemain = input.size() - i;
            const std::size_t inChunk  = (inRemain > 256) ? 256 : inRemain;

            const auto inSpan = input.subspan(i, inChunk);
            const auto outByteCap = (output.size() - outAtoms);

            // tmp capacity needs to fits in final output
            const std::size_t tmpCap = std::min<std::size_t>(sizeof(tmp), outByteCap);

            auto result = ansi::utils::Utf8Decoder::normalizeUtf8To(inSpan, std::span<uint8_t>(tmp, tmpCap));
            // you eat to much and are now full
            if (result.bytesWritten == 0)
                break;

            // normalized
            ByteLatticeKernel::batchEncode(std::span<const uint8_t>(tmp, result.bytesWritten),
                                           output.subspan(outAtoms, result.bytesWritten),
                                           mass);

            // consumed/produced
            outAtoms += result.bytesWritten;
            i += result.bytesRead;
        }

        return outAtoms;
    }

    // Atoms -> bytes (decode lattice to bytes, then normalize UTF-8 bytes).
    [[nodiscard]] std::size_t decode(std::span<const ByteLattice> input,
                                     std::span<uint8_t> output) override
    {
        uint8_t tmp[256] = {};
        std::size_t outBytes = 0;
        std::size_t i = 0;

        while (i < input.size() && outBytes < output.size()) {
            const std::size_t inRemain = input.size() - i;

            // Only decode what can fit in 'tmp'.
            // Note: If 'output' is near full,  we might decode 256 lattices but only consume 5 bytes. The next loop will re-decode the rest... whatever.
            // This is intentional. Batch decoding is faster than fine-grained checks
            const std::size_t inChunk = (inRemain > sizeof(tmp)) ? sizeof(tmp) : inRemain;

            // lattice -> bytes
            ByteLatticeKernel::batchDecode(input.subspan(i, inChunk),
                                           std::span<uint8_t>(tmp, inChunk));

            // bytes -> output
            auto result = ansi::utils::Utf8Decoder::normalizeUtf8To(
                std::span<const uint8_t>(tmp, inChunk),
                output.subspan(outBytes)
                );

            if (result.bytesRead == 0 || result.bytesWritten == 0)
                break;

            outBytes += result.bytesWritten;

            // advance lattice cursor
            // !!!! SAFETY !!!  relies on 1 lattice == 1 byte invariant.
            i += result.bytesRead;
        }

        return outBytes;
    }

    void mutate(uint64_t) override
    {
        // Characters don't mutate. The world does.
    }
};

} // namespace job::ai::token
