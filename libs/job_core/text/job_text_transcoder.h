#pragma once

#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

#include "job_lattice_codec.h"
#include "job_lattice_kernel.h"
#include "job_text_transcode_status.h"

#include "job_unicode_scalar.h"

namespace job::core::text {

class TextTranscoder
{
public:
    using Result = lattice::LatticeKernelResult<TextTranscodeStatus>;

    TextTranscoder() = delete;
    ~TextTranscoder() = delete;

    TextTranscoder(const TextTranscoder &) = delete;
    TextTranscoder &operator=(const TextTranscoder &) = delete;
    TextTranscoder(TextTranscoder &&) = delete;
    TextTranscoder &operator=(TextTranscoder &&) = delete;

    template <lattice::LatticeCodec FromCodec, lattice::LatticeCodec ToCodec>
    [[nodiscard]] static constexpr Result transcode(
        typename FromCodec::UnitInput input,
        typename ToCodec::UnitOutput output) noexcept
    {
        using FromStatus = std::remove_cvref_t<decltype(std::declval<typename FromCodec::Result>().status)>;
        using ToStatus = std::remove_cvref_t<decltype(std::declval<typename ToCodec::Result>().status)>;

        std::size_t inputConsumed = 0;
        std::size_t outputProduced = 0;

        while (inputConsumed < input.size()) {
            std::array<UnicodeScalar, 1> scalar{};

            const auto decodeResult = FromCodec::decode(input.subspan(inputConsumed), scalar);
            if (decodeResult.status == FromStatus::Incomplete)
                return {inputConsumed, outputProduced, TextTranscodeStatus::Incomplete};

            if (decodeResult.status == FromStatus::Invalid)
                return {inputConsumed, outputProduced, TextTranscodeStatus::Invalid};

            const bool scalarDecoded =
                decodeResult.outputProduced == 1 &&
                decodeResult.inputConsumed > 0 &&
                (decodeResult.status == FromStatus::Complete ||
                 decodeResult.status == FromStatus::OutputFull);

            if (!scalarDecoded)
                return {inputConsumed, outputProduced, TextTranscodeStatus::Invalid};

            const auto encodeResult = ToCodec::encode(scalar, output.subspan(outputProduced));
            if (encodeResult.status == ToStatus::OutputFull)
                return {inputConsumed, outputProduced, TextTranscodeStatus::OutputFull};

            if (encodeResult.status != ToStatus::Complete)
                return {inputConsumed, outputProduced, TextTranscodeStatus::Invalid};

            inputConsumed += decodeResult.inputConsumed;
            outputProduced += encodeResult.outputProduced;
        }

        return {inputConsumed, outputProduced, TextTranscodeStatus::Complete};
    }
};

} // namespace job::core::text