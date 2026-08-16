#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "jobsound_export.h"

namespace job::sound {

enum class AudioCodecType : std::uint8_t {
    Unknown = 0,
    Pcm,
    Opus,
    Flac,
    Vorbis,
    WavPack
};

enum class SampleFormat : std::uint8_t {
    Unknown = 0,
    Int16,
    Int24,
    Int32,
    Float32
};

struct JOBSOUND_EXPORT AudioCodecConfig {
    AudioCodecType codecType{AudioCodecType::Unknown};
    SampleFormat sampleFormat{SampleFormat::Float32};
    unsigned int sampleRate{48000};
    unsigned int channels{1};
    unsigned int bitrate{64000};         // Target bitrate in bps (for lossy codecs like Opus/Vorbis)
    std::size_t frameSize{960};          // Samples per channel per codec frame (e.g. 20ms @ 48kHz = 960)
    int compressionLevel{5};             // Codec compression effort (e.g. 0-10 for Opus/FLAC)

    [[nodiscard]] bool isValid() const noexcept
    {
        return sampleRate > 0 && channels > 0 && frameSize > 0;
    }
};

} // namespace job::sound