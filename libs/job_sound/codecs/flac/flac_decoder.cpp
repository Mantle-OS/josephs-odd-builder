#include "flac_decoder.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <FLAC/stream_decoder.h>
#include <job_logger.h>

namespace job::sound {

struct FlacDecoderCallbacks {
    static FLAC__StreamDecoderReadStatus readCallback(
        const FLAC__StreamDecoder*,
        FLAC__byte buffer[],
        std::size_t* bytes,
        void* clientData) {
        auto* self = static_cast<FlacAudioDecoder*>(clientData);

        if (!self || *bytes == 0) return FLAC__STREAM_DECODER_READ_STATUS_ABORT;

        const std::size_t available = (self->m_inputOffset < self->m_currentInputSpan.size())
                                          ? (self->m_currentInputSpan.size() - self->m_inputOffset)
                                          : 0;

        if (available == 0) {
            *bytes = 0;
            return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
        }

        const std::size_t toCopy = std::min(*bytes, available);
        std::memcpy(buffer, self->m_currentInputSpan.data() + self->m_inputOffset, toCopy);
        self->m_inputOffset += toCopy;
        *bytes = toCopy;

        return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
    }

    static FLAC__StreamDecoderWriteStatus writeCallback(
        const FLAC__StreamDecoder*,
        const FLAC__Frame* frame,
        const FLAC__int32* const buffer[],
        void* clientData) {
        auto* self = static_cast<FlacAudioDecoder*>(clientData);

        if (!self || !self->m_currentOutputTarget || !frame) {
            return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
        }

        const unsigned int channels = frame->header.channels;
        const unsigned int blocksize = frame->header.blocksize;
        const unsigned int bps = frame->header.bits_per_sample;

        // Dynamic scale factor according to bit depth
        float scale = 1.0f / 32768.0f;
        if (bps == 24) {
            scale = 1.0f / 8388608.0f;
        } else if (bps == 32) {
            scale = 1.0f / 2147483648.0f;
        }

        auto& target = *self->m_currentOutputTarget;
        const std::size_t currentSize = target.size();
        target.resize(currentSize + (blocksize * channels));

        // Interleave planar channels into target float buffer
        for (unsigned int i = 0; i < blocksize; ++i) {
            for (unsigned int ch = 0; ch < channels; ++ch) {
                target[currentSize + (i * channels) + ch] = static_cast<float>(buffer[ch][i]) * scale;
            }
        }

        return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
    }

    static void metadataCallback(
        const FLAC__StreamDecoder*,
        const FLAC__StreamMetadata* metadata,
        void* clientData) {
        auto* self = static_cast<FlacAudioDecoder*>(clientData);
        if (!self || !metadata) return;

        if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
            self->m_config.sampleRate = metadata->data.stream_info.sample_rate;
            self->m_config.channels   = metadata->data.stream_info.channels;
            const unsigned int bps   = metadata->data.stream_info.bits_per_sample;

            if (bps == 16)      self->m_config.sampleFormat = SampleFormat::Int16;
            else if (bps == 24) self->m_config.sampleFormat = SampleFormat::Int24;
            else if (bps == 32) self->m_config.sampleFormat = SampleFormat::Int32;

            self->m_metadataParsed = true;
        }
    }

    static void errorCallback(
        const FLAC__StreamDecoder*,
        FLAC__StreamDecoderErrorStatus status,
        void*) {
        JOB_LOG_WARN("[FlacDecoder] Stream error: {}", FLAC__StreamDecoderErrorStatusString[status]);
    }
};

void FlacAudioDecoder::FlacDeleter::operator()(::FLAC__StreamDecoder* ptr) const noexcept {
    if (ptr) {
        FLAC__stream_decoder_finish(ptr);
        FLAC__stream_decoder_delete(ptr);
    }
}

FlacAudioDecoder::FlacAudioDecoder() = default;

FlacAudioDecoder::~FlacAudioDecoder() = default;

FlacAudioDecoder::FlacAudioDecoder(FlacAudioDecoder&&) noexcept = default;

FlacAudioDecoder& FlacAudioDecoder::operator=(FlacAudioDecoder&&) noexcept = default;

bool FlacAudioDecoder::init(const AudioCodecConfig& config) {
    reset();

    ::FLAC__StreamDecoder* dec = FLAC__stream_decoder_new();
    if (!dec) {
        JOB_LOG_WARN("[FlacDecoder] Failed to allocate FLAC stream decoder");
        return false;
    }

    m_decoder.reset(dec);
    m_config = config;
    m_config.codecType = AudioCodecType::Flac;
    m_config.sampleFormat = SampleFormat::Float32;
    m_metadataParsed = false;

    const FLAC__StreamDecoderInitStatus status = FLAC__stream_decoder_init_stream(
        m_decoder.get(),
        &FlacDecoderCallbacks::readCallback,
        nullptr, // seek_callback
        nullptr, // tell_callback
        nullptr, // length_callback
        nullptr, // eof_callback
        &FlacDecoderCallbacks::writeCallback,
        &FlacDecoderCallbacks::metadataCallback,
        &FlacDecoderCallbacks::errorCallback,
        this
        );

    if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        JOB_LOG_WARN("[FlacDecoder] Init stream failed: {}", FLAC__StreamDecoderInitStatusString[status]);
        m_decoder.reset();
        return false;
    }

    return true;
}

void FlacAudioDecoder::reset() noexcept {
    if (m_decoder) {
        FLAC__stream_decoder_reset(m_decoder.get());
    }
    m_currentInputSpan = {};
    m_inputOffset = 0;
    m_currentOutputTarget = nullptr;
    m_metadataParsed = false;
}

bool FlacAudioDecoder::decode(std::span<const std::uint8_t> packetIn, std::vector<float>& pcmOut) {
    pcmOut.clear();

    if (!m_decoder) {
        JOB_LOG_WARN("[FlacDecoder] Decoder is not initialized");
        return false;
    }

    if (packetIn.empty()) return true;

    m_currentInputSpan    = packetIn;
    m_inputOffset         = 0;
    m_currentOutputTarget = &pcmOut;

    // Process available input chunks
    while (m_inputOffset < m_currentInputSpan.size()) {
        if (!FLAC__stream_decoder_process_single(m_decoder.get())) {
            break;
        }

        const FLAC__StreamDecoderState state = FLAC__stream_decoder_get_state(m_decoder.get());
        if (state == FLAC__STREAM_DECODER_END_OF_STREAM || state == FLAC__STREAM_DECODER_ABORTED) {
            break;
        }
    }

    m_currentOutputTarget = nullptr;
    return true;
}

bool FlacAudioDecoder::flush(std::vector<float>& pcmOut) {
    pcmOut.clear();
    if (!m_decoder) return false;

    m_currentInputSpan    = {};
    m_inputOffset         = 0;
    m_currentOutputTarget = &pcmOut;

    FLAC__stream_decoder_flush(m_decoder.get());
    m_currentOutputTarget = nullptr;
    return true;
}

} // namespace job::sound