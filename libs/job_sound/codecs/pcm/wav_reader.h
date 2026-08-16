#pragma once

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <span>
#include <string>
#include <vector>

#include "codecs/audio_codec_params.h"
#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT WavReader {
public:
    WavReader() = default;
    ~WavReader();

    WavReader(const WavReader&) = delete;
    WavReader& operator=(const WavReader&) = delete;
    WavReader(WavReader&&) noexcept;
    WavReader& operator=(WavReader&&) noexcept;

    // Opens a WAV file from disk and parses its header metadata
    [[nodiscard]] bool open(const std::string &filepath);

    // Opens a WAV file from an in-memory byte buffer
    [[nodiscard]] bool open(std::span<const std::uint8_t> memoryBuffer);

    void close() noexcept;

    [[nodiscard]] bool isOpen() const noexcept { return m_isOpen; }
    [[nodiscard]] const AudioCodecConfig& config() const noexcept { return m_config; }
    [[nodiscard]] std::size_t totalFrames() const noexcept { return m_totalFrames; }
    [[nodiscard]] std::size_t remainingFrames() const noexcept { return m_totalFrames - m_framesRead; }

    // Reads up to maxFrames into interleaved float buffer [-1.0f, 1.0f].
    // Returns the actual number of sample frames read (frames = samples / channels).
    [[nodiscard]] std::size_t readFrames(std::vector<float>& pcmOut, std::size_t maxFrames);

    // Reads the entire remaining audio payload in one shot
    [[nodiscard]] bool readAll(std::vector<float>& pcmOut);

    // Seeks to an absolute sample frame index
    [[nodiscard]] bool seek(std::size_t frameIndex);

private:
    bool parseHeader();
    std::size_t readRawBytes(std::uint8_t* dest, std::size_t byteCount);

private:
    AudioCodecConfig m_config;
    std::size_t m_totalFrames{0};
    std::size_t m_framesRead{0};
    std::size_t m_dataChunkOffset{0};
    std::size_t m_dataChunkSize{0};
    std::uint16_t m_audioFormat{0}; // 1 = PCM, 3 = IEEE Float
    std::uint16_t m_bitsPerSample{0};

    std::ifstream m_fileStream;
    std::span<const std::uint8_t> m_memSpan;
    std::size_t m_memOffset{0};
    bool m_fromMemory{false};
    bool m_isOpen{false};
};

} // namespace job::sound