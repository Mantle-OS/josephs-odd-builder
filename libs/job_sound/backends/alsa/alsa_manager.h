#pragma once

#include <initializer_list>
#include <memory>
#include <string>
#include <utility>

#include <alsa/asoundlib.h>

#include <job_logger.h>
#include <job_obj_hash.h>

#include "alsa_device.h"
#include "alsa_sound_card.h"
#include "dsp_manager.h"
#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT AlsaManager {
public:
    using Ptr  = std::shared_ptr<AlsaManager>;
    using WPtr = std::weak_ptr<AlsaManager>;
    using UPtr = std::unique_ptr<AlsaManager>;
    using CardModel = core::JobObjHashFast<AlsaSoundCard::UPtr>;

    static Ptr  createShared() { return std::make_shared<AlsaManager>(); }
    static UPtr createUnique() { return std::make_unique<AlsaManager>(); }
    static AlsaManager* instance() {
        static AlsaManager s_instance;
        return &s_instance;
    }

    explicit AlsaManager() :
        m_cards(std::make_unique<CardModel>()),
        m_processingManager(AudioProcessingManager::createUnique()) {
        populateAlsaHardware();
    }

    AlsaManager(const AlsaManager&) = delete;
    AlsaManager& operator=(const AlsaManager&) = delete;
    AlsaManager(AlsaManager&&) = delete;
    AlsaManager& operator=(AlsaManager&&) = delete;
    ~AlsaManager() = default;

    // --- Accessors ---
    [[nodiscard]] CardModel* cards() noexcept { return m_cards.get(); }
    [[nodiscard]] const CardModel* cards() const noexcept { return m_cards.get(); }

    [[nodiscard]] AlsaSoundCard* selectedCard() noexcept { return m_selectedCard; }
    [[nodiscard]] const AlsaSoundCard* selectedCard() const noexcept { return m_selectedCard; }

    [[nodiscard]] AlsaDevice* selectedDevice() noexcept { return m_selectedDevice; }
    [[nodiscard]] const AlsaDevice* selectedDevice() const noexcept { return m_selectedDevice; }

    [[nodiscard]] AudioProcessingManager* processingManager() noexcept { return m_processingManager.get(); }
    [[nodiscard]] const AudioProcessingManager* processingManager() const noexcept { return m_processingManager.get(); }

    // --- Mutators ---
    void setSelectedCard(AlsaSoundCard* card) noexcept {
        if (m_selectedCard != card) m_selectedCard = card;
    }

    void setSelectedDevice(AlsaDevice* device) {
        if (m_selectedDevice == device) return;
        m_selectedDevice = device;
        onSelectedDeviceChanged(device);
    }

    void populateAlsaHardware() {
        m_selectedCard = nullptr;
        m_selectedDevice = nullptr;
        if (!m_cards->isEmpty()) m_cards->clear();

        const int defaultCardIndex = snd_card_get_index("default");
        int cardNum = -1;

        while (snd_card_next(&cardNum) >= 0 && cardNum >= 0) {
            const std::string hwName = "hw:" + std::to_string(cardNum);

            snd_ctl_t* rawCtl = nullptr;
            if (snd_ctl_open(&rawCtl, hwName.c_str(), 0) < 0)
                continue;
            std::unique_ptr<snd_ctl_t, decltype(&snd_ctl_close)> ctl(rawCtl, &snd_ctl_close);

            snd_ctl_card_info_t* info = nullptr;
            snd_ctl_card_info_alloca(&info);

            if (snd_ctl_card_info(ctl.get(), info) < 0) continue;

            auto card = AlsaSoundCard::createUnique();
            card->setUid(hwName);
            card->setName(snd_ctl_card_info_get_name(info));
            card->setCard(snd_ctl_card_info_get_id(info));
            card->setChip(snd_ctl_card_info_get_mixername(info));
            card->setDriver(snd_ctl_card_info_get_driver(info));
            card->setLongName(snd_ctl_card_info_get_longname(info));
            card->setComponents(snd_ctl_card_info_get_components(info));
            card->setPath(hwName);

            enumeratePcmDevices(ctl.get(), card.get(), hwName);

            AlsaSoundCard* cardPtr = card.get();
            m_cards->insert(std::move(card));

            if (cardNum == defaultCardIndex && !m_selectedCard) {
                selectDefaultOrFirstCard(cardPtr);
            }
        }

        // Fallback to first available card if default index wasn't selected
        if (!m_selectedCard && !m_cards->isEmpty()) {
            selectDefaultOrFirstCard(m_cards->at(0));
        }
    }

private:
    void enumeratePcmDevices(snd_ctl_t* ctl, AlsaSoundCard* card, const std::string& hwName) {
        int device = -1;
        while (snd_ctl_pcm_next_device(ctl, &device) >= 0 && device >= 0) {
            for (const snd_pcm_stream_t stream : {SND_PCM_STREAM_PLAYBACK, SND_PCM_STREAM_CAPTURE}) {
                snd_pcm_info_t* pcmInfo = nullptr;
                snd_pcm_info_alloca(&pcmInfo);
                snd_pcm_info_set_device(pcmInfo, static_cast<unsigned int>(device));
                snd_pcm_info_set_subdevice(pcmInfo, 0);
                snd_pcm_info_set_stream(pcmInfo, stream);

                if (snd_ctl_pcm_info(ctl, pcmInfo) < 0) continue;

                const bool playback = (stream == SND_PCM_STREAM_PLAYBACK);
                const bool capture  = (stream == SND_PCM_STREAM_CAPTURE);

                auto dev = AlsaDevice::createUnique();
                dev->setPcmClass(snd_pcm_info_get_class(pcmInfo));
                dev->setSubClass(snd_pcm_info_get_subclass(pcmInfo));
                dev->setName(snd_pcm_info_get_name(pcmInfo));
                dev->setSubdeviceName(snd_pcm_info_get_subdevice_name(pcmInfo));
                dev->setDeviceNum(device);
                dev->setSubdeviceNum(static_cast<int>(snd_pcm_info_get_subdevice(pcmInfo)));
                dev->setType(playback ? "Playback" : "Capture");
                dev->setIsPlayback(playback);
                dev->setIsCapture(capture);
                dev->setUid(hwName + "," + std::to_string(device) + (playback ? ":playback" : ":capture"));

                if (playback || capture) {
                    dev->populateAlsaControls(card->path());
                }

                card->devices()->insert(std::move(dev));
            }
        }
    }

    void selectDefaultOrFirstCard(AlsaSoundCard* card) {
        if (!card) return;
        setSelectedCard(card);
        if (card->devices() && !card->devices()->isEmpty()) {
            setSelectedDevice(card->devices()->at(0));
        }
    }

    void onSelectedDeviceChanged(AlsaDevice* device) {
        if (!device || !m_selectedCard || !m_processingManager) return;

        JOB_LOG_DEBUG("[AlsaManager] Using device: {} Path: {}", device->name(), m_selectedCard->path());

        const std::string fullDevice = m_selectedCard->path() + "," + std::to_string(device->deviceNum());

        if (device->isCapture())  m_processingManager->startCaptureFrom(fullDevice);
        if (device->isPlayback()) m_processingManager->startPlaybackTo(fullDevice);
    }

private:
    std::unique_ptr<CardModel> m_cards;

    // Non-owning pointers managed by m_cards hierarchy
    AlsaSoundCard* m_selectedCard   = nullptr;
    AlsaDevice*    m_selectedDevice = nullptr;

    AudioProcessingManager::UPtr m_processingManager;
};

} // namespace job::sound