#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT PipeWirePort
{
public:
    using Ptr  = std::shared_ptr<PipeWirePort>;
    using WPtr = std::weak_ptr<PipeWirePort>;
    using UPtr = std::unique_ptr<PipeWirePort>;

    static Ptr createShared() { return std::make_shared<PipeWirePort>(); }
    static UPtr createUnique() { return std::make_unique<PipeWirePort>(); }

    explicit PipeWirePort() = default;

    PipeWirePort(const PipeWirePort &) = default;
    PipeWirePort(PipeWirePort &&) noexcept = default;

    PipeWirePort &operator=(const PipeWirePort &) = default;
    PipeWirePort &operator=(PipeWirePort &&) noexcept = default;

    ~PipeWirePort() = default;

    [[nodiscard]] std::string uid() const
    {
        return std::to_string(m_portId);
    }

    [[nodiscard]] const std::string &name() const noexcept
    {
        return m_name;
    }

    [[nodiscard]] const std::string &direction() const noexcept
    {
        return m_direction;
    }

    [[nodiscard]] std::uint32_t portId() const noexcept
    {
        return m_portId;
    }

    void setName(const std::string &name)
    {
        m_name = name;
    }

    void setDirection(const std::string &direction)
    {
        m_direction = direction;
    }

    void setPortId(std::uint32_t portId) noexcept
    {
        m_portId = portId;
    }

private:
    std::string   m_name;
    std::string   m_direction; // "Input" or "Output"
    std::uint32_t m_portId = 0;
};

} // namespace job::sound