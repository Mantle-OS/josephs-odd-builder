#pragma once

#include <memory>

#include <job_obj_hash.h>

#include "jobsound_export.h"
#include "pipewire_port.h"

namespace job::sound {

class JOBSOUND_EXPORT PipeWireGraphNode
{
public:
    using Ptr  = std::shared_ptr<PipeWireGraphNode>;
    using WPtr = std::weak_ptr<PipeWireGraphNode>;
    using UPtr = std::unique_ptr<PipeWireGraphNode>;

    using PortModel = core::JobObjHashFast<PipeWirePort::UPtr>;

    static Ptr createShared(){ return std::make_shared<PipeWireGraphNode>(); }
    static UPtr createUnique() { return std::make_unique<PipeWireGraphNode>(); }

    explicit PipeWireGraphNode();
    ~PipeWireGraphNode() = default;

    PipeWireGraphNode(const PipeWireGraphNode &other);
    PipeWireGraphNode(PipeWireGraphNode &&) noexcept = default;
    PipeWireGraphNode &operator=(const PipeWireGraphNode &other);
    PipeWireGraphNode &operator=(PipeWireGraphNode &&) noexcept = default;

    [[nodiscard]] const std::string &uid() const noexcept;
    void setUid(const std::string &uid);

    [[nodiscard]] const std::string &name() const noexcept;
    void setName(const std::string &name);

    [[nodiscard]] const std::string &mediaClass() const noexcept;
    void setMediaClass(const std::string &mediaClass);

    [[nodiscard]] std::uint32_t nodeId() const noexcept;
    void setNodeId(std::uint32_t nodeId) noexcept;

    [[nodiscard]] PortModel *ports() noexcept;
    [[nodiscard]] const PortModel *ports() const noexcept;

private:
    std::string     m_uid;
    std::string     m_name;
    std::string     m_mediaClass;
    std::uint32_t   m_nodeId = 0;
    std::unique_ptr<PortModel> m_ports;
};
}