#include "opus_decoder.h"

#include <opus/opus.h>
#include <job_logger.h>

namespace job::sound {

void OpusAudioDecoder::OpusDeleter::operator()(::OpusDecoder* ptr) const noexcept {
    if (ptr) {
        opus_decoder_destroy(ptr);
    }
}

OpusAudioDecoder::OpusAudioDecoder() = default;

OpusAudioDecoder::~OpusAudioDecoder() = default;

OpusAudioDecoder::OpusAudioDecoder(OpusAudioDecoder&&) noexcept = default;

OpusAudioDecoder& OpusAudioDecoder::operator=(OpusAudioDecoder&&) noexcept = default;

bool OpusAudioDecoder::init(const AudioCodecConfig& config) {
    reset();

    if (config.sampleRate != 8000 && config.sampleRate != 12000 &&
        config.sampleRate != 16000 && config.sampleRate != 24000 &&
        config.sampleRate != 48000) {
        JOB_LOG_WARN("[OpusDecoder] Unsupported sample rate: {} Hz (must be 8k, 12k, 16k, 24k, or 48k)", config.sampleRate);
        return false;
    }

    if (config.channels != 1 && config.channels != 2) {
        JOB_LOG_WARN("[OpusDecoder] Unsupported channel count: {} (must be 1 or 2)", config.channels);
        return false;
    }

    int error = OPUS_OK;
    ::OpusDecoder* dec = opus_decoder_create(
        static_cast<opus_int32>(config.sampleRate),
        static_cast<int>(config.channels),
        &error
        );

    if (error != OPUS_OK || !dec) {
        JOB_LOG_WARN("[OpusDecoder] Failed to create Opus decoder: {}", opus_strerror(error));
        return false;
    }

    m_decoder.reset(dec);
    m_config = config;
    m_config.codecType = AudioCodecType::Opus;
    m_config.sampleFormat = SampleFormat::Float32;

    if (m_config.frameSize == 0) {
        m_config.frameSize = (config.sampleRate * 20) / 1000;
    }

    return true;
}

void OpusAudioDecoder::reset() noexcept {
    if (m_decoder) {
        opus_decoder_ctl(m_decoder.get(), OPUS_RESET_STATE);
    }
}

bool OpusAudioDecoder::decode(std::span<const std::uint8_t> packetIn, std::vector<float>& pcmOut) {
    return decodeFrame(packetIn, pcmOut, false);
}

bool OpusAudioDecoder::decodeFrame(std::span<const std::uint8_t> packetIn, std::vector<float>& pcmOut, bool decodeFec) {
    if (!m_decoder) {
        JOB_LOG_WARN("[OpusDecoder] Decoder is not initialized");
        return false;
    }

    const bool isPlc = packetIn.empty();

    // For PLC, request the expected frame size (e.g. 960). For real frames, allow up to 120ms (5760).
    const int samplesToRequest = isPlc
                                     ? static_cast<int>(m_config.frameSize)
                                     : static_cast<int>((m_config.sampleRate * 120) / 1000);

    const std::size_t maxTotalSamples = static_cast<std::size_t>(samplesToRequest * m_config.channels);
    pcmOut.resize(maxTotalSamples);

    const unsigned char* payload = isPlc ? nullptr : packetIn.data();
    const opus_int32 payloadBytes = static_cast<opus_int32>(packetIn.size());

    const int decodedFrames = opus_decode_float(
        m_decoder.get(),
        payload,
        payloadBytes,
        pcmOut.data(),
        samplesToRequest,
        decodeFec ? 1 : 0
        );

    if (decodedFrames < 0) {
        JOB_LOG_WARN("[OpusDecoder] Decode error: {}", opus_strerror(decodedFrames));
        pcmOut.clear();
        return false;
    }

    pcmOut.resize(static_cast<std::size_t>(decodedFrames * m_config.channels));
    return true;
}

} // namespace job::sound


// #include "opus_decoder.h"

// #include <opus/opus.h>
// #include <job_logger.h>

// namespace job::sound {

// void OpusAudioDecoder::OpusDeleter::operator()(::OpusDecoder* ptr) const noexcept {
//     if (ptr) {
//         opus_decoder_destroy(ptr);
//     }
// }

// OpusAudioDecoder::OpusAudioDecoder() = default;

// OpusAudioDecoder::~OpusAudioDecoder() = default;

// OpusAudioDecoder::OpusAudioDecoder(OpusAudioDecoder&&) noexcept = default;

// OpusAudioDecoder& OpusAudioDecoder::operator=(OpusAudioDecoder&&) noexcept = default;

// bool OpusAudioDecoder::init(const AudioCodecConfig& config) {
//     reset();

//     // Opus supports 8kHz, 12kHz, 16kHz, 24kHz, and 48kHz
//     if (config.sampleRate != 8000 && config.sampleRate != 12000 &&
//         config.sampleRate != 16000 && config.sampleRate != 24000 &&
//         config.sampleRate != 48000) {
//         JOB_LOG_WARN("[OpusDecoder] Unsupported sample rate: {} Hz (must be 8k, 12k, 16k, 24k, or 48k)", config.sampleRate);
//         return false;
//     }

//     if (config.channels != 1 && config.channels != 2) {
//         JOB_LOG_WARN("[OpusDecoder] Unsupported channel count: {} (must be 1 or 2)", config.channels);
//         return false;
//     }

//     int error = OPUS_OK;
//     ::OpusDecoder* dec = opus_decoder_create(
//         static_cast<opus_int32>(config.sampleRate),
//         static_cast<int>(config.channels),
//         &error
//         );

//     if (error != OPUS_OK || !dec) {
//         JOB_LOG_WARN("[OpusDecoder] Failed to create Opus decoder: {}", opus_strerror(error));
//         return false;
//     }

//     m_decoder.reset(dec);
//     m_config = config;
//     m_config.codecType = AudioCodecType::Opus;
//     m_config.sampleFormat = SampleFormat::Float32;

//     if (m_config.frameSize == 0) {
//         // Default to standard 20ms frame size (960 samples @ 48kHz)
//         m_config.frameSize = (config.sampleRate * 20) / 1000;
//     }

//     return true;
// }

// void OpusAudioDecoder::reset() noexcept {
//     if (m_decoder) {
//         opus_decoder_ctl(m_decoder.get(), OPUS_RESET_STATE);
//     }
// }

// bool OpusAudioDecoder::decode(std::span<const std::uint8_t> packetIn, std::vector<float>& pcmOut) {
//     return decodeFrame(packetIn, pcmOut, false);
// }

// bool OpusAudioDecoder::decodeFrame(std::span<const std::uint8_t> packetIn, std::vector<float>& pcmOut, bool decodeFec) {
//     if (!m_decoder) {
//         JOB_LOG_WARN("[OpusDecoder] Decoder is not initialized");
//         return false;
//     }

//     // Maximum Opus frame duration is 120ms (5760 samples @ 48kHz)
//     const int maxSamplesPerChannel = static_cast<int>((m_config.sampleRate * 120) / 1000);
//     const std::size_t maxTotalSamples = static_cast<std::size_t>(maxSamplesPerChannel * m_config.channels);

//     pcmOut.resize(maxTotalSamples);

//     const unsigned char* payload = packetIn.empty() ? nullptr : packetIn.data();
//     const opus_int32 payloadBytes = static_cast<opus_int32>(packetIn.size());

//     // opus_decode_float produces interleaved samples across channels
//     const int decodedFrames = opus_decode_float(
//         m_decoder.get(),
//         payload,
//         payloadBytes,
//         pcmOut.data(),
//         maxSamplesPerChannel,
//         decodeFec ? 1 : 0
//         );

//     if (decodedFrames < 0) {
//         JOB_LOG_WARN("[OpusDecoder] Decode error: {}", opus_strerror(decodedFrames));
//         pcmOut.clear();
//         return false;
//     }

//     pcmOut.resize(static_cast<std::size_t>(decodedFrames * m_config.channels));
//     return true;
// }

// } // namespace job::sound