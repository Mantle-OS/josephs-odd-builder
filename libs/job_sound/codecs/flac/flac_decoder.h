#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include <FLAC/stream_decoder.h>

#include "codecs/audio_codec_params.h"
#include "codecs/audio_decoder.h"
#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT FlacAudioDecoder final : public AudioDecoder {
public:
    FlacAudioDecoder();
    ~FlacAudioDecoder() override;

    FlacAudioDecoder(const FlacAudioDecoder&) = delete;
    FlacAudioDecoder& operator=(const FlacAudioDecoder&) = delete;
    FlacAudioDecoder(FlacAudioDecoder&&) noexcept;
    FlacAudioDecoder& operator=(FlacAudioDecoder&&) noexcept;

    // Initializes the FLAC decoder
    [[nodiscard]] bool init(const AudioCodecConfig& config) override;

    // Decodes a FLAC byte packet or chunk into interleaved float PCM
    [[nodiscard]] bool decode(std::span<const std::uint8_t> packetIn, std::vector<float>& pcmOut) override;

    // Flushes remaining decoded samples
    [[nodiscard]] bool flush(std::vector<float>& pcmOut) override;

    [[nodiscard]] const AudioCodecConfig& config() const noexcept override { return m_config; }
    [[nodiscard]] bool isInitialized() const noexcept override { return m_decoder != nullptr; }

    void reset() noexcept;

private:
    struct FlacDeleter {
        void operator()(::FLAC__StreamDecoder* ptr) const noexcept;
    };

    // Callback helpers for libFLAC
    friend struct FlacDecoderCallbacks;

    std::unique_ptr<::FLAC__StreamDecoder, FlacDeleter> m_decoder;
    AudioCodecConfig m_config;

    std::span<const std::uint8_t> m_currentInputSpan;
    std::size_t m_inputOffset{0};
    std::vector<float>* m_currentOutputTarget{nullptr};
    bool m_metadataParsed{false};
};

} // namespace job::sound