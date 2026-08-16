#pragma once

#include <memory>
#include <string>
#include <utility>

#include <alsa/asoundlib.h>

#include <job_obj_hash.h>

#include "alsa_control.h"

#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT AlsaDevice
{
public:
    using Ptr  = std::shared_ptr<AlsaDevice>;
    using WPtr = std::weak_ptr<AlsaDevice>;
    using UPtr = std::unique_ptr<AlsaDevice>;

    using MVC = std::unique_ptr<core::JobObjHashFast<AlsaControl::UPtr>>;

    static Ptr createShared() { return std::make_shared<AlsaDevice>(); }
    static UPtr createUnique() { return std::make_unique<AlsaDevice>(); }

    explicit AlsaDevice()
        : m_controls(std::make_unique<core::JobObjHashFast<AlsaControl::UPtr>>())
    {
    }

    AlsaDevice(const AlsaDevice &other)
        : m_pcmClass(other.m_pcmClass),
        m_subClass(other.m_subClass),
        m_name(other.m_name),
        m_uid(other.m_uid),
        m_type(other.m_type),
        m_deviceNum(other.m_deviceNum),
        m_subdeviceNum(other.m_subdeviceNum),
        m_subdeviceName(other.m_subdeviceName),
        m_isPlayback(other.m_isPlayback),
        m_isCapture(other.m_isCapture),
        m_controls(std::make_unique<core::JobObjHashFast<AlsaControl::UPtr>>())
    {
        if (!other.m_controls)
            return;

        m_controls->reserve(other.m_controls->size());
        for (const auto &item : *other.m_controls) {
            const auto &control = item.second;
            if (!control)
                continue;

            m_controls->insert(std::make_unique<AlsaControl>(*control));
        }
    }

    AlsaDevice(AlsaDevice &&) noexcept = default;

    AlsaDevice &operator=(const AlsaDevice &other)
    {
        if (this == &other)
            return *this;

        m_pcmClass      = other.m_pcmClass;
        m_subClass      = other.m_subClass;
        m_name          = other.m_name;
        m_uid           = other.m_uid;
        m_type          = other.m_type;
        m_deviceNum     = other.m_deviceNum;
        m_subdeviceNum  = other.m_subdeviceNum;
        m_subdeviceName = other.m_subdeviceName;
        m_isPlayback    = other.m_isPlayback;
        m_isCapture     = other.m_isCapture;

        if (!m_controls)
            m_controls = std::make_unique<core::JobObjHashFast<AlsaControl::UPtr>>();
        else if (!m_controls->isEmpty())
            m_controls->clear();

        if (!other.m_controls)
            return *this;

        m_controls->reserve(other.m_controls->size());
        for (const auto &item : *other.m_controls) {
            const auto &control = item.second;
            if (!control)
                continue;

            m_controls->insert(std::make_unique<AlsaControl>(*control));
        }

        return *this;
    }

    AlsaDevice &operator=(AlsaDevice &&) noexcept = default;

    ~AlsaDevice() = default;

    [[nodiscard]] snd_pcm_class_t pcmClass() const noexcept { return m_pcmClass; }
    [[nodiscard]] snd_pcm_subclass_t subClass() const noexcept { return m_subClass; }

    [[nodiscard]] const std::string &name() const noexcept { return m_name; }
    [[nodiscard]] const std::string &uid() const noexcept { return m_uid; }
    [[nodiscard]] const std::string &type() const noexcept { return m_type; }

    [[nodiscard]] int deviceNum() const noexcept { return m_deviceNum; }
    [[nodiscard]] int subdeviceNum() const noexcept { return m_subdeviceNum; }

    [[nodiscard]] const std::string &subdeviceName() const noexcept { return m_subdeviceName; }

    [[nodiscard]] bool isPlayback() const noexcept { return m_isPlayback; }
    [[nodiscard]] bool isCapture() const noexcept { return m_isCapture; }

    [[nodiscard]] core::JobObjHashFast<AlsaControl::UPtr> *controls() noexcept
    {
        return m_controls.get();
    }

    [[nodiscard]] const core::JobObjHashFast<AlsaControl::UPtr> *controls() const noexcept
    {
        return m_controls.get();
    }

    void setPcmClass(snd_pcm_class_t pcmClass) noexcept { m_pcmClass = pcmClass; }
    void setSubClass(snd_pcm_subclass_t subClass) noexcept { m_subClass = subClass; }

    void setName(const std::string &name) { m_name = name; }
    void setUid(const std::string &uid) { m_uid = uid; }
    void setType(const std::string &type) { m_type = type; }

    void setDeviceNum(int deviceNum) noexcept { m_deviceNum = deviceNum; }
    void setSubdeviceNum(int subdeviceNum) noexcept { m_subdeviceNum = subdeviceNum; }

    void setSubdeviceName(const std::string &subdeviceName)
    {
        m_subdeviceName = subdeviceName;
    }

    void setIsPlayback(bool playback) noexcept { m_isPlayback = playback; }
    void setIsCapture(bool capture) noexcept { m_isCapture = capture; }

    void populateAlsaControls(const std::string &cardName)
    {
        if (!m_controls->isEmpty())
            m_controls->clear();

        snd_mixer_t *mixer = nullptr;
        if (snd_mixer_open(&mixer, 0) < 0)
            return;

        // [[FIXME LATER what if UTF8 shannagaings]]
        if (snd_mixer_attach(mixer, cardName.c_str()) < 0) {
            snd_mixer_close(mixer);
            return;
        }

        snd_mixer_selem_register(mixer, nullptr, nullptr);

        if (snd_mixer_load(mixer) < 0) {
            snd_mixer_close(mixer);
            return;
        }

        for (snd_mixer_elem_t *elem = snd_mixer_first_elem(mixer);
             elem;
             elem = snd_mixer_elem_next(elem)) {

            if (!snd_mixer_selem_is_active(elem))
                continue;


            const char *elemName = snd_mixer_selem_get_name(elem);
            const unsigned int elemIndex = snd_mixer_selem_get_index(elem);

            long minVol = 0;
            long maxVol = 0;
            snd_mixer_selem_get_playback_volume_range(elem, &minVol, &maxVol);

            long left  = 0;
            long right = 0;

            snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &left);
            snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT, &right);

            const int avg = static_cast<int>((left + right) / 2);

            long minDb = 0;
            long maxDb = 0;
            snd_mixer_selem_get_playback_dB_range(elem, &minDb, &maxDb);

            long dbValue = 0;
            snd_mixer_selem_get_playback_dB(elem, SND_MIXER_SCHN_FRONT_LEFT, &dbValue);

            // yikes again with the from/to Utf8 madness
            AlsaControl::UPtr control = AlsaControl::createUnique();
            control->setUid(std::string{elemName} + ":" + std::to_string(elemIndex));
            control->setMinVolume(minVol);
            control->setMaxVolume(maxVol);
            control->setVolume(avg);

            control->setMinDb(minDb / 100.0f);
            control->setMaxDb(maxDb / 100.0f);
            control->setDBVolume(dbValue / 100.0f);

            int sw = 0;
            snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_FRONT_LEFT, &sw);
            control->setMute(!sw);

            m_controls->insert(std::move(control));
        }

        snd_mixer_close(mixer);
    }

private:
    snd_pcm_class_t    m_pcmClass = SND_PCM_CLASS_GENERIC;
    snd_pcm_subclass_t m_subClass = SND_PCM_SUBCLASS_GENERIC_MIX;

    std::string m_name;                 // ALSA name like "Master", "PCM", etc.
    std::string m_uid;                  // Unique ID: e.g., "hw:0,0:playback"
    std::string m_type;                 // Playback, Capture, Both

    int m_deviceNum    = 0;             // e.g., 0, 1, etc.
    int m_subdeviceNum = 0;

    std::string m_subdeviceName;

    bool m_isPlayback = false;
    bool m_isCapture  = false;

    MVC m_controls;                     // Owned
};

} // namespace job::sound