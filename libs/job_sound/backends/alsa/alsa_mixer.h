#pragma once

#include <algorithm>
#include <cstring>
#include <memory>

#include <alsa/asoundlib.h>

#include <job_logger.h>
#include <job_obj_hash.h>

#include "jobsound_export.h"
#include "virtual_mixer_channel.h"

namespace job::sound {

class JOBSOUND_EXPORT AlsaMixer
{
public:
    using Ptr  = std::shared_ptr<AlsaMixer>;
    using WPtr = std::weak_ptr<AlsaMixer>;
    using UPtr = std::unique_ptr<AlsaMixer>;

    using MVC = core::JobObjHashFast<VirtualMixerChannel::UPtr>;

    static Ptr createShared(bool init = true)
    {
        return std::make_shared<AlsaMixer>(init);
    }

    static UPtr createUnique(bool init = true)
    {
        return std::make_unique<AlsaMixer>(init);
    }

    explicit AlsaMixer(bool init = true)
    {
        if (init) {
            if (!initMixer())
                JOB_LOG_WARN("Failed to initialize ALSA mixer");
        }
    }

    AlsaMixer(const AlsaMixer &) = delete;
    AlsaMixer(AlsaMixer &&) = delete;

    AlsaMixer &operator=(const AlsaMixer &) = delete;
    AlsaMixer &operator=(AlsaMixer &&) = delete;

    ~AlsaMixer();

    [[nodiscard]] long masterVolume() const noexcept { return m_masterVolume; }
    [[nodiscard]] long captureVolume() const noexcept { return m_captureVolume; }

    [[nodiscard]] long masterMinVolume() const noexcept { return m_masterMinVolume; }
    [[nodiscard]] long masterMaxVolume() const noexcept { return m_masterMaxVolume; }

    [[nodiscard]] long captureMinVolume() const noexcept { return m_captureMinVolume; }
    [[nodiscard]] long captureMaxVolume() const noexcept { return m_captureMaxVolume; }

    [[nodiscard]] MVC &mixerModel() noexcept { return m_mixerModel; }
    [[nodiscard]] const MVC &mixerModel() const noexcept { return m_mixerModel; }

    long setMasterVolume(long masterVolume)
    {
        const long volume = std::clamp(masterVolume, m_masterMinVolume, m_masterMaxVolume);
        if (m_masterVolume != volume) {
            m_masterVolume = volume;

            if (m_masterElem)
                snd_mixer_selem_set_playback_volume_all(m_masterElem, volume);
        }

        return m_masterVolume;
    }

    long setCaptureVolume(long captureVolume)
    {
        const long volume = std::clamp(captureVolume, m_captureMinVolume, m_captureMaxVolume);
        if (m_captureVolume != volume) {
            m_captureVolume = volume;
            if (m_captureElem)
                snd_mixer_selem_set_capture_volume_all(m_captureElem, volume);
        }
        return m_captureVolume;
    }

    void setMasterMinVolume(long volume) noexcept
    {
        m_masterMinVolume = volume;
        if (m_masterMaxVolume < m_masterMinVolume)
            m_masterMaxVolume = m_masterMinVolume;
        m_masterMaxVolume = std::min(m_masterMaxVolume, kMaximumVolume);
    }

    void setMasterMaxVolume(long volume) noexcept
    {
        m_masterMaxVolume = std::clamp(volume, m_masterMinVolume, kMaximumVolume);
    }

    void setCaptureMinVolume(long volume) noexcept
    {
        m_captureMinVolume = volume;

        if (m_captureMaxVolume < m_captureMinVolume)
            m_captureMaxVolume = m_captureMinVolume;

        m_captureMaxVolume = std::min(m_captureMaxVolume, kMaximumVolume);
    }

    void setCaptureMaxVolume(long volume) noexcept
    {
        m_captureMaxVolume = std::clamp(volume, m_captureMinVolume, kMaximumVolume);
    }

private:
    static constexpr long kMaximumVolume = 120;

    bool initMixer(const char *card = "default")
    {
        if (snd_mixer_open(&m_mixer, 0) < 0)
            return false;

        if (snd_mixer_attach(m_mixer, card) < 0)
            return false;

        if (snd_mixer_selem_register(m_mixer, nullptr, nullptr) < 0)
            return false;

        if (snd_mixer_load(m_mixer) < 0)
            return false;

        m_masterElem  = findElement("Master");
        m_captureElem = findElement("Capture");

        if (m_masterElem) {
            long min, max  = 0;

            snd_mixer_selem_get_playback_volume_range(m_masterElem, &min, &max);
            setMasterMinVolume(min);
            setMasterMaxVolume(std::min(max, kMaximumVolume));
            long val = 0;
            snd_mixer_selem_get_playback_volume(m_masterElem, SND_MIXER_SCHN_FRONT_LEFT, &val);
            setMasterVolume(val);
        }

        if (m_captureElem) {
            long min = 0;
            long max = 0;

            snd_mixer_selem_get_capture_volume_range(m_captureElem, &min, &max);
            setCaptureMinVolume(min);
            setCaptureMaxVolume(std::min(max, kMaximumVolume));

            long val = 0;
            snd_mixer_selem_get_capture_volume(m_captureElem, SND_MIXER_SCHN_FRONT_LEFT, &val);
            setCaptureVolume(val);
        }

        return true;
    }

    [[nodiscard]] snd_mixer_elem_t *findElement(const char *name) noexcept
    {
        for (snd_mixer_elem_t *elem = snd_mixer_first_elem(m_mixer);
             elem;
             elem = snd_mixer_elem_next(elem)) {

            if (!snd_mixer_selem_is_active(elem))
                continue;

            if (std::strcmp(snd_mixer_selem_get_name(elem), name) == 0)
                return elem;
        }

        return nullptr;
    }

private:
    long m_masterVolume     = 0;
    long m_captureVolume    = 0;

    long m_masterMinVolume  = 0;
    long m_masterMaxVolume  = 100;

    long m_captureMinVolume = 0;
    long m_captureMaxVolume = 100;

    MVC m_mixerModel;

    snd_mixer_t      *m_mixer      = nullptr;
    snd_mixer_elem_t *m_masterElem = nullptr;
    snd_mixer_elem_t *m_captureElem = nullptr;
};

} // namespace job::sound