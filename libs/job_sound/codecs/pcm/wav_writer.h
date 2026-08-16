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

class JOBSOUND_EXPORT WavWriter {
public:
    WavWriter() = default;
    ~WavWriter();

    WavWriter(const WavWriter&) = delete;
    WavWriter& operator=(const WavWriter&) = delete;
    WavWriter(WavWriter&&) noexcept;
    WavWriter& operator=(WavWriter&&) noexcept;

    // Opens a file on disk for streaming WAV output
    [[nodiscard]] bool open(const std::string& filepath, const AudioCodecConfig& config);

    // Opens an in-memory stream buffer for writing
    [[nodiscard]] bool open(std::vector<std::uint8_t>& targetBuffer, const AudioCodecConfig& config);

    // Finalizes RIFF headers with total written sizes and closes the stream
    void close() noexcept;

    [[nodiscard]] bool isOpen() const noexcept { return m_isOpen; }
    [[nodiscard]] const AudioCodecConfig& config() const noexcept { return m_config; }
    [[nodiscard]] std::size_t writtenFrames() const noexcept { return m_writtenFrames; }

    // Writes a chunk of interleaved float PCM frames [-1.0f, 1.0f].
    // Returns the number of complete frames written.
    std::size_t writeFrames(std::span<const float> pcmIn);

    // Helper static method for one-shot file writing
    static bool writeOneShot(const std::string& filepath,
                             std::span<const float> samples,
                             const AudioCodecConfig& config);

private:
    bool writeHeaders();
    bool updateHeaders();
    bool writeRawBytes(const std::uint8_t* data, std::size_t byteCount);

private:
    AudioCodecConfig m_config;
    std::size_t m_writtenFrames{0};
    std::size_t m_dataBytesWritten{0};
    std::uint16_t m_audioFormat{1}; // 1 = PCM, 3 = IEEE Float
    std::uint16_t m_bitsPerSample{16};

    std::ofstream m_fileStream;
    std::vector<std::uint8_t>* m_memoryTarget{nullptr};
    bool m_toMemory{false};
    bool m_isOpen{false};
};

} // namespace job::sound