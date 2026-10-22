#pragma once

#include <cstdint>
#include <memory>

#include <job_base_obj.h>
#include <job_obj_annotation.h>

#include <real_type.h>
#include <inplace_vector>

#include "jobmodel_export.h"

namespace job::model {

enum RopeType : std::uint8_t {
    Normal = 0,
    NormalInplace ,
    Ext,
    ExtInplace,
    ExtBack,
    Multi,
    MultiBack,
    MultiInplace,
    Custom,
    CustomInplace
};

enum RopeMode : std::uint8_t {
    None = 0,
    OneD = 1,
    TwoD = 2,
    ThreeD = 3,
};

class JOBMODEL_EXPORT RopeConfig : public job::core::BaseObject
{
public:
    using Ptr  = std::shared_ptr<RopeConfig>;
    using WPtr = std::weak_ptr<RopeConfig>;
    using UPtr = std::unique_ptr<RopeConfig>;

    RopeConfig();
    ~RopeConfig() override = default;

    [[nodiscard]] static Ptr createShared() { return std::make_shared<RopeConfig>(); }
    [[nodiscard]] static UPtr createUniq()  { return std::make_unique<RopeConfig>(); }

    RopeConfig(const RopeConfig &) = default;
    RopeConfig &operator=(const RopeConfig &) = default;
    RopeConfig(RopeConfig &&) noexcept = default;
    RopeConfig &operator=(RopeConfig &&) noexcept = default;

    [[nodiscard]] std::uint32_t ropeDimensionCount() const noexcept { return m_ropeDimensionCount; }
    void setRopeDimensionCount(std::uint32_t value) noexcept { m_ropeDimensionCount = value; }
    [[nodiscard]] int ropeDimCntInt() const noexcept { return static_cast<int>(m_ropeDimensionCount); }

    [[nodiscard]] float ropeFreqBase() const noexcept { return m_ropeFreqBase; }
    void setRopeFreqBase(float value);

    [[nodiscard]] float ropeFreqScale() const noexcept { return m_ropeFreqScale; }
    void setRopeFreqScale(float value);

    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] RopeMode mode()const noexcept {return m_mode; }
    void setMode(RopeMode mode){m_mode = mode;}
    void setModeInt(int mode)
    {
        if(mode >=0 && mode <= 3)
            m_mode = static_cast<RopeMode>(mode);
    }
    [[nodiscard]] int modeInt() const noexcept { return static_cast<int>(m_mode); }

    [[nodiscard]] RopeType ropeType() const noexcept { return m_type; }
    void setRopeType(RopeType type){m_type = type;}

    [[nodiscard]] std::uint32_t ctxOrigCount()const noexcept{return m_ctxOrigCount;}
    void setCtxOrigCount(std::uint32_t value){ m_ctxOrigCount = value; }
    [[nodiscard]] int ctxOrigCountInt() const noexcept { return static_cast<int>(m_ctxOrigCount); }

    [[nodiscard]] const std::inplace_vector<int, 4> &sections() const noexcept { return m_sections; }
    void setSections(const std::inplace_vector<int, 4> &sections) { m_sections = sections; }

    [[nodiscard]] float extFactor()const noexcept{return m_extFactor;  }
    void setExtFactor(float value){ m_extFactor = value; }

    [[nodiscard]] float attnFactor()const noexcept{return m_attnFactor; }
    void setAttnFactor(float value){ m_attnFactor = value; }

    [[nodiscard]] float betaFast()const noexcept{return m_betaFast;   }
    void setBetaFast(float value) { m_betaFast = value; }

    [[nodiscard]] float betaSlow()const noexcept{return m_betaSlow;    }
    void setBetaSlow(float value) { m_betaSlow = value; }

private:
    RopeType        m_type{RopeType::Normal};
    RopeMode        m_mode{RopeMode::TwoD};
    std::uint32_t   m_ropeDimensionCount{0};
    float           m_ropeFreqBase{10000.0f};
    float           m_ropeFreqScale{1.0f};

    std::uint32_t               m_ctxOrigCount{0};
    std::inplace_vector<int, 4> m_sections{};// GGML_MROPE_SECTIONS  which is defined as 4
    float                       m_extFactor{0.0f};
    float                       m_attnFactor{1.0f};
    float                       m_betaFast{0.0f};
    float                       m_betaSlow{0.0f};
};

} // namespace job::model