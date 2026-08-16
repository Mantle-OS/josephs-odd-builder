#include "pipewire_graph_node.h"

namespace job::sound {

PipeWireGraphNode::PipeWireGraphNode() :
    m_ports(std::make_unique<PortModel>())
{

}

PipeWireGraphNode::PipeWireGraphNode(const PipeWireGraphNode &other) :
    m_uid(other.m_uid),
    m_name(other.m_name),
    m_mediaClass(other.m_mediaClass),
    m_nodeId(other.m_nodeId),
    m_ports(std::make_unique<PortModel>())
{
    if (!other.m_ports)
        return;

    m_ports->reserve(other.m_ports->size());

    for (const auto &item : *other.m_ports) {
        const auto &port = item.second;

        if (!port)
            continue;

        m_ports->insert(std::make_unique<PipeWirePort>(*port));
    }
}

PipeWireGraphNode &PipeWireGraphNode::operator=(const PipeWireGraphNode &other)
{
    if (this == &other)
        return *this;

    m_uid        = other.m_uid;
    m_name       = other.m_name;
    m_mediaClass = other.m_mediaClass;
    m_nodeId     = other.m_nodeId;

    if (!m_ports)
        m_ports = std::make_unique<PortModel>();
    else if (!m_ports->isEmpty())
        m_ports->clear();

    if (!other.m_ports)
        return *this;

    m_ports->reserve(other.m_ports->size());

    for (const auto &item : *other.m_ports) {
        const auto &port = item.second;

        if (!port)
            continue;

        m_ports->insert(std::make_unique<PipeWirePort>(*port));
    }

    return *this;
}

const std::string &PipeWireGraphNode::uid() const noexcept
{
    return m_uid;
}

const std::string &PipeWireGraphNode::name() const noexcept
{
    return m_name;
}

const std::string &PipeWireGraphNode::mediaClass() const noexcept
{
    return m_mediaClass;
}

std::uint32_t PipeWireGraphNode::nodeId() const noexcept
{
    return m_nodeId;
}

PipeWireGraphNode::PortModel *PipeWireGraphNode::ports() noexcept
{
    return m_ports.get();
}

const PipeWireGraphNode::PortModel *PipeWireGraphNode::ports() const noexcept
{
    return m_ports.get();
}

void PipeWireGraphNode::setUid(const std::string &uid)
{
    m_uid = uid;
}

void PipeWireGraphNode::setName(const std::string &name)
{
    m_name = name;
}

void PipeWireGraphNode::setMediaClass(const std::string &mediaClass)
{
    m_mediaClass = mediaClass;
}

void PipeWireGraphNode::setNodeId(std::uint32_t nodeId) noexcept
{
    m_nodeId = nodeId;
}


}