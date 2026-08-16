#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include <FLAC/stream_encoder.h>

#include "codecs/audio_codec_params.h"
#include "codecs/audio_encoder.h"
#include "codecs/audio_packet.h"
#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT FlacAudioEncoder final : public AudioEncoder {
public:
    FlacAudioEncoder();
    ~FlacAudioEncoder() override;

    FlacAudioEncoder(const FlacAudioEncoder&) = delete;
    FlacAudioEncoder& operator=(const FlacAudioEncoder&) = delete;
    FlacAudioEncoder(FlacAudioEncoder&&) noexcept;
    FlacAudioEncoder& operator=(FlacAudioEncoder&&) noexcept;

    // Initializes the FLAC encoder instance
    [[nodiscard]] bool init(const AudioCodecConfig& config) override;

    // Encodes one block of interleaved float PCM frames into a compressed AudioPacket
    [[nodiscard]] bool encode(std::span<const float> pcmIn, AudioPacket& packetOut) override;

    // Flushes all remaining internal encoder delay samples and finalizes the stream
    [[nodiscard]] bool flush(std::vector<AudioPacket>& flushedPackets) override;

    [[nodiscard]] const AudioCodecConfig& config() const noexcept override { return m_config; }
    [[nodiscard]] bool isInitialized() const noexcept override { return m_encoder != nullptr; }

    void reset() noexcept;

private:
    struct FlacDeleter {
        void operator()(::FLAC__StreamEncoder* ptr) const noexcept;
    };

    // Callback helpers for libFLAC
    friend struct FlacEncoderCallbacks;

    std::unique_ptr<::FLAC__StreamEncoder, FlacDeleter> m_encoder;
    AudioCodecConfig m_config;

    std::vector<std::uint8_t> m_pendingOutput;
    std::vector<std::int32_t> m_pcmIntBuffer;
    unsigned int m_bitsPerSample{16};
    std::int64_t m_currentPts{0};
};

} // namespace job::sound