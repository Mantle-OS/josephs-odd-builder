#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include "audio_codec_params.h"
#include "audio_packet.h"
#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT AudioDecoder {
public:
    using Ptr  = std::shared_ptr<AudioDecoder>;
    using UPtr = std::unique_ptr<AudioDecoder>;

    virtual ~AudioDecoder() = default;

    // Initializes the decoder instance
    [[nodiscard]] virtual bool init(const AudioCodecConfig& config) = 0;

    // Decodes a raw compressed packet into interleaved PCM float samples [-1.0f, 1.0f]
    [[nodiscard]] virtual bool decode(std::span<const std::uint8_t> packetIn, std::vector<float>& pcmOut) = 0;

    // Helper overload accepting an AudioPacket directly
    [[nodiscard]] virtual bool decode(const AudioPacket& packet, std::vector<float>& pcmOut) {
        return decode(packet.span(), pcmOut);
    }

    // Flushes remaining decoded frames at EOF
    [[nodiscard]] virtual bool flush(std::vector<float>& pcmOut) {
        (void)pcmOut;
        return true;
    }

    [[nodiscard]] virtual const AudioCodecConfig& config() const noexcept = 0;
    [[nodiscard]] virtual bool isInitialized() const noexcept = 0;
};

} // namespace job::sound