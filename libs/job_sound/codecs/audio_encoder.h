#pragma once

#include <memory>
#include <span>
#include <vector>

#include "audio_codec_params.h"
#include "audio_packet.h"
#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT AudioEncoder {
public:
    using Ptr  = std::shared_ptr<AudioEncoder>;
    using UPtr = std::unique_ptr<AudioEncoder>;

    virtual ~AudioEncoder() = default;

    // Initializes the encoder instance with target settings
    [[nodiscard]] virtual bool init(const AudioCodecConfig& config) = 0;

    // Encodes one complete block of interleaved PCM float samples [-1.0f, 1.0f]
    [[nodiscard]] virtual bool encode(std::span<const float> pcmIn, AudioPacket& packetOut) = 0;

    // Flushes buffered encoder delay samples (if any) to final packets
    [[nodiscard]] virtual bool flush(std::vector<AudioPacket>& flushedPackets) {
        (void)flushedPackets;
        return true;
    }

    [[nodiscard]] virtual const AudioCodecConfig& config() const noexcept = 0;
    [[nodiscard]] virtual bool isInitialized() const noexcept = 0;
};

} // namespace job::sound