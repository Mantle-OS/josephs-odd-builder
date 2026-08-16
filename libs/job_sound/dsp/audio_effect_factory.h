#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "audio_effects.h"
#include "bitcrusher_effect.h"
#include "chorus_effect.h"
#include "compressor_effect.h"
#include "delay_effect.h"
#include "envelope_filter_effect.h"
#include "flanger_effect.h"
#include "tremolo_effect.h"
#include "noise_gate_effect.h"
#include "overdrive_effect.h"
#include "phaser_effect.h"
#include "reverb_effect.h"
#include "ring_modulator_effect.h"
#include "wah_effect.h"

#include "jobsound_export.h"
namespace job::sound {

class JOBSOUND_EXPORT AudioEffectFactory {
public:
    using CreatorFunc = std::function<std::unique_ptr<AudioEffect>()>;

    AudioEffectFactory();
    ~AudioEffectFactory() = default;

    AudioEffectFactory(const AudioEffectFactory&) = delete;
    AudioEffectFactory& operator=(const AudioEffectFactory&) = delete;
    AudioEffectFactory(AudioEffectFactory&&) noexcept = default;
    AudioEffectFactory& operator=(AudioEffectFactory&&) noexcept = default;

    // Singleton global factory access
    [[nodiscard]] static AudioEffectFactory& instance();

    // Create an effect instance by its unique identifier (e.g., "delay", "compressor", "overdrive")
    [[nodiscard]] std::unique_ptr<AudioEffect> createUnique(std::string_view uid) const;
    [[nodiscard]] std::shared_ptr<AudioEffect> createShared(std::string_view uid) const;

    // Template helpers for direct type-based instantiation
    template <typename T>
    [[nodiscard]] std::unique_ptr<T> create() const {
        return T::createUnique();
    }

    // Register a custom user effect
    bool registerEffect(std::string uid, CreatorFunc creator);

    // Unregister an effect by UID
    bool unregisterEffect(std::string_view uid);

    // Check if an effect UID is registered
    [[nodiscard]] bool hasEffect(std::string_view uid) const noexcept;

    // Get a list of all registered effect UIDs
    [[nodiscard]] std::vector<std::string> availableEffects() const;

private:
    void registerDefaults();

private:
    std::unordered_map<std::string, CreatorFunc> m_registry;
};

} // namespace job::sound