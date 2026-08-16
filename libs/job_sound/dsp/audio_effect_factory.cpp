#include "audio_effect_factory.h"

#include <mutex>

namespace job::sound {

namespace {
std::mutex g_factoryMutex;
}

AudioEffectFactory::AudioEffectFactory() {
    registerDefaults();
}

AudioEffectFactory& AudioEffectFactory::instance() {
    static AudioEffectFactory s_factory;
    return s_factory;
}

void AudioEffectFactory::registerDefaults() {
    registerEffect("overdrive",       [] { return OverdriveEffect::createUnique(); });
    registerEffect("chorus",          [] { return ChorusEffect::createUnique(); });
    registerEffect("flanger",         [] { return FlangerEffect::createUnique(); });
    registerEffect("delay",           [] { return DelayEffect::createUnique(); });
    registerEffect("compressor",      [] { return CompressorEffect::createUnique(); });
    registerEffect("reverb",          [] { return ReverbEffect::createUnique(); });
    registerEffect("tremolo",         [] { return TremoloEffect::createUnique(); });
    registerEffect("bitcrusher",      [] { return BitcrusherEffect::createUnique(); });
    registerEffect("phaser",          [] { return PhaserEffect::createUnique(); });
    registerEffect("noisegate",       [] { return NoiseGateEffect::createUnique(); });
    registerEffect("envelope_filter", [] { return EnvelopeFilterEffect::createUnique(); });
    registerEffect("wah",             [] { return WahEffect::createUnique(); });
    registerEffect("ring_modulator",  [] { return RingModulatorEffect::createUnique(); });
}

bool AudioEffectFactory::registerEffect(std::string uid, CreatorFunc creator) {
    std::lock_guard<std::mutex> lock(g_factoryMutex);
    if (uid.empty() || !creator) return false;

    m_registry[std::move(uid)] = std::move(creator);
    return true;
}

bool AudioEffectFactory::unregisterEffect(std::string_view uid) {
    std::lock_guard<std::mutex> lock(g_factoryMutex);
    const auto it = m_registry.find(std::string(uid));
    if (it != m_registry.end()) {
        m_registry.erase(it);
        return true;
    }
    return false;
}

bool AudioEffectFactory::hasEffect(std::string_view uid) const noexcept {
    std::lock_guard<std::mutex> lock(g_factoryMutex);
    return m_registry.find(std::string(uid)) != m_registry.end();
}

std::unique_ptr<AudioEffect> AudioEffectFactory::createUnique(std::string_view uid) const {
    std::lock_guard<std::mutex> lock(g_factoryMutex);
    const auto it = m_registry.find(std::string(uid));
    if (it != m_registry.end() && it->second) {
        return it->second();
    }
    return nullptr;
}

std::shared_ptr<AudioEffect> AudioEffectFactory::createShared(std::string_view uid) const {
    return createUnique(uid);
}

std::vector<std::string> AudioEffectFactory::availableEffects() const {
    std::lock_guard<std::mutex> lock(g_factoryMutex);
    std::vector<std::string> uids;
    uids.reserve(m_registry.size());
    for (const auto& [uid, _] : m_registry) {
        uids.push_back(uid);
    }
    return uids;
}

} // namespace job::sound