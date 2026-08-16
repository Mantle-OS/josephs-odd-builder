# JOB Sound Subsystem (`job_sound`)

`JosephsOddBuilder_Sound` (`job_sound`) is a modern, high-performance, real-time audio library engineered for low-latency acquisition, real-time DSP processing, tone generation, spectral analysis, and multi-format audio encoding/decoding.

---

## 1. Architectural Overview

The audio pipeline follows a deterministic sample-flow architecture:

```text
Acquisition / Input                     Processing Pipeline                        Output / Sinks
┌─────────────────────┐               ┌────────────────────────┐               ┌─────────────────────┐
│ ALSA Capture Thread │ ───┐          │  VirtualEqProcessor    │          ┌──► │ ALSA Playback Thread│
└─────────────────────┘    │          │  (31-Band Biquad EQ)   │          │    └─────────────────────┘
                           │          └───────────┬────────────┘          │
┌─────────────────────┐    │                      │                       │    ┌─────────────────────┐
│ PipeWire Stream In  │ ───┼──► Float ◄───────────┼──────────────► Float ─┼──► │ PipeWire Stream Out │
└─────────────────────┘    │    PCM               ▼                PCM    │    └─────────────────────┘
                           │    Buffer   ┌─────────────────┐       Buffer │
┌─────────────────────┐    │   [-1, 1]   │ OverdriveEffect │              │    ┌─────────────────────┐
│ Synth / Tone Source │ ───┤             └────────┬────────┘              ├──► │ FftAnalyzer (Hann/  │
└─────────────────────┘    │                      ▼                       │    │ Blackman-Harris)    │
                           │             ┌─────────────────┐              │    └─────────────────────┘
┌─────────────────────┐    │             │  Chorus /       │              │
│ Codec Decoders      │ ───┘             │  Flanger Effects│              │    ┌─────────────────────┐
│ (WAV / Opus / FLAC) │                  └────────┬────────┘              └──► │ Codec Encoders      │
└─────────────────────┘                           ▼                            │ (WAV / Opus / FLAC) │
                                         ┌─────────────────┐                   └─────────────────────┘
                                         │  Virtual Mixer  │
                                         │  Channel Strip  │
                                         └─────────────────┘

```

---

## 2. Core Modules

### 2.1. Hardware & Audio Server Backends (`backends/`)

* **ALSA (`backends/alsa/`)**:
* `AlsaDevice` / `AlsaSoundCard`: Enumerate, query, and configure hardware soundcards, PCM subdevices, sample formats, and hardware mixer controls.
* `AlsaPlaybackThread`: Lock-free, real-time audio playback loop feeding raw hardware or ALSA `default`/`dmix` sinks.
* `AlsaCaptureThread`: Low-latency microphone and line-in PCM capture worker.
* `AlsaMixer`: Hardware volume, mute, and capture gain control abstractions.


* **PipeWire & WirePlumber (`backends/pipewire/`)**:
* `PipewireManager`: Connects to the PipeWire graph registry, tracking global node events and server state.
* `PipewireStream`: Native PipeWire audio stream wrapper for zero-latency, graph-synchronized capture and playback.
* `PipewireLinkLayer` / `PipewireGraphAdapter`: Inspects graph topology, calculates Bezier link routing, and exposes programmatic node linking.
* `PipewireToneSource`: Direct PipeWire-driven procedural waveform generator.



### 2.2. Digital Signal Processing (`dsp/`)

All DSP stages operate on normalized single-precision floating point samples ($[-1.0\text{f}, 1.0\text{f}]$) at arbitrary sample rates (default $48.0\text{ kHz}$).

* **`EqualizerBank` / `BiquadEq31Band**`:
* Complete 31-band ISO standard $(20\text{ Hz} - 20\text{ kHz})$ $1/3$-octave graphic equalizer.
* Direct Form II Transposed biquad filters ensuring minimal numerical roundoff and high SIMD cache locality.
* Support for per-band gain adjustment $(\pm 12\text{ dB})$ with real-time parameter smoothing.


* **`VirtualEqProcessor`**:
* Thread-safe singleton processor exposing clean buffer-level transformations for GUI and app pipelines.


* **`AudioEffects`**:
* `OverdriveEffect`: Nonlinear soft-clipping waveshaping with dynamic pre-gain and master level control.
* `ChorusEffect`: Modulated short delay lines with LFO rate, depth, and dry/wet mix parameters.
* `FlangerEffect`: Comb-filtering delay line with feedback regeneration and phase sweep.


* **`FftAnalyzer`**:
* Spectral analysis engine supporting configurable FFT sizes ($512$ to $8192$ points).
* Windowing functions: `Hann`, `Hamming`, `BlackmanHarris`, `Rectangular`.
* Computes magnitude spectrum, peak frequency identification, and dominant bin estimation.



### 2.3. Codecs Subsystem (`codecs/`)

Uniform, polymorphic interfaces for audio compression, file serialization, and container formatting.

* **Abstract Base Classes**:
* `AudioCodecConfig`: Standardized configuration struct specifying sample rate, channels, bit depth, bitrates, and nominal frame sizes.
* `AudioPacket`: Value-type container holding compressed byte payloads, presentation timestamps (`pts`), duration, and keyframe metadata.
* `AudioEncoder`: Base class defining `init()`, `encode(std::span<const float>, AudioPacket&)`, and `flush()`.
* `AudioDecoder`: Base class defining `init()`, `decode(std::span<const uint8_t>, std::vector<float>&)`, and `flush()`.


* **Implementations**:
* `WavReader` / `WavWriter`: Fast, zero-dependency streaming and one-shot RIFF/WAVE parser and serializer (supports $16$-bit, $24$-bit, $32$-bit Integer PCM, and $32$-bit IEEE Float).
* `OpusAudioEncoder` / `OpusAudioDecoder`: Low-latency $48\text{ kHz}$ Opus compression ($2.5\text{ ms} - 60\text{ ms}$ frames, dynamic VBR/bitrate adjustments, and Packet Loss Concealment (PLC)).
* `FlacAudioEncoder` / `FlacAudioDecoder`: Lossless integer FLAC compression with automatic stream metadata serialization and flush semantics.



---

## 3. Usage Examples

### 3.1. Encoding and Decoding with Opus

```cpp
#include <codecs/opus/opus_encoder.h>
#include <codecs/opus/opus_decoder.h>

using namespace job::sound;

// 1. Configure for 20ms @ 48kHz mono
AudioCodecConfig config{
    .codecType  = AudioCodecType::Opus,
    .sampleRate = 48000,
    .channels   = 1,
    .bitrate    = 64000,
    .frameSize  = 960
};

OpusAudioEncoder encoder;
OpusAudioDecoder decoder;
encoder.init(config);
decoder.init(config);

// 2. Encode 960 samples
std::vector<float> inputPcm(960, 0.5f);
AudioPacket packet;
if (encoder.encode(inputPcm, packet)) {
    // packet.data contains compressed Opus payload
    // packet.pts tracks timeline sample count
}

// 3. Decode back to PCM
std::vector<float> outputPcm;
if (decoder.decode(packet.span(), outputPcm)) {
    // outputPcm contains 960 decoded PCM float samples
}

```

### 3.2. Chaining Real-Time DSP Effects

```cpp
#include <dsp/equalizerbank.h>
#include <dsp/overdrive_effect.h>
#include <dsp/chorus_effect.h>

using namespace job::sound;

EqualizerBank eq{48000.0f};
eq.setGain(17, 3.0f); // Boost 1 kHz band (+3 dB)

OverdriveEffect drive;
drive.setGain(2.5f);
drive.setLevel(0.8f);

ChorusEffect chorus;
chorus.setMix(0.35f);

// Process audio buffer in-place
void processAudioBlock(std::span<float> buffer) {
    for (float& sample : buffer) {
        sample = eq.processSample(sample);
        sample = drive.process(sample);
        sample = chorus.process(sample);
    }
}

```

---


