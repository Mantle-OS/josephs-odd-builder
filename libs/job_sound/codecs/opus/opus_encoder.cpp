#include "opus_encoder.h"

#include <algorithm>
#include <opus/opus.h>
#include <job_logger.h>

namespace job::sound {

void OpusAudioEncoder::OpusDeleter::operator()(::OpusEncoder* ptr) const noexcept {
    if (ptr) {
        opus_encoder_destroy(ptr);
    }
}

OpusAudioEncoder::OpusAudioEncoder() = default;

OpusAudioEncoder::~OpusAudioEncoder() = default;

OpusAudioEncoder::OpusAudioEncoder(OpusAudioEncoder&&) noexcept = default;

OpusAudioEncoder& OpusAudioEncoder::operator=(OpusAudioEncoder&&) noexcept = default;

bool OpusAudioEncoder::isValidOpusFrameSize(std::size_t frameSize, unsigned int sampleRate) noexcept {
    // Valid frame durations in ms: 2.5, 5, 10, 20, 40, 60
    // frameSize is in samples per channel
    const std::size_t rate = sampleRate;
    return (frameSize == (rate * 25) / 10000) || // 2.5 ms
           (frameSize == (rate * 5)  / 1000)  || // 5.0 ms
           (frameSize == (rate * 10) / 1000)  || // 10.0 ms
           (frameSize == (rate * 20) / 1000)  || // 20.0 ms
           (frameSize == (rate * 40) / 1000)  || // 40.0 ms
           (frameSize == (rate * 60) / 1000);    // 60.0 ms
}

bool OpusAudioEncoder::init(const AudioCodecConfig& config) {
    reset();

    // Opus supports 8kHz, 12kHz, 16kHz, 24kHz, and 48kHz
    if (config.sampleRate != 8000 && config.sampleRate != 12000 &&
        config.sampleRate != 16000 && config.sampleRate != 24000 &&
        config.sampleRate != 48000) {
        JOB_LOG_WARN("[OpusEncoder] Unsupported sample rate: {} Hz (must be 8k, 12k, 16k, 24k, or 48k)", config.sampleRate);
        return false;
    }

    if (config.channels != 1 && config.channels != 2) {
        JOB_LOG_WARN("[OpusEncoder] Unsupported channel count: {} (must be 1 or 2)", config.channels);
        return false;
    }

    // Default to 20ms frame size if unspecified (960 samples @ 48kHz)
    std::size_t frameSize = config.frameSize;
    if (frameSize == 0) {
        frameSize = (config.sampleRate * 20) / 1000;
    }

    if (!isValidOpusFrameSize(frameSize, config.sampleRate)) {
        JOB_LOG_WARN("[OpusEncoder] Invalid frame size: {} samples (must be 2.5ms, 5ms, 10ms, 20ms, 40ms, or 60ms)", frameSize);
        return false;
    }

    int error = OPUS_OK;
    ::OpusEncoder* enc = opus_encoder_create(
        static_cast<opus_int32>(config.sampleRate),
        static_cast<int>(config.channels),
        OPUS_APPLICATION_AUDIO, // High fidelity / general audio
        &error
        );

    if (error != OPUS_OK || !enc) {
        JOB_LOG_WARN("[OpusEncoder] Failed to create Opus encoder: {}", opus_strerror(error));
        return false;
    }

    m_encoder.reset(enc);
    m_config = config;
    m_config.codecType = AudioCodecType::Opus;
    m_config.sampleFormat = SampleFormat::Float32;
    m_config.frameSize = frameSize;
    m_currentPts = 0;

    // Apply baseline options
    setBitrate(config.bitrate > 0 ? config.bitrate : 64000);
    setComplexity(config.compressionLevel >= 0 ? config.compressionLevel : 5);
    setVbr(true);

    return true;
}

bool OpusAudioEncoder::setBitrate(unsigned int bitrateBps) {
    if (!m_encoder) return false;
    const int ret = opus_encoder_ctl(m_encoder.get(), OPUS_SET_BITRATE(static_cast<opus_int32>(bitrateBps)));
    if (ret == OPUS_OK) {
        m_config.bitrate = bitrateBps;
        return true;
    }
    return false;
}

bool OpusAudioEncoder::setComplexity(int complexity) {
    if (!m_encoder) return false;
    const int clamped = std::clamp(complexity, 0, 10);
    const int ret = opus_encoder_ctl(m_encoder.get(), OPUS_SET_COMPLEXITY(clamped));
    if (ret == OPUS_OK) {
        m_config.compressionLevel = clamped;
        return true;
    }
    return false;
}

bool OpusAudioEncoder::setVbr(bool enabled) {
    if (!m_encoder) return false;
    return opus_encoder_ctl(m_encoder.get(), OPUS_SET_VBR(enabled ? 1 : 0)) == OPUS_OK;
}

void OpusAudioEncoder::reset() noexcept {
    if (m_encoder) {
        opus_encoder_ctl(m_encoder.get(), OPUS_RESET_STATE);
    }
    m_currentPts = 0;
}

bool OpusAudioEncoder::encode(std::span<const float> pcmIn, AudioPacket& packetOut) {
    packetOut.clear();

    if (!m_encoder) {
        JOB_LOG_WARN("[OpusEncoder] Encoder is not initialized");
        return false;
    }

    const std::size_t requiredSamples = m_config.frameSize * m_config.channels;
    if (pcmIn.size() != requiredSamples) {
        JOB_LOG_WARN("[OpusEncoder] Input size mismatch: expected {} samples, got {}", requiredSamples, pcmIn.size());
        return false;
    }

    // Max recommended payload buffer for an Opus packet is 4000 bytes
    constexpr std::size_t kMaxPacketBytes = 4000;
    packetOut.data.resize(kMaxPacketBytes);

    const opus_int32 bytesEncoded = opus_encode_float(
        m_encoder.get(),
        pcmIn.data(),
        static_cast<int>(m_config.frameSize),
        packetOut.data.data(),
        static_cast<opus_int32>(packetOut.data.size())
        );

    if (bytesEncoded < 0) {
        JOB_LOG_WARN("[OpusEncoder] Encode error: {}", opus_strerror(bytesEncoded));
        packetOut.clear();
        return false;
    }

    packetOut.data.resize(static_cast<std::size_t>(bytesEncoded));
    packetOut.pts = m_currentPts;
    packetOut.duration = static_cast<std::int64_t>(m_config.frameSize);
    packetOut.isKeyframe = true;

    m_currentPts += static_cast<std::int64_t>(m_config.frameSize);
    return true;
}

bool OpusAudioEncoder::flush(std::vector<AudioPacket>& flushedPackets) {
    flushedPackets.clear();
    // Standard Opus stream encoder doesn't store trailing multi-packet queues
    return true;
}

} // namespace job::sound