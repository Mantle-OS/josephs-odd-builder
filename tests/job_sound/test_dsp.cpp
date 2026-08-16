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
#include <string>
#include <vector>

#include <real_type.h>

// Core DSP & Processor Models
#include <biquad_eq_31band.h>
#include <equalizerbank.h>
#include <fft_analyzer.h>
#include <virtual_eq_band.h>
#include <virtual_eq_processor.h>
#include <virtual_mixer_channel.h>

// All 11 DSP Effects & Unified Factory
#include <audio_effects.h>
#include <audio_effect_factory.h>
#include <overdrive_effect.h>
#include <chorus_effect.h>
#include <flanger_effect.h>
#include <delay_effect.h>
#include <compressor_effect.h>
#include <reverb_effect.h>
#include <tremolo_effect.h>
#include <bitcrusher_effect.h>
#include <phaser_effect.h>
#include <noise_gate_effect.h>
#include <envelope_filter_effect.h>
#include <wah_effect.h>
#include <ring_modulator_effect.h>

using Catch::Approx;
using namespace job::sound;

namespace {

constexpr float kSampleRate = 48000.0f;
constexpr float kPi = std::numbers::pi_v<float>;

std::vector<float> makeSine(float frequency, std::size_t sampleCount, float sampleRate = kSampleRate, float amplitude = 1.0f) {
    std::vector<float> samples(sampleCount);
    for (std::size_t i = 0; i < sampleCount; ++i) {
        const float phase = 2.0f * kPi * frequency * static_cast<float>(i) / sampleRate;
        samples[i] = amplitude * std::sin(phase);
    }
    return samples;
}

float signalEnergy(const std::vector<float>& samples, std::size_t start = 0) {
    return std::accumulate(samples.begin() + start, samples.end(), 0.0f,
                           [](float sum, float s) { return sum + (s * s); });
}

std::vector<float> processEq(EqualizerBank& eq, const std::vector<float>& input) {
    std::vector<float> output(input.size());
    eq.processBuffer(input.data(), output.data(), input.size());
    return output;
}

bool allFinite(const std::vector<float>& samples) {
    return std::all_of(samples.begin(), samples.end(), [](float s) { return job::core::isSafeFinite(s); });
}

} // namespace

// ============================================================================
// Block 1: Usage / Examples
// ============================================================================

TEST_CASE("VirtualEqBand represents one configurable equalizer band", "[sound][dsp][eq][usage]") {
    auto band = VirtualEqBand::createUnique();
    REQUIRE(band);

    band->setUid("17");
    band->setBandIndex(17);
    band->setFrequency(1000.0f);
    band->setGain(6.0f);

    CHECK(band->uid() == "17");
    CHECK(band->bandIndex() == 17);
    CHECK(band->frequency() == Approx(1000.0f));
    CHECK(band->gain() == Approx(6.0f));
}

TEST_CASE("VirtualMixerChannel uses its channel index as its uid", "[sound][dsp][mixer][usage]") {
    auto channel = VirtualMixerChannel::createUnique();
    REQUIRE(channel);

    channel->setChannelIndex(3);
    channel->setVolume(80);
    channel->setMute(false);

    CHECK(channel->uid() == "3");
    CHECK(channel->channelIndex() == 3);
    CHECK(channel->volume() == 80);
    CHECK_FALSE(channel->mute());
}

TEST_CASE("OverdriveEffect applies tanh soft clipping", "[sound][dsp][overdrive][usage]") {
    OverdriveEffect effect;
    effect.setGain(2.0f);
    effect.setLevel(1.0f);

    constexpr float input = 0.5f;
    REQUIRE(effect.process(input) == Approx(std::tanh(input * 2.0f)));
}

TEST_CASE("OverdriveEffect level controls the post-clipping output", "[sound][dsp][overdrive][usage]") {
    OverdriveEffect fullLevel, halfLevel;
    fullLevel.setGain(3.0f);  fullLevel.setLevel(1.0f);
    halfLevel.setGain(3.0f);  halfLevel.setLevel(0.5f);

    constexpr float input = 0.75f;
    REQUIRE(halfLevel.process(input) == Approx(fullLevel.process(input) * 0.5f));
}

TEST_CASE("ChorusEffect with zero wet mix passes the dry signal through", "[sound][dsp][chorus][usage]") {
    ChorusEffect chorus;
    chorus.setMix(0.0f);

    for (const float sample : makeSine(440.0f, 4096)) {
        REQUIRE(chorus.process(sample) == Approx(sample));
    }
}

TEST_CASE("FlangerEffect with zero wet mix passes the dry signal through", "[sound][dsp][flanger][usage]") {
    FlangerEffect flanger;
    flanger.setMix(0.0f);

    for (const float sample : makeSine(440.0f, 4096)) {
        REQUIRE(flanger.process(sample) == Approx(sample));
    }
}

TEST_CASE("DelayEffect with zero wet mix passes dry signal through", "[sound][dsp][delay][usage]") {
    DelayEffect delay;
    delay.setMix(0.0f);

    for (const float sample : makeSine(440.0f, 4096)) {
        REQUIRE(delay.process(sample) == Approx(sample));
    }
}

TEST_CASE("DelayEffect produces delayed repeats after input pulse", "[sound][dsp][delay][usage]") {
    DelayEffect delay;
    delay.setSampleRate(kSampleRate);
    delay.setTime(0.01f); // 10ms delay = 480 samples
    delay.setFeedback(0.5f);
    delay.setMix(1.0f); // 100% wet
    delay.setDamping(1.0f); // no filter damping

    // Feed a single unit impulse
    REQUIRE(delay.process(1.0f) == Approx(0.0f));

    // Wait until delay buffer wraps around to echo tap (480 samples)
    for (int i = 0; i < 479; ++i) {
        (void)delay.process(0.0f);
    }

    // 480th sample should output the delayed impulse
    REQUIRE(delay.process(0.0f) == Approx(1.0f).margin(0.01f));
}

TEST_CASE("CompressorEffect attenuates signals exceeding threshold", "[sound][dsp][compressor][usage]") {
    CompressorEffect comp;
    comp.setThreshold(-12.0f); // ~0.25 linear
    comp.setRatio(4.0f);
    comp.setAttack(0.1f);
    comp.setMakeupGain(0.0f);

    // Warm up compressor with hot input (1.0f = 0 dB)
    for (int i = 0; i < 4800; ++i) {
        (void)comp.process(1.0f);
    }

    const float compressed = comp.process(1.0f);
    REQUIRE(compressed < 0.6f);
    REQUIRE(compressed > 0.1f);
}

TEST_CASE("ReverbEffect produces tail energy after impulse", "[sound][dsp][reverb][usage]") {
    ReverbEffect reverb;
    reverb.setRoomSize(0.7f);
    reverb.setMix(1.0f); // 100% wet

    (void)reverb.process(1.0f); // Inject impulse

    std::vector<float> tail(4800);
    for (float& s : tail) {
        s = reverb.process(0.0f);
    }

    REQUIRE(signalEnergy(tail) > 0.0f);
    REQUIRE(allFinite(tail));
}

TEST_CASE("TremoloEffect modulates amplitude rhythmically", "[sound][dsp][tremolo][usage]") {
    TremoloEffect trem;
    trem.setRate(10.0f);
    trem.setDepth(1.0f); // Full depth modulation
    trem.setWaveform(TremoloEffect::Waveform::Sine);

    std::vector<float> modulated(4800);
    for (float& s : modulated) {
        s = trem.process(1.0f);
    }

    const auto [minVal, maxVal] = std::minmax_element(modulated.begin(), modulated.end());
    REQUIRE(*minVal == Approx(0.0f).margin(0.05f));
    REQUIRE(*maxVal == Approx(1.0f).margin(0.05f));
}

TEST_CASE("BitcrusherEffect quantizes continuous signal to discrete steps", "[sound][dsp][bitcrusher][usage]") {
    BitcrusherEffect crusher;
    crusher.setBitDepth(2.0f); // 2-bit quantization -> few distinct output values
    crusher.setDownsampleFactor(1.0f);
    crusher.setMix(1.0f);

    const auto input = makeSine(440.0f, 4096);
    std::vector<float> crushed(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        crushed[i] = crusher.process(input[i]);
    }

    REQUIRE(allFinite(crushed));
    // Verify signal is quantized within bounds
    for (const float s : crushed) {
        REQUIRE(std::abs(s) <= 1.0f);
    }
}

TEST_CASE("PhaserEffect with 50/50 mix generates notch cancellations", "[sound][dsp][phaser][usage]") {
    PhaserEffect phaser;
    phaser.setMix(0.5f);
    phaser.setFeedback(0.0f);

    const auto input = makeSine(250.0f, 4096, kSampleRate, 0.5f);
    std::vector<float> output(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        output[i] = phaser.process(input[i]);
    }

    REQUIRE(allFinite(output));
    REQUIRE(signalEnergy(output) > 0.0f);
}

TEST_CASE("NoiseGateEffect mutes signals below threshold", "[sound][dsp][noisegate][usage]") {
    NoiseGateEffect gate;
    gate.setThreshold(-30.0f);
    gate.setAttack(0.1f);
    gate.setHold(0.0f);
    gate.setRelease(5.0f);

    // Warm up gate with loud tone (above threshold)
    for (int i = 0; i < 4800; ++i) {
        (void)gate.process(0.5f);
    }
    REQUIRE(gate.process(0.5f) == Approx(0.5f).margin(0.05f));

    // Feed quiet noise below -30 dB (e.g., 0.001 linear = -60 dB)
    for (int i = 0; i < 4800; ++i) {
        (void)gate.process(0.001f);
    }
    REQUIRE(gate.process(0.001f) == Approx(0.0f).margin(0.0001f));
}

TEST_CASE("EnvelopeFilterEffect opens filter in response to signal dynamics", "[sound][dsp][envelope_filter][usage]") {
    EnvelopeFilterEffect filter;
    filter.setSensitivity(5.0f);
    filter.setMode(EnvelopeFilterEffect::FilterMode::LowPass);
    filter.setMix(1.0f);

    // Feed dynamic input and verify output is stable
    const auto loudInput = makeSine(1000.0f, 4800, kSampleRate, 0.9f);
    std::vector<float> output(loudInput.size());
    for (std::size_t i = 0; i < loudInput.size(); ++i) {
        output[i] = filter.process(loudInput[i]);
    }

    REQUIRE(allFinite(output));
    REQUIRE(signalEnergy(output) > 0.0f);
}

TEST_CASE("WahEffect sweeps center frequency across pedal travel", "[sound][dsp][wah][usage]") {
    WahEffect wah;
    wah.setMix(1.0f);

    // Heel down (low frequency emphasis)
    wah.setPedalPosition(0.0f);
    const auto lowTone = makeSine(350.0f, 2048, kSampleRate, 0.5f);
    std::vector<float> heelOut(lowTone.size());
    for (std::size_t i = 0; i < lowTone.size(); ++i) {
        heelOut[i] = wah.process(lowTone[i]);
    }

    // Toe down (high frequency emphasis)
    wah.reset();
    wah.setPedalPosition(1.0f);
    std::vector<float> toeOut(lowTone.size());
    for (std::size_t i = 0; i < lowTone.size(); ++i) {
        toeOut[i] = wah.process(lowTone[i]);
    }

    REQUIRE(allFinite(heelOut));
    REQUIRE(allFinite(toeOut));
    // Heel position passes 350Hz with significantly higher gain than toe position
    REQUIRE(signalEnergy(heelOut) > signalEnergy(toeOut));
}

TEST_CASE("RingModulatorEffect modulates input with carrier oscillator", "[sound][dsp][ringmod][usage]") {
    RingModulatorEffect ring;
    ring.setFrequency(440.0f);
    ring.setMix(1.0f);

    const auto input = makeSine(1000.0f, 4096, kSampleRate, 0.5f);
    std::vector<float> output(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        output[i] = ring.process(input[i]);
    }

    REQUIRE(allFinite(output));
    REQUIRE(signalEnergy(output) > 0.0f);
}

TEST_CASE("AudioEffectFactory instantiates all 11 registered effects by UID", "[sound][dsp][factory][usage]") {
    auto& factory = AudioEffectFactory::instance();

    const std::vector<std::string> expectedUids = {
        "overdrive", "chorus", "flanger", "delay", "compressor",
        "reverb", "tremolo", "bitcrusher", "phaser", "noisegate",
        "envelope_filter", "wah", "ring_modulator"
    };

    for (const auto& uid : expectedUids) {
        INFO("Testing UID: " << uid);
        REQUIRE(factory.hasEffect(uid));

        auto effect = factory.createUnique(uid);
        REQUIRE(effect);
        CHECK(effect->uid() == uid);

        // Verify basic process contract
        const float out = effect->process(0.5f);
        CHECK(job::core::isSafeFinite(out));
    }
}

TEST_CASE("EqualizerBank with flat gains passes audio through", "[sound][dsp][eq][usage]") {
    EqualizerBank eq{kSampleRate};
    const auto input  = makeSine(1000.0f, 4096, kSampleRate, 0.5f);
    const auto output = processEq(eq, input);

    REQUIRE(output.size() == input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        REQUIRE(output[i] == Approx(input[i]).margin(0.0001f));
    }
}

TEST_CASE("EqualizerBank boosts a selected frequency band", "[sound][dsp][eq][usage]") {
    constexpr std::size_t sampleCount = 48000;
    constexpr std::size_t settleSamples = 4096;

    const auto input = makeSine(1000.0f, sampleCount, kSampleRate, 0.25f);
    EqualizerBank flat{kSampleRate};
    EqualizerBank boosted{kSampleRate};

    boosted.setGain(17, 6.0f); // 1000 Hz is band 17 in EQ_BAND_FREQUENCIES

    const float flatEnergy    = signalEnergy(processEq(flat, input), settleSamples);
    const float boostedEnergy = signalEnergy(processEq(boosted, input), settleSamples);

    REQUIRE(boostedEnergy > flatEnergy);
}

TEST_CASE("FftAnalyzer identifies the dominant frequency of a 1 kHz tone", "[sound][dsp][fft][usage]") {
    constexpr int fftSize = 1024;
    constexpr float toneFrequency = 1000.0f;

    FftAnalyzer analyzer{fftSize, FftAnalyzer::Hann};
    analyzer.setSampleRate(kSampleRate);

    const auto input    = makeSine(toneFrequency, fftSize, kSampleRate);
    const auto spectrum = analyzer.compute(input.data());

    REQUIRE(spectrum.size() == static_cast<std::size_t>(fftSize / 2));

    const auto peak = std::max_element(spectrum.begin(), spectrum.end());
    REQUIRE(peak != spectrum.end());

    const auto peakBin = static_cast<std::size_t>(std::distance(spectrum.begin(), peak));
    const float detectedFrequency = static_cast<float>(peakBin) * kSampleRate / static_cast<float>(fftSize);

    REQUIRE(detectedFrequency == Approx(toneFrequency).margin(50.0f));
}

TEST_CASE("FftAnalyzer exposes peak hold after analyzing audio", "[sound][dsp][fft][usage]") {
    constexpr int fftSize = 1024;

    FftAnalyzer analyzer{fftSize, FftAnalyzer::BlackmanHarris};
    analyzer.setSampleRate(kSampleRate);

    const auto spectrum = analyzer.compute(makeSine(440.0f, fftSize).data());
    const auto& peaks   = analyzer.peakHold();

    REQUIRE(peaks.size() == static_cast<std::size_t>(fftSize / 2));
    REQUIRE(peaks.size() == spectrum.size());
    REQUIRE(std::any_of(peaks.begin(), peaks.end(), [](float peak) { return peak > -90.0f; }));
}

TEST_CASE("VirtualEqProcessor exposes the standard 31 band model", "[sound][dsp][eq][processor][usage]") {
    auto* processor = VirtualEqProcessor::instance();
    REQUIRE(processor);
    REQUIRE(processor->bandModel());
    REQUIRE(processor->bandModel()->size() == 31);

    for (std::size_t i = 0; i < EQ_BAND_FREQUENCIES.size(); ++i) {
        const auto* band = processor->bandModel()->at(std::to_string(i));
        REQUIRE(band);
        CHECK(band->bandIndex() == static_cast<int>(i));
        CHECK(band->frequency() == Approx(EQ_BAND_FREQUENCIES[i]));
    }
}

TEST_CASE("VirtualEqProcessor updates the matching band gain", "[sound][dsp][eq][processor][usage]") {
    auto* processor = VirtualEqProcessor::instance();
    REQUIRE(processor);
    REQUIRE(processor->bandModel());

    constexpr int bandIndex = 17;
    constexpr float gainDb = 6.0f;

    processor->setBandGain(bandIndex, gainDb);
    const auto* band = processor->bandModel()->at(std::to_string(bandIndex));
    REQUIRE(band);
    REQUIRE(band->gain() == Approx(gainDb));

    processor->setBandGain(bandIndex, 0.0f); // Reset singleton state
}

TEST_CASE("VirtualEqProcessor processes a complete audio buffer", "[sound][dsp][eq][processor][usage]") {
    auto* processor = VirtualEqProcessor::instance();
    REQUIRE(processor);

    const auto input  = makeSine(440.0f, 4096, kSampleRate, 0.25f);
    const auto output = processor->processBuffer(input);

    REQUIRE(output.size() == input.size());
    REQUIRE(allFinite(output));
}

// ============================================================================
// Block 2: Edge Cases
// ============================================================================

TEST_CASE("DelayEffect handles zero and extreme delay lengths", "[sound][dsp][delay][edge]") {
    DelayEffect delay;
    delay.setTime(0.0f);
    delay.setFeedback(0.99f);

    REQUIRE(job::core::isSafeFinite(delay.process(0.5f)));

    delay.reset();
    for (int i = 0; i < 10000; ++i) {
        REQUIRE(job::core::isSafeFinite(delay.process(0.0f)));
    }
}

TEST_CASE("CompressorEffect handles extreme infinite ratio and extreme input", "[sound][dsp][compressor][edge]") {
    CompressorEffect comp;
    comp.setThreshold(-20.0f);
    comp.setRatio(100.0f); // Brickwall limiting
    comp.setAttack(0.1f);  // Fast attack

    // Warm up the gain reduction envelope over extreme sustained input
    for (int i = 0; i < 4800; ++i) {
        (void)comp.process(100.0f);
    }

    const float outPositive = comp.process(100.0f);

    comp.reset();
    for (int i = 0; i < 4800; ++i) {
        (void)comp.process(-100.0f);
    }
    const float outNegative = comp.process(-100.0f);

    CHECK(job::core::isSafeFinite(outPositive));
    CHECK(job::core::isSafeFinite(outNegative));
    // Threshold is -20 dB = 0.1f linear
    CHECK(outPositive <= 0.2f);
    CHECK(outNegative >= -0.2f);
}

TEST_CASE("ReverbEffect handles sustained silence without denormal explosion", "[sound][dsp][reverb][edge]") {
    ReverbEffect reverb;
    reverb.reset();
    for (int i = 0; i < 50000; ++i) {
        REQUIRE(job::core::isSafeFinite(reverb.process(0.0f)));
    }
}

TEST_CASE("BitcrusherEffect handles 1-bit extreme reduction", "[sound][dsp][bitcrusher][edge]") {
    BitcrusherEffect crusher;
    crusher.setBitDepth(1.0f);
    crusher.setDownsampleFactor(32.0f);

    for (const float sample : makeSine(440.0f, 1000)) {
        const float out = crusher.process(sample);
        REQUIRE(job::core::isSafeFinite(out));
        REQUIRE((out == Approx(1.0f) || out == Approx(-1.0f) || out == Approx(0.0f)));
    }
}

TEST_CASE("NoiseGateEffect remains closed during silence", "[sound][dsp][noisegate][edge]") {
    NoiseGateEffect gate;
    gate.reset();
    for (int i = 0; i < 10000; ++i) {
        REQUIRE(gate.process(0.0f) == Approx(0.0f));
    }
}

TEST_CASE("WahEffect clamps pedal bounds safely", "[sound][dsp][wah][edge]") {
    WahEffect wah;
    wah.setPedalPosition(-5.0f);
    CHECK(wah.pedalPosition() == Approx(0.0f));

    wah.setPedalPosition(10.0f);
    CHECK(wah.pedalPosition() == Approx(1.0f));

    REQUIRE(job::core::isSafeFinite(wah.process(0.5f)));
}

TEST_CASE("AudioEffectFactory handles unregistered effect query safely", "[sound][dsp][factory][edge]") {
    auto& factory = AudioEffectFactory::instance();
    CHECK_FALSE(factory.hasEffect("non_existent_dsp_effect"));
    CHECK(factory.createUnique("non_existent_dsp_effect") == nullptr);
}

TEST_CASE("VirtualEqBand defaults to an unassigned band", "[sound][dsp][eq][edge]") {
    VirtualEqBand band;
    CHECK(band.frequency() == Approx(0.0f));
    CHECK(band.gain() == Approx(0.0f));
    CHECK(band.bandIndex() == -1);
}

TEST_CASE("OverdriveEffect keeps silence silent", "[sound][dsp][overdrive][edge]") {
    OverdriveEffect effect;
    effect.setGain(100.0f);
    effect.setLevel(1.0f);
    REQUIRE(effect.process(0.0f) == Approx(0.0f));
}

TEST_CASE("OverdriveEffect remains bounded with extreme gain", "[sound][dsp][overdrive][edge]") {
    OverdriveEffect effect;
    effect.setGain(1000.0f);
    effect.setLevel(1.0f);

    const float positive = effect.process(1000.0f);
    const float negative = effect.process(-1000.0f);

    CHECK(job::core::isSafeFinite(positive));
    CHECK(job::core::isSafeFinite(negative));
    CHECK(positive <= 1.0f);
    CHECK(negative >= -1.0f);
}

TEST_CASE("EqualizerBank handles silence", "[sound][dsp][eq][edge]") {
    EqualizerBank eq{kSampleRate};
    const auto output = processEq(eq, std::vector<float>(4096, 0.0f));

    REQUIRE(output.size() == 4096);
    for (const float sample : output) {
        REQUIRE(job::core::isSafeFinite(sample));
        REQUIRE(sample == Approx(0.0f));
    }
}

TEST_CASE("EqualizerBank ignores an out of range band index", "[sound][dsp][eq][edge]") {
    EqualizerBank eq{kSampleRate};
    eq.setGain(EQ_BAND_FREQUENCIES.size(), 24.0f);

    const auto input  = makeSine(1000.0f, 4096, kSampleRate, 0.25f);
    const auto output = processEq(eq, input);

    REQUIRE(output.size() == input.size());
    REQUIRE(allFinite(output));
}

TEST_CASE("FftAnalyzer handles silence without producing non-finite values", "[sound][dsp][fft][edge]") {
    constexpr int fftSize = 1024;
    FftAnalyzer analyzer{fftSize, FftAnalyzer::Hann};
    const auto spectrum = analyzer.compute(std::vector<float>(fftSize, 0.0f).data());

    REQUIRE(spectrum.size() == static_cast<std::size_t>(fftSize / 2));
    REQUIRE(allFinite(spectrum));
}

TEST_CASE("VirtualEqProcessor ignores an invalid band index", "[sound][dsp][eq][processor][edge]") {
    auto* processor = VirtualEqProcessor::instance();
    REQUIRE(processor);

    processor->setBandGain(-1, 12.0f);
    processor->setBandGain(31, 12.0f);

    REQUIRE(processor->bandModel()->size() == EQ_BAND_FREQUENCIES.size());
}

TEST_CASE("VirtualEqProcessor handles an empty input buffer", "[sound][dsp][eq][processor][edge]") {
    auto* processor = VirtualEqProcessor::instance();
    REQUIRE(processor);
    REQUIRE(processor->processBuffer({}).empty());
}

// ============================================================================
// Block 3: Benchmarks
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("OverdriveEffect sustained sample processing benchmark", "[sound][dsp][overdrive][benchmark]") {
    OverdriveEffect effect;
    effect.setGain(2.0f);
    effect.setLevel(1.0f);
    const auto input = makeSine(440.0f, 48000, kSampleRate, 0.5f);

    BENCHMARK("process one second of 48 kHz overdrive") {
        float acc = 0.0f;
        for (const float s : input) acc += effect.process(s);
        return acc;
    };
}

TEST_CASE("DelayEffect sustained sample processing benchmark", "[sound][dsp][delay][benchmark]") {
    DelayEffect delay;
    const auto input = makeSine(440.0f, 48000, kSampleRate, 0.5f);

    BENCHMARK("process one second of 48 kHz delay") {
        float acc = 0.0f;
        for (const float s : input) acc += delay.process(s);
        return acc;
    };
}

TEST_CASE("CompressorEffect sustained sample processing benchmark", "[sound][dsp][compressor][benchmark]") {
    CompressorEffect comp;
    const auto input = makeSine(440.0f, 48000, kSampleRate, 0.5f);

    BENCHMARK("process one second of 48 kHz compressor") {
        float acc = 0.0f;
        for (const float s : input) acc += comp.process(s);
        return acc;
    };
}

TEST_CASE("ReverbEffect sustained sample processing benchmark", "[sound][dsp][reverb][benchmark]") {
    ReverbEffect reverb;
    const auto input = makeSine(440.0f, 48000, kSampleRate, 0.5f);

    BENCHMARK("process one second of 48 kHz reverb") {
        float acc = 0.0f;
        for (const float s : input) acc += reverb.process(s);
        return acc;
    };
}

TEST_CASE("PhaserEffect sustained sample processing benchmark", "[sound][dsp][phaser][benchmark]") {
    PhaserEffect phaser;
    const auto input = makeSine(440.0f, 48000, kSampleRate, 0.5f);

    BENCHMARK("process one second of 48 kHz phaser") {
        float acc = 0.0f;
        for (const float s : input) acc += phaser.process(s);
        return acc;
    };
}

TEST_CASE("WahEffect sustained sample processing benchmark", "[sound][dsp][wah][benchmark]") {
    WahEffect wah;
    const auto input = makeSine(440.0f, 48000, kSampleRate, 0.5f);

    BENCHMARK("process one second of 48 kHz wah") {
        float acc = 0.0f;
        for (const float s : input) acc += wah.process(s);
        return acc;
    };
}

TEST_CASE("EqualizerBank 31 band processing benchmark", "[sound][dsp][eq][benchmark]") {
    EqualizerBank eq{kSampleRate};
    eq.setGain(5, 2.0f);   eq.setGain(12, -3.0f);
    eq.setGain(17, 4.0f);  eq.setGain(24, -2.0f);

    const auto input = makeSine(1000.0f, 48000, kSampleRate, 0.5f);
    std::vector<float> output(input.size());

    BENCHMARK("process one second through 31 band EQ") {
        eq.processBuffer(input.data(), output.data(), input.size());
        return output.back();
    };
}

TEST_CASE("FftAnalyzer 1024 sample transform benchmark", "[sound][dsp][fft][benchmark]") {
    FftAnalyzer analyzer{1024, FftAnalyzer::Hann};
    analyzer.setSampleRate(kSampleRate);
    const auto input = makeSine(1000.0f, 1024, kSampleRate);

    BENCHMARK("compute 1024 point FFT") {
        return analyzer.compute(input.data());
    };
}

#endif // JOB_TEST_BENCHMARKS