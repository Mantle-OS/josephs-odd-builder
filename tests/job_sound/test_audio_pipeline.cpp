#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <numeric>
#include <vector>

#include <real_type.h>

#include <chorus_effect.h>
#include <equalizerbank.h>
#include <fft_analyzer.h>
#include <flanger_effect.h>
#include <overdrive_effect.h>
#include <virtual_eq_processor.h>

using Catch::Approx;
using namespace job::sound;

namespace {

constexpr float kSampleRate = 48000.0f;
constexpr float kPi = std::numbers::pi_v<float>;

[[nodiscard]] std::vector<float> makeSine(float frequency, std::size_t sampleCount, float amplitude = 1.0f, float sampleRate = kSampleRate) {
    std::vector<float> samples(sampleCount);
    for (std::size_t i = 0; i < sampleCount; ++i) {
        const float phase = 2.0f * kPi * frequency * static_cast<float>(i) / sampleRate;
        samples[i] = amplitude * std::sin(phase);
    }
    return samples;
}

[[nodiscard]] std::vector<float> makeTwoTone(float f1, float f2, std::size_t sampleCount, float a1 = 0.5f, float a2 = 0.5f, float sampleRate = kSampleRate) {
    std::vector<float> samples(sampleCount);
    for (std::size_t i = 0; i < sampleCount; ++i) {
        const float time = static_cast<float>(i) / sampleRate;
        samples[i] = a1 * std::sin(2.0f * kPi * f1 * time) + a2 * std::sin(2.0f * kPi * f2 * time);
    }
    return samples;
}

[[nodiscard]] bool allFinite(const std::vector<float>& samples) {
    return std::all_of(samples.begin(), samples.end(), [](float s) { return job::core::isSafeFinite(s); });
}

[[nodiscard]] float signalEnergy(const std::vector<float>& samples, std::size_t start = 0) {
    return std::accumulate(samples.begin() + start, samples.end(), 0.0f,
                           [](float sum, float s) { return sum + (s * s); });
}

[[nodiscard]] std::size_t dominantBin(const std::vector<float>& spectrum) {
    const auto it = std::max_element(spectrum.begin(), spectrum.end());
    return (it == spectrum.end()) ? 0 : static_cast<std::size_t>(std::distance(spectrum.begin(), it));
}

[[nodiscard]] float binFrequency(std::size_t bin, std::size_t fftSize, float sampleRate = kSampleRate) {
    return static_cast<float>(bin) * sampleRate / static_cast<float>(fftSize);
}

[[nodiscard]] std::vector<float> processEqualizer(EqualizerBank& equalizer, const std::vector<float>& input) {
    std::vector<float> output(input.size());
    equalizer.processBuffer(input.data(), output.data(), static_cast<int>(input.size()));
    return output;
}

template <typename Effect>
[[nodiscard]] std::vector<float> processEffect(Effect& effect, const std::vector<float>& input) {
    std::vector<float> output;
    output.reserve(input.size());
    for (const float sample : input) {
        output.push_back(effect.process(sample));
    }
    return output;
}

} // namespace

// ============================================================================
// Block 1: Usage / Examples
// ============================================================================

TEST_CASE("Generated tone survives a flat equalizer and remains detectable by FFT", "[sound][pipeline][eq][fft][usage]") {
    constexpr int fftSize = 1024;
    constexpr float frequency = 1000.0f;

    const auto input = makeSine(frequency, fftSize, 0.5f);
    EqualizerBank equalizer{kSampleRate};
    const auto output = processEqualizer(equalizer, input);

    REQUIRE(output.size() == input.size());
    REQUIRE(allFinite(output));

    FftAnalyzer analyzer{fftSize, FftAnalyzer::Hann};
    analyzer.setSampleRate(kSampleRate);
    const auto spectrum = analyzer.compute(output.data());

    REQUIRE(spectrum.size() == static_cast<std::size_t>(fftSize / 2));
    const float detectedFrequency = binFrequency(dominantBin(spectrum), fftSize);
    REQUIRE(detectedFrequency == Approx(frequency).margin(50.0f));
}

TEST_CASE("Equalizer boost increases energy near the selected band", "[sound][pipeline][eq][usage]") {
    constexpr std::size_t sampleCount = 48000;
    constexpr std::size_t settleSamples = 4096;

    const auto input = makeSine(1000.0f, sampleCount, 0.25f);
    EqualizerBank flat{kSampleRate};
    EqualizerBank boosted{kSampleRate};

    boosted.setGain(17, 6.0f); // 1000 Hz is band 17

    const float flatEnergy    = signalEnergy(processEqualizer(flat, input), settleSamples);
    const float boostedEnergy = signalEnergy(processEqualizer(boosted, input), settleSamples);

    REQUIRE(boostedEnergy > flatEnergy);
}

TEST_CASE("Overdrive changes waveform shape while preserving the dominant tone", "[sound][pipeline][overdrive][fft][usage]") {
    constexpr int fftSize = 2048;
    constexpr float frequency = 440.0f;

    const auto input = makeSine(frequency, fftSize, 0.75f);
    OverdriveEffect overdrive;
    overdrive.setGain(4.0f);
    overdrive.setLevel(0.8f);

    const auto output = processEffect(overdrive, input);
    REQUIRE(output.size() == input.size());
    REQUIRE(allFinite(output));

    // Nonlinear stage must alter the waveform
    const bool changed = std::any_of(input.begin(), input.end(), [&, i = 0](float in) mutable {
        return std::abs(in - output[i++]) > 0.001f;
    });
    REQUIRE(changed);

    FftAnalyzer analyzer{fftSize, FftAnalyzer::Hann};
    analyzer.setSampleRate(kSampleRate);
    const auto spectrum = analyzer.compute(output.data());

    // Fundamental tone remains dominant
    const float detectedFrequency = binFrequency(dominantBin(spectrum), fftSize);
    REQUIRE(detectedFrequency == Approx(frequency).margin(30.0f));
}

TEST_CASE("Chorus processes a complete tone buffer without losing signal", "[sound][pipeline][chorus][usage]") {
    const auto input = makeSine(440.0f, 48000, 0.25f);
    ChorusEffect chorus;
    chorus.setDepth(0.005f);
    chorus.setRate(0.8f);
    chorus.setMix(0.5f);

    const auto output = processEffect(chorus, input);
    REQUIRE(output.size() == input.size());
    REQUIRE(allFinite(output));
    REQUIRE(signalEnergy(output) > 0.0f);
}

TEST_CASE("Flanger processes a complete tone buffer without losing signal", "[sound][pipeline][flanger][usage]") {
    const auto input = makeSine(440.0f, 48000, 0.25f);
    FlangerEffect flanger;
    flanger.setDepth(0.002f);
    flanger.setRate(0.25f);
    flanger.setFeedback(0.3f);
    flanger.setMix(0.5f);

    const auto output = processEffect(flanger, input);
    REQUIRE(output.size() == input.size());
    REQUIRE(allFinite(output));
    REQUIRE(signalEnergy(output) > 0.0f);
}

TEST_CASE("Tone can pass through equalizer overdrive chorus and FFT analysis", "[sound][pipeline][full][usage]") {
    constexpr int fftSize = 4096;
    constexpr float frequency = 440.0f;

    const auto input = makeSine(frequency, fftSize, 0.2f);

    EqualizerBank equalizer{kSampleRate};
    equalizer.setGain(17, 2.0f);
    auto equalized = processEqualizer(equalizer, input);

    OverdriveEffect overdrive;
    overdrive.setGain(1.5f);
    overdrive.setLevel(0.8f);
    auto driven = processEffect(overdrive, equalized);

    ChorusEffect chorus;
    chorus.setMix(0.25f);
    auto finalOutput = processEffect(chorus, driven);

    REQUIRE(finalOutput.size() == input.size());
    REQUIRE(allFinite(finalOutput));
    REQUIRE(signalEnergy(finalOutput) > 0.0f);

    FftAnalyzer analyzer{fftSize, FftAnalyzer::Hann};
    analyzer.setSampleRate(kSampleRate);
    const auto spectrum = analyzer.compute(finalOutput.data());

    REQUIRE(spectrum.size() == static_cast<std::size_t>(fftSize / 2));
    REQUIRE(allFinite(spectrum));

    const float detectedFrequency = binFrequency(dominantBin(spectrum), fftSize);
    REQUIRE(detectedFrequency == Approx(frequency).margin(15.0f));
}

TEST_CASE("FFT distinguishes two tones in the same pipeline buffer", "[sound][pipeline][fft][multitone][usage]") {
    constexpr int fftSize = 4096;
    const auto input = makeTwoTone(440.0f, 1000.0f, fftSize, 0.75f, 0.25f);

    EqualizerBank equalizer{kSampleRate};
    const auto output = processEqualizer(equalizer, input);

    FftAnalyzer analyzer{fftSize, FftAnalyzer::BlackmanHarris};
    analyzer.setSampleRate(kSampleRate);
    const auto spectrum = analyzer.compute(output.data());

    REQUIRE(spectrum.size() == static_cast<std::size_t>(fftSize / 2));

    const float strongestFrequency = binFrequency(dominantBin(spectrum), fftSize);
    REQUIRE(strongestFrequency == Approx(440.0f).margin(15.0f));
}

TEST_CASE("VirtualEqProcessor processes generated audio through its public API", "[sound][pipeline][eq][processor][usage]") {
    auto* processor = VirtualEqProcessor::instance();
    REQUIRE(processor);

    const auto input = makeSine(1000.0f, 4096, 0.25f);
    processor->setBandGain(17, 3.0f);

    const auto output = processor->processBuffer(input);
    REQUIRE(output.size() == input.size());
    REQUIRE(allFinite(output));

    processor->setBandGain(17, 0.0f); // Reset singleton state
}

// ============================================================================
// Block 2: Edge Cases
// ============================================================================

TEST_CASE("Empty audio buffer survives the pipeline", "[sound][pipeline][edge]") {
    const std::vector<float> input;
    EqualizerBank equalizer{kSampleRate};
    std::vector<float> output;

    if (!input.empty()) {
        output.resize(input.size());
        equalizer.processBuffer(input.data(), output.data(), static_cast<int>(input.size()));
    }
    REQUIRE(output.empty());
}

TEST_CASE("Silent audio remains finite through the complete DSP pipeline", "[sound][pipeline][silence][edge]") {
    const std::vector<float> input(48000, 0.0f);

    EqualizerBank equalizer{kSampleRate};
    OverdriveEffect overdrive;
    ChorusEffect chorus;
    FlangerEffect flanger;

    auto equalized = processEqualizer(equalizer, input);
    auto driven    = processEffect(overdrive, equalized);
    auto chorused  = processEffect(chorus, driven);
    auto output    = processEffect(flanger, chorused);

    REQUIRE(output.size() == input.size());
    REQUIRE(allFinite(output));
    for (const float sample : output) {
        REQUIRE(sample == Approx(0.0f));
    }
}

TEST_CASE("Full DSP pipeline remains finite for sustained near-full-scale audio", "[sound][pipeline][stress][edge]") {
    const auto input = makeSine(1000.0f, 200000, 0.99f);

    EqualizerBank equalizer{kSampleRate};
    equalizer.setGain(17, 6.0f);

    OverdriveEffect overdrive;
    overdrive.setGain(8.0f);
    overdrive.setLevel(1.0f);

    ChorusEffect chorus;
    chorus.setMix(0.75f);

    FlangerEffect flanger;
    flanger.setFeedback(0.5f);
    flanger.setMix(0.5f);

    auto equalized = processEqualizer(equalizer, input);
    auto driven    = processEffect(overdrive, equalized);
    auto chorused  = processEffect(chorus, driven);
    auto output    = processEffect(flanger, chorused);

    REQUIRE(output.size() == input.size());
    REQUIRE(allFinite(output));
}

TEST_CASE("Pipeline preserves sample count through every DSP stage", "[sound][pipeline][buffer][edge]") {
    constexpr std::size_t sampleCount = 12345;
    const auto input = makeSine(523.25f, sampleCount, 0.25f);

    EqualizerBank equalizer{kSampleRate};
    OverdriveEffect overdrive;
    ChorusEffect chorus;
    FlangerEffect flanger;

    auto stageOne   = processEqualizer(equalizer, input);
    auto stageTwo   = processEffect(overdrive, stageOne);
    auto stageThree = processEffect(chorus, stageTwo);
    auto stageFour  = processEffect(flanger, stageThree);

    CHECK(stageOne.size() == sampleCount);
    CHECK(stageTwo.size() == sampleCount);
    CHECK(stageThree.size() == sampleCount);
    CHECK(stageFour.size() == sampleCount);
}

TEST_CASE("Pipeline handles a single sample", "[sound][pipeline][buffer][edge]") {
    const std::vector<float> input{0.5f};

    EqualizerBank equalizer{kSampleRate};
    OverdriveEffect overdrive;
    ChorusEffect chorus;
    FlangerEffect flanger;

    auto stageOne   = processEqualizer(equalizer, input);
    auto stageTwo   = processEffect(overdrive, stageOne);
    auto stageThree = processEffect(chorus, stageTwo);
    auto output     = processEffect(flanger, stageThree);

    REQUIRE(output.size() == 1);
    REQUIRE(job::core::isSafeFinite(output.front()));
}

// ============================================================================
// Block 3: Benchmarks
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Complete DSP pipeline benchmark", "[sound][pipeline][benchmark]") {
    const auto input = makeSine(440.0f, 48000, 0.5f);

    EqualizerBank equalizer{kSampleRate};
    equalizer.setGain(17, 3.0f);

    OverdriveEffect overdrive;
    overdrive.setGain(2.0f);
    overdrive.setLevel(0.8f);

    ChorusEffect chorus;
    chorus.setMix(0.35f);

    FlangerEffect flanger;
    flanger.setMix(0.25f);

    std::vector<float> output(input.size());

    BENCHMARK("process one second through EQ overdrive chorus and flanger") {
        for (std::size_t i = 0; i < input.size(); ++i) {
            float s = equalizer.processSample(input[i]);
            s = overdrive.process(s);
            s = chorus.process(s);
            s = flanger.process(s);
            output[i] = s;
        }
        return output.back();
    };
}

TEST_CASE("Complete DSP pipeline plus FFT benchmark", "[sound][pipeline][fft][benchmark]") {
    constexpr int fftSize = 4096;
    const auto input = makeSine(1000.0f, fftSize, 0.5f);

    EqualizerBank equalizer{kSampleRate};
    OverdriveEffect overdrive;
    ChorusEffect chorus;

    std::vector<float> processed(input.size());
    FftAnalyzer analyzer{fftSize, FftAnalyzer::Hann};
    analyzer.setSampleRate(kSampleRate);

    BENCHMARK("process DSP chain and analyze 4096 samples") {
        for (std::size_t i = 0; i < input.size(); ++i) {
            float s = equalizer.processSample(input[i]);
            s = overdrive.process(s);
            s = chorus.process(s);
            processed[i] = s;
        }
        return analyzer.compute(processed.data());
    };
}

#endif // JOB_TEST_BENCHMARKS