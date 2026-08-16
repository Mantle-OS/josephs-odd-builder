#include "alsa_control.h"

#include <utility>

namespace job::sound {

const std::string &AlsaControl::uid() const noexcept
{
    return m_uid;
}

bool AlsaControl::isPlayback() const noexcept
{
    return m_isPlayback;
}

bool AlsaControl::isCapture() const noexcept
{
    return m_isCapture;
}

bool AlsaControl::isMuted() const noexcept
{
    return m_isMuted;
}

bool AlsaControl::mute() const noexcept
{
    return m_mute;
}

long AlsaControl::volume() const noexcept
{
    return m_volume;
}

long AlsaControl::minVolume() const noexcept
{
    return m_minVolume;
}

long AlsaControl::maxVolume() const noexcept
{
    return m_maxVolume;
}

float AlsaControl::dBVolume() const noexcept
{
    return m_dBVolume;
}

float AlsaControl::minDb() const noexcept
{
    return m_minDb;
}

float AlsaControl::maxDb() const noexcept
{
    return m_maxDb;
}

void AlsaControl::setUid(const std::string &ctlName)
{
    if (!ctlName.empty() && ctlName != m_uid)
        m_uid = ctlName;
}

void AlsaControl::setIsPlayback(bool playback) noexcept
{
    m_isPlayback = playback;
}

void AlsaControl::setIsCapture(bool capture) noexcept
{
    m_isCapture = capture;
}

void AlsaControl::setIsMuted(bool muted) noexcept
{
    m_isMuted = muted;
}

void AlsaControl::setMute(bool mute) noexcept
{
    m_mute = mute;
}

void AlsaControl::setVolume(long volume) noexcept
{
    m_volume = volume;
}

void AlsaControl::setMinVolume(long minVolume) noexcept
{
    m_minVolume = minVolume;
}

void AlsaControl::setMaxVolume(long maxVolume) noexcept
{
    m_maxVolume = maxVolume;
}

void AlsaControl::setDBVolume(float dbVolume) noexcept
{
    m_dBVolume = dbVolume;
}

void AlsaControl::setMinDb(float minDb) noexcept
{
    m_minDb = minDb;
}

void AlsaControl::setMaxDb(float maxDb) noexcept
{
    m_maxDb = maxDb;
}

void AlsaControl::updateVolume(long vol, long min, long max) noexcept
{
    setVolume(vol);
    setMinVolume(min);
    setMaxVolume(max);
}

void AlsaControl::updateDb(float db, float min, float max) noexcept
{
    setDBVolume(db);
    setMinDb(min);
    setMaxDb(max);
}

void AlsaControl::updateState(bool muted, bool playback, bool capture) noexcept
{
    setIsMuted(muted);
    setIsPlayback(playback);
    setIsCapture(capture);
}

} //  namespace
