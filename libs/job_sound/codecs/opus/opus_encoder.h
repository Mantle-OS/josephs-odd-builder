#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include "codecs/audio_codec_params.h"
#include "codecs/audio_encoder.h"
#include "codecs/audio_packet.h"
#include "jobsound_export.h"

// Forward declaration of Opus internal encoder state
struct OpusEncoder;

namespace job::sound {

class JOBSOUND_EXPORT OpusAudioEncoder final : public AudioEncoder {
public:
    OpusAudioEncoder();
    ~OpusAudioEncoder() override;

    OpusAudioEncoder(const OpusAudioEncoder&) = delete;
    OpusAudioEncoder& operator=(const OpusAudioEncoder&) = delete;
    OpusAudioEncoder(OpusAudioEncoder&&) noexcept;
    OpusAudioEncoder& operator=(OpusAudioEncoder&&) noexcept;

    // Initializes the Opus encoder instance (sampleRate: 8k, 12k, 16k, 24k, 48k; channels: 1 or 2)
    [[nodiscard]] bool init(const AudioCodecConfig& config) override;

    // Encodes one block of interleaved float PCM frames into a compressed AudioPacket
    [[nodiscard]] bool encode(std::span<const float> pcmIn, AudioPacket& packetOut) override;

    // Flush remaining internal delay line samples
    [[nodiscard]] bool flush(std::vector<AudioPacket>& flushedPackets) override;

    [[nodiscard]] const AudioCodecConfig& config() const noexcept override { return m_config; }
    [[nodiscard]] bool isInitialized() const noexcept override { return m_encoder != nullptr; }

    // Dynamic runtime property updates
    bool setBitrate(unsigned int bitrateBps);
    bool setComplexity(int complexity); // 0 (fastest) - 10 (highest quality)
    bool setVbr(bool enabled);
    void reset() noexcept;

private:
    [[nodiscard]] static bool isValidOpusFrameSize(std::size_t frameSize, unsigned int sampleRate) noexcept;

private:
    struct OpusDeleter {
        void operator()(::OpusEncoder* ptr) const noexcept;
    };

    std::unique_ptr<::OpusEncoder, OpusDeleter> m_encoder;
    AudioCodecConfig m_config;
    std::int64_t m_currentPts{0};
};

} // namespace job::sound