#include "job_light_object.h"

#include <algorithm>

namespace job::core {

std::atomic<LightObject::ObjectId> LightObject::s_nextObjectId{0};

LightObject::LightObject() :
    m_uid(++s_nextObjectId)
{
}

LightObject::~LightObject()
{
    disconnectAll();
}

LightObject::ObjectId LightObject::uid() const noexcept
{
    return m_uid;
}

bool LightObject::blockSignals(bool block) noexcept
{
    return m_signalsBlocked.exchange(block, std::memory_order_acq_rel);
}

bool LightObject::signalsBlocked() const noexcept
{
    return m_signalsBlocked.load(std::memory_order_acquire);
}

void LightObject::registerConnection(const Connection &connection)
{
    if (!connection)
        return;

    std::lock_guard<std::mutex> lock(m_connMutex);

    pruneConnectionsLocked();
    m_connections.push_back(connection);
}

void LightObject::registerConnection(Connection &&connection)
{
    if (!connection)
        return;

    std::lock_guard<std::mutex> lock(m_connMutex);

    pruneConnectionsLocked();
    m_connections.push_back(std::move(connection));
}

void LightObject::disconnectAll()
{
    std::vector<Connection> connections;

    {
        std::lock_guard<std::mutex> lock(m_connMutex);
        connections.swap(m_connections);
    }

    for (auto &connection : connections)
        connection.disconnect();
}

std::size_t LightObject::connectionCount() const
{
    std::lock_guard<std::mutex> lock(m_connMutex);

    return static_cast<std::size_t>(
        std::count_if(m_connections.begin(), m_connections.end(), [](const Connection &connection) {
            return connection.connected();
        }));
}

const std::atomic<bool> *LightObject::signalBlockState() const noexcept
{
    return &m_signalsBlocked;
}

void LightObject::pruneConnectionsLocked()
{
    std::erase_if(m_connections, [](const Connection &connection) {
        return !connection.connected();
    });
}

} // namespace job::core