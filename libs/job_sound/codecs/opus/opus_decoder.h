#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include "codecs/audio_codec_params.h"
#include "codecs/audio_decoder.h"
#include "jobsound_export.h"

// Forward declaration of Opus internal state
struct OpusDecoder;

namespace job::sound {

class JOBSOUND_EXPORT OpusAudioDecoder final : public AudioDecoder {
public:
    OpusAudioDecoder();
    ~OpusAudioDecoder() override;

    OpusAudioDecoder(const OpusAudioDecoder&) = delete;
    OpusAudioDecoder& operator=(const OpusAudioDecoder&) = delete;
    OpusAudioDecoder(OpusAudioDecoder&&) noexcept;
    OpusAudioDecoder& operator=(OpusAudioDecoder&&) noexcept;

    // Initializes the Opus decoder (sampleRate must be 8000, 12000, 16000, 24000, or 48000; channels 1 or 2)
    [[nodiscard]] bool init(const AudioCodecConfig& config) override;

    // Decodes one Opus payload packet into interleaved float PCM.
    // Passing an empty packet triggers Opus Packet Loss Concealment (PLC).
    [[nodiscard]] bool decode(std::span<const std::uint8_t> packetIn, std::vector<float>& pcmOut) override;

    // Overload allowing explicit control over Packet Loss Concealment (PLC)
    [[nodiscard]] bool decodeFrame(std::span<const std::uint8_t> packetIn, std::vector<float>& pcmOut, bool decodeFec = false);

    [[nodiscard]] const AudioCodecConfig& config() const noexcept override { return m_config; }
    [[nodiscard]] bool isInitialized() const noexcept override { return m_decoder != nullptr; }

    void reset() noexcept;

private:
    struct OpusDeleter {
        void operator()(::OpusDecoder* ptr) const noexcept;
    };

    std::unique_ptr<::OpusDecoder, OpusDeleter> m_decoder;
    AudioCodecConfig m_config;
};

} // namespace job::sound