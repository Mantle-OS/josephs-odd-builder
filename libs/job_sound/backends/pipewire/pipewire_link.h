#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT PipeWireLink
{
public:
    using Ptr  = std::shared_ptr<PipeWireLink>;
    using WPtr = std::weak_ptr<PipeWireLink>;
    using UPtr = std::unique_ptr<PipeWireLink>;

    static Ptr createShared() { return std::make_shared<PipeWireLink>(); }
    static UPtr createUnique() { return std::make_unique<PipeWireLink>(); }

    explicit PipeWireLink() = default;

    PipeWireLink(const PipeWireLink &) = default;
    PipeWireLink(PipeWireLink &&) noexcept = default;

    PipeWireLink &operator=(const PipeWireLink &) = default;
    PipeWireLink &operator=(PipeWireLink &&) noexcept = default;

    ~PipeWireLink() = default;

    [[nodiscard]] std::string uid() const
    {
        return std::to_string(m_linkId);
    }

    [[nodiscard]] std::uint32_t linkId() const noexcept { return m_linkId; }
    [[nodiscard]] std::uint32_t outputNodeId() const noexcept { return m_outputNodeId; }
    [[nodiscard]] std::uint32_t outputPortId() const noexcept { return m_outputPortId; }
    [[nodiscard]] std::uint32_t inputNodeId() const noexcept { return m_inputNodeId; }
    [[nodiscard]] std::uint32_t inputPortId() const noexcept { return m_inputPortId; }

    void setLinkId(std::uint32_t linkId) noexcept { m_linkId = linkId; }
    void setOutputNodeId(std::uint32_t outputNodeId) noexcept { m_outputNodeId = outputNodeId; }
    void setOutputPortId(std::uint32_t outputPortId) noexcept { m_outputPortId = outputPortId; }
    void setInputNodeId(std::uint32_t inputNodeId) noexcept { m_inputNodeId = inputNodeId; }
    void setInputPortId(std::uint32_t inputPortId) noexcept { m_inputPortId = inputPortId; }

private:
    std::uint32_t m_linkId       = 0;
    std::uint32_t m_outputNodeId = 0;
    std::uint32_t m_outputPortId = 0;
    std::uint32_t m_inputNodeId  = 0;
    std::uint32_t m_inputPortId  = 0;
};

} // namespace job::sound