#pragma once

#include <memory>
#include <string>

#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT AlsaControl
{
public:
    using Ptr  = std::shared_ptr<AlsaControl>;
    using WPtr = std::weak_ptr<AlsaControl>;
    using UPtr = std::unique_ptr<AlsaControl>;
    static Ptr createShared() { return std::make_shared<AlsaControl>();}
    static UPtr createUnique() { return std::make_unique<AlsaControl>();}

    explicit AlsaControl() = default;

    AlsaControl(const AlsaControl &) = default;
    AlsaControl(AlsaControl &&) noexcept = default;
    AlsaControl &operator=(const AlsaControl &) = default;
    AlsaControl &operator=(AlsaControl &&) noexcept = default;

    ~AlsaControl() = default;

    [[nodiscard]] const std::string &uid() const noexcept ;

    [[nodiscard]] bool isPlayback() const noexcept;
    [[nodiscard]] bool isCapture() const noexcept;
    [[nodiscard]] bool isMuted() const noexcept;
    [[nodiscard]] bool mute() const noexcept;

    [[nodiscard]] long volume() const noexcept;
    [[nodiscard]] long minVolume() const noexcept;
    [[nodiscard]] long maxVolume() const noexcept;

    [[nodiscard]] float dBVolume() const noexcept;
    [[nodiscard]] float minDb() const noexcept;
    [[nodiscard]] float maxDb() const noexcept;

    void setUid(const std::string &ctlName);

    void setIsPlayback(bool playback) noexcept;
    void setIsCapture(bool capture) noexcept;
    void setIsMuted(bool muted) noexcept;
    void setMute(bool mute) noexcept;

    void setVolume(long volume) noexcept;
    void setMinVolume(long minVolume) noexcept;
    void setMaxVolume(long maxVolume) noexcept;

    void setDBVolume(float dbVolume) noexcept;
    void setMinDb(float minDb) noexcept;
    void setMaxDb(float maxDb) noexcept;

    void updateVolume(long vol, long min, long max) noexcept;
    void updateDb(float db, float min, float max) noexcept;
    void updateState(bool muted, bool playback, bool capture) noexcept;

private:
    std::string m_uid;

    bool m_isPlayback = false;
    bool m_isCapture  = false;
    bool m_isMuted    = false;

    long m_volume    = 0;
    long m_minVolume = 0;
    long m_maxVolume = 100;

    float m_dBVolume = 0.0f;
    float m_minDb    = -100.0f;
    float m_maxDb    = 0.0f;

    bool m_mute = false;
};

} // namespace job::sound