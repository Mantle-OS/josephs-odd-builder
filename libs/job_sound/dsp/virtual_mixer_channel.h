#pragma once

#include <memory>

#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT VirtualMixerChannel
{
public:
    using Ptr  = std::shared_ptr<VirtualMixerChannel>;
    using WPtr = std::weak_ptr<VirtualMixerChannel>;
    using UPtr = std::unique_ptr<VirtualMixerChannel>;

    static Ptr createShared() { return std::make_shared<VirtualMixerChannel>(); }
    static UPtr createUnique() { return std::make_unique<VirtualMixerChannel>(); }

    explicit VirtualMixerChannel() = default;

    VirtualMixerChannel(const VirtualMixerChannel &) = default;
    VirtualMixerChannel(VirtualMixerChannel &&) noexcept = default;

    VirtualMixerChannel &operator=(const VirtualMixerChannel &) = default;
    VirtualMixerChannel &operator=(VirtualMixerChannel &&) noexcept = default;

    ~VirtualMixerChannel() = default;

    [[nodiscard]] std::string uid() const { return std::to_string(m_channelIndex); }
    [[nodiscard]] int channelIndex() const noexcept { return m_channelIndex; }
    [[nodiscard]] int volume() const noexcept { return m_volume; }
    [[nodiscard]] bool mute() const noexcept { return m_mute; }

    void setChannelIndex(int channelIndex) noexcept { m_channelIndex = channelIndex; }
    void setVolume(int volume) noexcept { m_volume = volume; }
    void setMute(bool mute) noexcept { m_mute = mute; }

private:
    std::string m_uid;  // This should just return m_channelIndex as a string
    int  m_channelIndex = 0;
    int  m_volume       = 75;
    bool m_mute         = false;
};

} // namespace job::sound