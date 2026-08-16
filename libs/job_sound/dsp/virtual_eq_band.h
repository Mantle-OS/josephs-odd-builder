#pragma once

#include <memory>
#include <string>
#include <utility>
#include "jobsound_export.h"
namespace job::sound {

class JOBSOUND_EXPORT VirtualEqBand
{
public:
    using Ptr  = std::shared_ptr<VirtualEqBand>;
    using WPtr = std::weak_ptr<VirtualEqBand>;
    using UPtr = std::unique_ptr<VirtualEqBand>;

    static Ptr createShared() { return std::make_shared<VirtualEqBand>(); }
    static UPtr createUnique() { return std::make_unique<VirtualEqBand>(); }

    explicit VirtualEqBand() = default;

    VirtualEqBand(const VirtualEqBand &) = default;
    VirtualEqBand(VirtualEqBand &&) noexcept = default;

    VirtualEqBand &operator=(const VirtualEqBand &) = default;
    VirtualEqBand &operator=(VirtualEqBand &&) noexcept = default;

    ~VirtualEqBand() = default;

    [[nodiscard]] const std::string &uid() const noexcept { return m_uid; }
    [[nodiscard]] float frequency() const noexcept { return m_frequency; }
    [[nodiscard]] float gain() const noexcept { return m_gain; }
    [[nodiscard]] int bandIndex() const noexcept { return m_bandIndex; }

    void setUid(const std::string &uid) { m_uid = uid; }
    void setFrequency(float frequency) noexcept { m_frequency = frequency; }
    void setGain(float gain) noexcept { m_gain = gain; }
    void setBandIndex(int bandIndex) noexcept { m_bandIndex = bandIndex; }

private:
    std::string     m_uid;
    float           m_frequency = 0.0f;
    float           m_gain      = 0.0f;
    int             m_bandIndex = -1;
};

} // namespace job::sound