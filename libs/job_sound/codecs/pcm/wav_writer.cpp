#include "wav_writer.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <job_logger.h>

namespace job::sound {

namespace {

#pragma pack(push, 1)
struct RiffHeader {
    char riffId[4]{'R', 'I', 'F', 'F'};
    std::uint32_t riffSize{0};
    char waveId[4]{'W', 'A', 'V', 'E'};
};

struct FmtChunk {
    char fmtId[4]{'f', 'm', 't', ' '};
    std::uint32_t fmtSize{16};
    std::uint16_t audioFormat{1};
    std::uint16_t numChannels{1};
    std::uint32_t sampleRate{48000};
    std::uint32_t byteRate{96000};
    std::uint16_t blockAlign{2};
    std::uint16_t bitsPerSample{16};
};

struct DataChunkHeader {
    char dataId[4]{'d', 'a', 't', 'a'};
    std::uint32_t dataSize{0};
};
#pragma pack(pop)

constexpr std::uint16_t kWavFormatPcm   = 1;
constexpr std::uint16_t kWavFormatFloat = 3;

} // namespace

WavWriter::~WavWriter() {
    close();
}

WavWriter::WavWriter(WavWriter&& other) noexcept {
    *this = std::move(other);
}

WavWriter& WavWriter::operator=(WavWriter&& other) noexcept {
    if (this != &other) {
        close();
        m_config            = other.m_config;
        m_writtenFrames     = other.m_writtenFrames;
        m_dataBytesWritten  = other.m_dataBytesWritten;
        m_audioFormat       = other.m_audioFormat;
        m_bitsPerSample     = other.m_bitsPerSample;
        m_fileStream        = std::move(other.m_fileStream);
        m_memoryTarget      = other.m_memoryTarget;
        m_toMemory          = other.m_toMemory;
        m_isOpen            = other.m_isOpen;

        other.m_isOpen = false;
        other.m_memoryTarget = nullptr;
    }
    return *this;
}

bool WavWriter::open(const std::string& filepath, const AudioCodecConfig& config) {
    close();
    if (!config.isValid()) {
        JOB_LOG_WARN("[WavWriter] Invalid audio config specified");
        return false;
    }

    m_toMemory = false;
    m_memoryTarget = nullptr;
    m_config = config;

    m_fileStream.open(filepath, std::ios::binary | std::ios::trunc);
    if (!m_fileStream.is_open()) {
        JOB_LOG_WARN("[WavWriter] Cannot open file for writing: {}", filepath);
        return false;
    }

    if (!writeHeaders()) {
        close();
        return false;
    }

    m_isOpen = true;
    return true;
}

bool WavWriter::open(std::vector<std::uint8_t>& targetBuffer, const AudioCodecConfig& config) {
    close();
    if (!config.isValid()) {
        JOB_LOG_WARN("[WavWriter] Invalid audio config specified");
        return false;
    }

    m_toMemory = true;
    m_memoryTarget = &targetBuffer;
    m_memoryTarget->clear();
    m_config = config;

    if (!writeHeaders()) {
        close();
        return false;
    }

    m_isOpen = true;
    return true;
}

void WavWriter::close() noexcept {
    if (!m_isOpen) return;

    (void)updateHeaders();

    if (m_fileStream.is_open()) {
        m_fileStream.close();
    }

    m_memoryTarget = nullptr;
    m_writtenFrames = 0;
    m_dataBytesWritten = 0;
    m_isOpen = false;
}

bool WavWriter::writeRawBytes(const std::uint8_t* data, std::size_t byteCount) {
    if (m_toMemory) {
        if (!m_memoryTarget) return false;
        m_memoryTarget->insert(m_memoryTarget->end(), data, data + byteCount);
        return true;
    }

    m_fileStream.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(byteCount));
    return m_fileStream.good();
}

bool WavWriter::writeHeaders() {
    switch (m_config.sampleFormat) {
    case SampleFormat::Int16:
        m_audioFormat = kWavFormatPcm;
        m_bitsPerSample = 16;
        break;
    case SampleFormat::Int24:
        m_audioFormat = kWavFormatPcm;
        m_bitsPerSample = 24;
        break;
    case SampleFormat::Int32:
        m_audioFormat = kWavFormatPcm;
        m_bitsPerSample = 32;
        break;
    case SampleFormat::Float32:
    default:
        m_audioFormat = kWavFormatFloat;
        m_bitsPerSample = 32;
        break;
    }

    const std::uint16_t numChannels = static_cast<std::uint16_t>(m_config.channels);
    const std::uint32_t sampleRate  = m_config.sampleRate;
    const std::uint16_t blockAlign  = numChannels * (m_bitsPerSample / 8);
    const std::uint32_t byteRate    = sampleRate * blockAlign;

    RiffHeader riff{};
    FmtChunk fmt{};
    fmt.audioFormat   = m_audioFormat;
    fmt.numChannels   = numChannels;
    fmt.sampleRate    = sampleRate;
    fmt.byteRate      = byteRate;
    fmt.blockAlign    = blockAlign;
    fmt.bitsPerSample = m_bitsPerSample;

    DataChunkHeader data{};

    if (!writeRawBytes(reinterpret_cast<const std::uint8_t*>(&riff), sizeof(riff))) return false;
    if (!writeRawBytes(reinterpret_cast<const std::uint8_t*>(&fmt), sizeof(fmt))) return false;
    if (!writeRawBytes(reinterpret_cast<const std::uint8_t*>(&data), sizeof(data))) return false;

    m_writtenFrames = 0;
    m_dataBytesWritten = 0;
    return true;
}

bool WavWriter::updateHeaders() {
    const std::uint32_t dataBytes = static_cast<std::uint32_t>(m_dataBytesWritten);
    const std::uint32_t riffSize  = static_cast<std::uint32_t>(sizeof(RiffHeader) - 8 + sizeof(FmtChunk) + sizeof(DataChunkHeader) + dataBytes);

    if (m_toMemory) {
        if (!m_memoryTarget || m_memoryTarget->size() < sizeof(RiffHeader) + sizeof(FmtChunk) + sizeof(DataChunkHeader)) {
            return false;
        }
        std::memcpy(m_memoryTarget->data() + 4, &riffSize, sizeof(riffSize));
        std::memcpy(m_memoryTarget->data() + sizeof(RiffHeader) + sizeof(FmtChunk) + 4, &dataBytes, sizeof(dataBytes));
        return true;
    }

    if (!m_fileStream.is_open()) return false;

    m_fileStream.seekp(4, std::ios::beg);
    m_fileStream.write(reinterpret_cast<const char*>(&riffSize), sizeof(riffSize));

    m_fileStream.seekp(sizeof(RiffHeader) + sizeof(FmtChunk) + 4, std::ios::beg);
    m_fileStream.write(reinterpret_cast<const char*>(&dataBytes), sizeof(dataBytes));

    m_fileStream.seekp(0, std::ios::end);
    return m_fileStream.good();
}

std::size_t WavWriter::writeFrames(std::span<const float> pcmIn) {
    if (!m_isOpen || pcmIn.empty()) return 0;

    const std::size_t totalSamples = pcmIn.size();
    const std::size_t numFrames = totalSamples / m_config.channels;
    const std::size_t validSamples = numFrames * m_config.channels;
    if (validSamples == 0) return 0;

    const std::size_t bytesPerSample = m_bitsPerSample / 8;
    const std::size_t byteCount = validSamples * bytesPerSample;

    std::vector<std::uint8_t> encodeBuffer;
    encodeBuffer.resize(byteCount);

    if (m_audioFormat == kWavFormatPcm) {
        if (m_bitsPerSample == 16) {
            auto* dst = reinterpret_cast<std::int16_t*>(encodeBuffer.data());
            for (std::size_t i = 0; i < validSamples; ++i) {
                const float s = std::clamp(pcmIn[i], -1.0f, 1.0f);
                dst[i] = static_cast<std::int16_t>(s >= 0.0f ? s * 32767.0f : s * 32768.0f);
            }
        } else if (m_bitsPerSample == 24) {
            std::uint8_t* dst = encodeBuffer.data();
            for (std::size_t i = 0; i < validSamples; ++i) {
                const float s = std::clamp(pcmIn[i], -1.0f, 1.0f);
                const auto val = static_cast<std::int32_t>(s >= 0.0f ? s * 8388607.0f : s * 8388608.0f);
                dst[0] = static_cast<std::uint8_t>(val & 0xFF);
                dst[1] = static_cast<std::uint8_t>((val >> 8) & 0xFF);
                dst[2] = static_cast<std::uint8_t>((val >> 16) & 0xFF);
                dst += 3;
            }
        } else if (m_bitsPerSample == 32) {
            auto* dst = reinterpret_cast<std::int32_t*>(encodeBuffer.data());
            for (std::size_t i = 0; i < validSamples; ++i) {
                const float s = std::clamp(pcmIn[i], -1.0f, 1.0f);
                dst[i] = static_cast<std::int32_t>(s >= 0.0f ? s * 2147483647.0f : s * 2147483648.0f);
            }
        }
    } else if (m_audioFormat == kWavFormatFloat && m_bitsPerSample == 32) {
        std::memcpy(encodeBuffer.data(), pcmIn.data(), byteCount);
    }

    if (!writeRawBytes(encodeBuffer.data(), byteCount)) {
        return 0;
    }

    m_writtenFrames += numFrames;
    m_dataBytesWritten += byteCount;
    return numFrames;
}

bool WavWriter::writeOneShot(const std::string& filepath,
                             std::span<const float> samples,
                             const AudioCodecConfig& config) {
    WavWriter writer;
    if (!writer.open(filepath, config)) return false;
    const std::size_t expectedFrames = samples.size() / config.channels;
    const std::size_t actualFrames = writer.writeFrames(samples);
    writer.close();
    return actualFrames == expectedFrames;
}

} // namespace job::sound