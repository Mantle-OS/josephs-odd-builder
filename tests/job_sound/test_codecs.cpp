#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <cmath>
#include <cstdint>
#include <numbers>
#include <vector>

#include "codecs/pcm/wav_reader.h"
#include "codecs/pcm/wav_writer.h"
#include "codecs/opus/opus_encoder.h"
#include "codecs/opus/opus_decoder.h"
#include "codecs/flac/flac_encoder.h"
#include "codecs/flac/flac_decoder.h"

#include "../transient_test_file.h"

using Catch::Approx;
using namespace job::sound;

namespace {

constexpr float kSampleRate = 48000.0f;
constexpr float kPi = std::numbers::pi_v<float>;

std::vector<float> makeSine(float frequency, std::size_t sampleCount, float amplitude = 0.5f, float sampleRate = kSampleRate) {
    std::vector<float> samples(sampleCount);
    for (std::size_t i = 0; i < sampleCount; ++i) {
        const float phase = 2.0f * kPi * frequency * static_cast<float>(i) / sampleRate;
        samples[i] = amplitude * std::sin(phase);
    }
    return samples;
}

} // namespace

// ============================================================================
// Block 1: Usage / Examples
// ============================================================================

TEST_CASE("WAV encoder and decoder round-trip disk session", "[sound][codec][wav][usage]") {
    const std::string testPath = "test_transient_output.wav";
    TransientTestFile fileGuard{testPath};

    AudioCodecConfig config{
        .sampleFormat = SampleFormat::Int16,
        .sampleRate = 48000,
        .channels = 1
    };

    const auto sourcePcm = makeSine(440.0f, 48000, 0.5f);

    // 1. Write to disk
    {
        WavWriter writer;
        REQUIRE(writer.open(testPath, config));
        REQUIRE(writer.writeFrames(sourcePcm) == 48000);
        writer.close();
        REQUIRE_FALSE(writer.isOpen());
    }

    // 2. Read back from disk
    {
        WavReader reader;
        REQUIRE(reader.open(testPath));
        CHECK(reader.config().sampleRate == 48000);
        CHECK(reader.config().channels == 1);
        CHECK(reader.totalFrames() == 48000);

        std::vector<float> readBack;
        REQUIRE(reader.readAll(readBack));
        REQUIRE(readBack.size() == sourcePcm.size());

        for (std::size_t i = 0; i < sourcePcm.size(); ++i) {
            REQUIRE(readBack[i] == Approx(sourcePcm[i]).margin(0.001f));
        }
    }
}

TEST_CASE("WAV in-memory streaming round-trip", "[sound][codec][wav][memory][usage]") {
    AudioCodecConfig config{
        .sampleFormat = SampleFormat::Float32,
        .sampleRate = 48000,
        .channels = 2
    };

    const auto monoSine = makeSine(1000.0f, 2048, 0.4f);
    std::vector<float> stereoSource(monoSine.size() * 2);
    for (std::size_t i = 0; i < monoSine.size(); ++i) {
        stereoSource[i * 2]     = monoSine[i];
        stereoSource[i * 2 + 1] = -monoSine[i];
    }

    std::vector<std::uint8_t> memoryStream;
    WavWriter writer;
    REQUIRE(writer.open(memoryStream, config));
    REQUIRE(writer.writeFrames(stereoSource) == 2048);
    writer.close();

    REQUIRE_FALSE(memoryStream.empty());

    WavReader reader;
    REQUIRE(reader.open(memoryStream));
    CHECK(reader.config().sampleRate == 48000);
    CHECK(reader.config().channels == 2);
    CHECK(reader.totalFrames() == 2048);

    std::vector<float> decoded;
    REQUIRE(reader.readAll(decoded));
    REQUIRE(decoded.size() == stereoSource.size());

    for (std::size_t i = 0; i < stereoSource.size(); ++i) {
        REQUIRE(decoded[i] == Approx(stereoSource[i]).margin(0.00001f));
    }
}

TEST_CASE("Opus encoder and decoder encode/decode 20ms audio frames", "[sound][codec][opus][usage]") {
    AudioCodecConfig config{
        .codecType = AudioCodecType::Opus,
        .sampleRate = 48000,
        .channels = 1,
        .bitrate = 64000,
        .frameSize = 960
    };

    OpusAudioEncoder encoder;
    OpusAudioDecoder decoder;

    REQUIRE(encoder.init(config));
    REQUIRE(decoder.init(config));

    const auto pcmChunk = makeSine(440.0f, config.frameSize, 0.5f);

    AudioPacket packet;
    REQUIRE(encoder.encode(pcmChunk, packet));
    CHECK_FALSE(packet.empty());
    CHECK(packet.duration == 960);
    CHECK(packet.pts == 0);

    std::vector<float> decodedPcm;
    REQUIRE(decoder.decode(packet.span(), decodedPcm));
    REQUIRE(decodedPcm.size() == config.frameSize);

    float correlation = 0.0f;
    for (std::size_t i = 0; i < pcmChunk.size(); ++i) {
        correlation += pcmChunk[i] * decodedPcm[i];
    }
    REQUIRE(correlation > 0.0f);
}

TEST_CASE("FLAC encoder and decoder lossless round-trip", "[sound][codec][flac][usage]") {
    AudioCodecConfig config{
        .codecType = AudioCodecType::Flac,
        .sampleFormat = SampleFormat::Int16,
        .sampleRate = 48000,
        .channels = 1,
        .frameSize = 512
    };

    FlacAudioEncoder encoder;
    FlacAudioDecoder decoder;

    REQUIRE(encoder.init(config));
    REQUIRE(decoder.init(config));

    const auto sourcePcm = makeSine(440.0f, 512, 0.5f);

    AudioPacket packet;
    REQUIRE(encoder.encode(sourcePcm, packet));

    std::vector<AudioPacket> flushed;
    REQUIRE(encoder.flush(flushed));

    std::vector<float> decodedPcm;
    if (!packet.empty()) {
        REQUIRE(decoder.decode(packet.span(), decodedPcm));
    }
    for (const auto& p : flushed) {
        std::vector<float> extra;
        REQUIRE(decoder.decode(p.span(), extra));
        decodedPcm.insert(decodedPcm.end(), extra.begin(), extra.end());
    }

    std::vector<float> remaining;
    REQUIRE(decoder.flush(remaining));
    decodedPcm.insert(decodedPcm.end(), remaining.begin(), remaining.end());

    REQUIRE(decodedPcm.size() == sourcePcm.size());
    for (std::size_t i = 0; i < sourcePcm.size(); ++i) {
        REQUIRE(decodedPcm[i] == Approx(sourcePcm[i]).margin(0.001f));
    }
}

// ============================================================================
// Block 2: Edge Cases / Invariants
// ============================================================================

TEST_CASE("WavReader handles corrupted/empty files gracefully", "[sound][codec][wav][edge]") {
    const std::string emptyPath = "test_empty.wav";
    TransientTestFile fileGuard{emptyPath, 0, 0};

    WavReader reader;
    REQUIRE_FALSE(reader.open(emptyPath));
    REQUIRE_FALSE(reader.isOpen());
}

TEST_CASE("Opus decoder handles packet loss concealment on empty packet", "[sound][codec][opus][plc][edge]") {
    AudioCodecConfig config{
        .codecType = AudioCodecType::Opus,
        .sampleRate = 48000,
        .channels = 1,
        .frameSize = 960
    };

    OpusAudioDecoder decoder;
    REQUIRE(decoder.init(config));

    std::vector<float> plcPcm;
    REQUIRE(decoder.decode({}, plcPcm));
    REQUIRE(plcPcm.size() == config.frameSize);
}

TEST_CASE("Opus encoder rejects invalid input buffer sizes", "[sound][codec][opus][size][edge]") {
    AudioCodecConfig config{
        .codecType = AudioCodecType::Opus,
        .sampleRate = 48000,
        .channels = 1,
        .frameSize = 960
    };

    OpusAudioEncoder encoder;
    REQUIRE(encoder.init(config));

    AudioPacket packet;
    const std::vector<float> badChunk(500, 0.0f);
    REQUIRE_FALSE(encoder.encode(badChunk, packet));
}

// ============================================================================
// Block 3: Benchmarks
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Opus 20ms frame encode benchmark", "[sound][codec][opus][benchmark]") {
    AudioCodecConfig config{
        .codecType = AudioCodecType::Opus,
        .sampleRate = 48000,
        .channels = 1,
        .bitrate = 64000,
        .frameSize = 960
    };

    OpusAudioEncoder encoder;
    REQUIRE(encoder.init(config));

    const auto pcmChunk = makeSine(440.0f, 960, 0.5f);
    AudioPacket packet;

    BENCHMARK("encode 20ms Opus frame") {
        return encoder.encode(pcmChunk, packet);
    };
}

TEST_CASE("Opus 20ms frame decode benchmark", "[sound][codec][opus][benchmark]") {
    AudioCodecConfig config{
        .codecType = AudioCodecType::Opus,
        .sampleRate = 48000,
        .channels = 1,
        .bitrate = 64000,
        .frameSize = 960
    };

    OpusAudioEncoder encoder;
    OpusAudioDecoder decoder;
    REQUIRE(encoder.init(config));
    REQUIRE(decoder.init(config));

    const auto pcmChunk = makeSine(440.0f, 960, 0.5f);
    AudioPacket packet;
    REQUIRE(encoder.encode(pcmChunk, packet));

    std::vector<float> pcmOut;

    BENCHMARK("decode 20ms Opus frame") {
        return decoder.decode(packet.span(), pcmOut);
    };
}

#endif // JOB_TEST_BENCHMARKS