#include "flac_encoder.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <FLAC/stream_encoder.h>
#include <job_logger.h>

namespace job::sound {

struct FlacEncoderCallbacks {
    static FLAC__StreamEncoderWriteStatus writeCallback(
        const FLAC__StreamEncoder*,
        const FLAC__byte buffer[],
        std::size_t bytes,
        unsigned int /*samples*/,
        unsigned int /*current_frame*/,
        void* clientData) {
        auto* self = static_cast<FlacAudioEncoder*>(clientData);
        if (!self || bytes == 0) return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;

        self->m_pendingOutput.insert(self->m_pendingOutput.end(), buffer, buffer + bytes);
        return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
    }
};

void FlacAudioEncoder::FlacDeleter::operator()(::FLAC__StreamEncoder* ptr) const noexcept {
    if (ptr) {
        FLAC__stream_encoder_finish(ptr);
        FLAC__stream_encoder_delete(ptr);
    }
}

FlacAudioEncoder::FlacAudioEncoder() = default;

FlacAudioEncoder::~FlacAudioEncoder() = default;

FlacAudioEncoder::FlacAudioEncoder(FlacAudioEncoder&&) noexcept = default;

FlacAudioEncoder& FlacAudioEncoder::operator=(FlacAudioEncoder&&) noexcept = default;

bool FlacAudioEncoder::init(const AudioCodecConfig& config) {
    reset();

    if (!config.isValid()) {
        JOB_LOG_WARN("[FlacEncoder] Invalid audio configuration");
        return false;
    }

    if (config.channels < 1 || config.channels > 8) {
        JOB_LOG_WARN("[FlacEncoder] Unsupported channel count: {} (FLAC supports 1-8)", config.channels);
        return false;
    }

    m_bitsPerSample = 16;
    if (config.sampleFormat == SampleFormat::Int24) {
        m_bitsPerSample = 24;
    } else if (config.sampleFormat == SampleFormat::Int32) {
        m_bitsPerSample = 32;
    }

    ::FLAC__StreamEncoder* enc = FLAC__stream_encoder_new();
    if (!enc) {
        JOB_LOG_WARN("[FlacEncoder] Failed to allocate FLAC stream encoder");
        return false;
    }

    m_encoder.reset(enc);
    m_config = config;
    m_config.codecType = AudioCodecType::Flac;
    m_currentPts = 0;

    const int compressionLevel = std::clamp(config.compressionLevel >= 0 ? config.compressionLevel : 5, 0, 8);

    FLAC__stream_encoder_set_channels(m_encoder.get(), config.channels);
    FLAC__stream_encoder_set_bits_per_sample(m_encoder.get(), m_bitsPerSample);
    FLAC__stream_encoder_set_sample_rate(m_encoder.get(), config.sampleRate);
    FLAC__stream_encoder_set_compression_level(m_encoder.get(), static_cast<unsigned int>(compressionLevel));

    // Set blocksize if specified
    if (config.frameSize > 0) {
        FLAC__stream_encoder_set_blocksize(m_encoder.get(), static_cast<unsigned int>(config.frameSize));
    }

    const FLAC__StreamEncoderInitStatus status = FLAC__stream_encoder_init_stream(
        m_encoder.get(),
        &FlacEncoderCallbacks::writeCallback,
        nullptr, // seek_callback
        nullptr, // tell_callback
        nullptr, // metadata_callback
        this
        );

    if (status != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
        JOB_LOG_WARN("[FlacEncoder] Init stream failed: {}", FLAC__StreamEncoderInitStatusString[status]);
        m_encoder.reset();
        return false;
    }

    return true;
}

void FlacAudioEncoder::reset() noexcept {
    m_encoder.reset();
    m_pendingOutput.clear();
    m_pcmIntBuffer.clear();
    m_currentPts = 0;
}

bool FlacAudioEncoder::encode(std::span<const float> pcmIn, AudioPacket& packetOut) {
    packetOut.clear();

    if (!m_encoder) {
        JOB_LOG_WARN("[FlacEncoder] Encoder is not initialized");
        return false;
    }

    if (pcmIn.empty()) return true;

    const std::size_t totalSamples = pcmIn.size();
    const std::size_t numFrames = totalSamples / m_config.channels;
    const std::size_t validSamples = numFrames * m_config.channels;
    if (validSamples == 0) return true;

    m_pcmIntBuffer.resize(validSamples);

    // Convert float [-1.0f, 1.0f] to integer PCM scaled for FLAC
    if (m_bitsPerSample == 16) {
        for (std::size_t i = 0; i < validSamples; ++i) {
            const float s = std::clamp(pcmIn[i], -1.0f, 1.0f);
            m_pcmIntBuffer[i] = static_cast<std::int32_t>(s >= 0.0f ? s * 32767.0f : s * 32768.0f);
        }
    } else if (m_bitsPerSample == 24) {
        for (std::size_t i = 0; i < validSamples; ++i) {
            const float s = std::clamp(pcmIn[i], -1.0f, 1.0f);
            m_pcmIntBuffer[i] = static_cast<std::int32_t>(s >= 0.0f ? s * 8388607.0f : s * 8388608.0f);
        }
    } else if (m_bitsPerSample == 32) {
        for (std::size_t i = 0; i < validSamples; ++i) {
            const float s = std::clamp(pcmIn[i], -1.0f, 1.0f);
            m_pcmIntBuffer[i] = static_cast<std::int32_t>(s >= 0.0f ? s * 2147483647.0f : s * 2147483648.0f);
        }
    }

    m_pendingOutput.clear();

    if (!FLAC__stream_encoder_process_interleaved(
            m_encoder.get(),
            m_pcmIntBuffer.data(),
            static_cast<unsigned int>(numFrames))) {
        JOB_LOG_WARN("[FlacEncoder] Process interleaved samples failed");
        return false;
    }

    if (!m_pendingOutput.empty()) {
        packetOut.data = std::move(m_pendingOutput);
        packetOut.pts = m_currentPts;
        packetOut.duration = static_cast<std::int64_t>(numFrames);
        packetOut.isKeyframe = true;
    }

    m_currentPts += static_cast<std::int64_t>(numFrames);
    return true;
}

bool FlacAudioEncoder::flush(std::vector<AudioPacket>& flushedPackets) {
    flushedPackets.clear();

    if (!m_encoder) return false;

    m_pendingOutput.clear();

    // Finalize stream to force out all buffered frames and metadata headers
    if (!FLAC__stream_encoder_finish(m_encoder.get())) {
        JOB_LOG_WARN("[FlacEncoder] Finish stream failed during flush");
        return false;
    }

    if (!m_pendingOutput.empty()) {
        AudioPacket packet;
        packet.data = std::move(m_pendingOutput);
        packet.pts = m_currentPts;
        packet.duration = 0;
        packet.isKeyframe = true;
        flushedPackets.push_back(std::move(packet));
    }

    return true;
}

} // namespace job::sound