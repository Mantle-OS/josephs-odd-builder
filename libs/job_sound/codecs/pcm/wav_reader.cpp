#include "wav_reader.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <job_logger.h>

namespace job::sound {

namespace {

#pragma pack(push, 1)
struct RiffChunkHeader {
    char id[4];
    std::uint32_t size;
};

struct FmtChunkBody {
    std::uint16_t audioFormat;
    std::uint16_t numChannels;
    std::uint32_t sampleRate;
    std::uint32_t byteRate;
    std::uint16_t blockAlign;
    std::uint16_t bitsPerSample;
};
#pragma pack(pop)

constexpr std::uint16_t kWavFormatPcm   = 1;
constexpr std::uint16_t kWavFormatFloat = 3;

} // namespace

WavReader::~WavReader() {
    close();
}

WavReader::WavReader(WavReader&& other) noexcept {
    *this = std::move(other);
}

WavReader& WavReader::operator=(WavReader&& other) noexcept {
    if (this != &other) {
        close();
        m_config          = other.m_config;
        m_totalFrames     = other.m_totalFrames;
        m_framesRead      = other.m_framesRead;
        m_dataChunkOffset = other.m_dataChunkOffset;
        m_dataChunkSize   = other.m_dataChunkSize;
        m_audioFormat     = other.m_audioFormat;
        m_bitsPerSample   = other.m_bitsPerSample;
        m_fileStream      = std::move(other.m_fileStream);
        m_memSpan         = other.m_memSpan;
        m_memOffset       = other.m_memOffset;
        m_fromMemory      = other.m_fromMemory;
        m_isOpen          = other.m_isOpen;

        other.m_isOpen = false;
    }
    return *this;
}

void WavReader::close() noexcept {
    if (m_fileStream.is_open()) {
        m_fileStream.close();
    }
    m_memSpan = {};
    m_memOffset = 0;
    m_framesRead = 0;
    m_totalFrames = 0;
    m_isOpen = false;
}

bool WavReader::open(const std::string& filepath) {
    close();
    m_fromMemory = false;
    m_fileStream.open(filepath, std::ios::binary);
    if (!m_fileStream.is_open()) {
        JOB_LOG_WARN("[WavReader] Cannot open file: {}", filepath);
        return false;
    }

    if (!parseHeader()) {
        close();
        return false;
    }

    m_isOpen = true;
    return true;
}

bool WavReader::open(std::span<const std::uint8_t> memoryBuffer) {
    close();
    m_fromMemory = true;
    m_memSpan = memoryBuffer;
    m_memOffset = 0;

    if (!parseHeader()) {
        close();
        return false;
    }

    m_isOpen = true;
    return true;
}

std::size_t WavReader::readRawBytes(std::uint8_t* dest, std::size_t byteCount) {
    if (m_fromMemory) {
        const std::size_t available = (m_memOffset < m_memSpan.size()) ? (m_memSpan.size() - m_memOffset) : 0;
        const std::size_t toRead = std::min(byteCount, available);
        if (toRead > 0) {
            std::memcpy(dest, m_memSpan.data() + m_memOffset, toRead);
            m_memOffset += toRead;
        }
        return toRead;
    }

    m_fileStream.read(reinterpret_cast<char*>(dest), static_cast<std::streamsize>(byteCount));
    return static_cast<std::size_t>(m_fileStream.gcount());
}

bool WavReader::parseHeader() {
    char riffHeader[12]{};
    if (readRawBytes(reinterpret_cast<std::uint8_t*>(riffHeader), 12) != 12) {
        return false;
    }

    if (std::memcmp(riffHeader, "RIFF", 4) != 0 || std::memcmp(riffHeader + 8, "WAVE", 4) != 0) {
        JOB_LOG_WARN("[WavReader] File is not a valid RIFF/WAVE container");
        return false;
    }

    bool foundFmt = false;
    bool foundData = false;
    std::size_t currentOffset = 12;

    while (!foundData) {
        RiffChunkHeader chunk{};
        if (readRawBytes(reinterpret_cast<std::uint8_t*>(&chunk), sizeof(chunk)) != sizeof(chunk)) {
            break;
        }
        currentOffset += sizeof(chunk);

        if (std::memcmp(chunk.id, "fmt ", 4) == 0) {
            if (chunk.size < sizeof(FmtChunkBody)) return false;

            FmtChunkBody fmt{};
            if (readRawBytes(reinterpret_cast<std::uint8_t*>(&fmt), sizeof(fmt)) != sizeof(fmt)) return false;

            m_audioFormat   = fmt.audioFormat;
            m_bitsPerSample = fmt.bitsPerSample;

            m_config.codecType  = AudioCodecType::Pcm;
            m_config.sampleRate = fmt.sampleRate;
            m_config.channels   = fmt.numChannels;

            if (m_audioFormat == kWavFormatPcm) {
                if (m_bitsPerSample == 16)      m_config.sampleFormat = SampleFormat::Int16;
                else if (m_bitsPerSample == 24) m_config.sampleFormat = SampleFormat::Int24;
                else if (m_bitsPerSample == 32) m_config.sampleFormat = SampleFormat::Int32;
                else return false;
            } else if (m_audioFormat == kWavFormatFloat && m_bitsPerSample == 32) {
                m_config.sampleFormat = SampleFormat::Float32;
            } else {
                JOB_LOG_WARN("[WavReader] Unsupported format: {} with {} bits/sample", m_audioFormat, m_bitsPerSample);
                return false;
            }

            // Skip any extra extension bytes in fmt chunk
            if (chunk.size > sizeof(fmt)) {
                const std::size_t skip = chunk.size - sizeof(fmt);
                if (m_fromMemory) m_memOffset += skip;
                else m_fileStream.seekg(static_cast<std::streamoff>(skip), std::ios::cur);
            }
            currentOffset += chunk.size;
            foundFmt = true;
        } else if (std::memcmp(chunk.id, "data", 4) == 0) {
            m_dataChunkOffset = currentOffset;
            m_dataChunkSize   = chunk.size;
            foundData = true;
        } else {
            // Skip unknown chunk
            if (m_fromMemory) m_memOffset += chunk.size;
            else m_fileStream.seekg(static_cast<std::streamoff>(chunk.size), std::ios::cur);
            currentOffset += chunk.size;
        }
    }

    if (!foundFmt || !foundData || m_config.channels == 0) {
        return false;
    }

    const std::size_t bytesPerFrame = m_config.channels * (m_bitsPerSample / 8);
    m_totalFrames = (bytesPerFrame > 0) ? (m_dataChunkSize / bytesPerFrame) : 0;
    m_framesRead = 0;

    return seek(0);
}

bool WavReader::seek(std::size_t frameIndex) {
    if (!m_isOpen && m_dataChunkOffset == 0) return false;
    if (frameIndex > m_totalFrames) return false;

    const std::size_t bytesPerFrame = m_config.channels * (m_bitsPerSample / 8);
    const std::size_t byteOffset = m_dataChunkOffset + (frameIndex * bytesPerFrame);

    if (m_fromMemory) {
        if (byteOffset > m_memSpan.size()) return false;
        m_memOffset = byteOffset;
    } else {
        m_fileStream.clear();
        m_fileStream.seekg(static_cast<std::streamoff>(byteOffset), std::ios::beg);
        if (!m_fileStream.good()) return false;
    }

    m_framesRead = frameIndex;
    return true;
}

std::size_t WavReader::readFrames(std::vector<float>& pcmOut, std::size_t maxFrames) {
    if (!m_isOpen) return 0;

    const std::size_t framesToRead = std::min(maxFrames, m_totalFrames - m_framesRead);
    if (framesToRead == 0) return 0;

    const std::size_t samplesToRead = framesToRead * m_config.channels;
    const std::size_t bytesPerSample = m_bitsPerSample / 8;
    const std::size_t bytesToRead = samplesToRead * bytesPerSample;

    std::vector<std::uint8_t> rawBuffer(bytesToRead);
    const std::size_t bytesGot = readRawBytes(rawBuffer.data(), bytesToRead);
    const std::size_t actualSamples = bytesGot / bytesPerSample;
    const std::size_t actualFrames  = actualSamples / m_config.channels;

    pcmOut.resize(actualSamples);

    if (m_audioFormat == kWavFormatPcm) {
        if (m_bitsPerSample == 16) {
            const auto* src = reinterpret_cast<const std::int16_t*>(rawBuffer.data());
            for (std::size_t i = 0; i < actualSamples; ++i) {
                pcmOut[i] = static_cast<float>(src[i]) / 32768.0f;
            }
        } else if (m_bitsPerSample == 24) {
            const std::uint8_t* src = rawBuffer.data();
            for (std::size_t i = 0; i < actualSamples; ++i) {
                // 24-bit sign extension
                std::int32_t val = (static_cast<std::uint32_t>(src[0])) |
                                   (static_cast<std::uint32_t>(src[1]) << 8) |
                                   (static_cast<std::uint32_t>(src[2]) << 16);
                if (val & 0x800000) val |= ~0xFFFFFF; // Sign extend to 32-bit
                pcmOut[i] = static_cast<float>(val) / 8388608.0f;
                src += 3;
            }
        } else if (m_bitsPerSample == 32) {
            const auto* src = reinterpret_cast<const std::int32_t*>(rawBuffer.data());
            for (std::size_t i = 0; i < actualSamples; ++i) {
                pcmOut[i] = static_cast<float>(src[i]) / 2147483648.0f;
            }
        }
    } else if (m_audioFormat == kWavFormatFloat && m_bitsPerSample == 32) {
        std::memcpy(pcmOut.data(), rawBuffer.data(), actualSamples * sizeof(float));
    }

    m_framesRead += actualFrames;
    return actualFrames;
}

bool WavReader::readAll(std::vector<float>& pcmOut) {
    if (!seek(0)) return false;
    return readFrames(pcmOut, m_totalFrames) == m_totalFrames;
}

} // namespace job::sound