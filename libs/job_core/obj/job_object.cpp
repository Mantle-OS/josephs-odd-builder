#include "job_object.h"

#include <algorithm>

namespace job::core {

std::atomic<Object::ObjectId> Object::s_nextObjectId{0};

Object::Object() :
    m_uid(++s_nextObjectId)
{
}

Object::~Object()
{
    disconnectAll();
}

Object::ObjectId Object::uid() const noexcept
{
    return m_uid;
}

bool Object::blockSignals(bool block) noexcept
{
    return m_signalsBlocked.exchange(block, std::memory_order_acq_rel);
}

bool Object::signalsBlocked() const noexcept
{
    return m_signalsBlocked.load(std::memory_order_acquire);
}

void Object::registerConnection(const Connection &connection)
{
    if (!connection)
        return;

    std::lock_guard<std::mutex> lock(m_connMutex);

    pruneConnectionsLocked();
    m_connections.push_back(connection);
}

void Object::registerConnection(Connection &&connection)
{
    if (!connection)
        return;

    std::lock_guard<std::mutex> lock(m_connMutex);

    pruneConnectionsLocked();
    m_connections.push_back(std::move(connection));
}

void Object::disconnectAll()
{
    std::vector<Connection> connections;

    {
        std::lock_guard<std::mutex> lock(m_connMutex);
        connections.swap(m_connections);
    }

    for (auto &connection : connections)
        connection.disconnect();
}

std::size_t Object::connectionCount() const
{
    std::lock_guard<std::mutex> lock(m_connMutex);

    return static_cast<std::size_t>(
        std::count_if(m_connections.begin(), m_connections.end(), [](const Connection &connection) {
            return connection.connected();
        }));
}

const std::atomic<bool> *Object::signalBlockState() const noexcept
{
    return &m_signalsBlocked;
}

void Object::pruneConnectionsLocked()
{
    std::erase_if(m_connections, [](const Connection &connection) {
        return !connection.connected();
    });
}

} // namespace job::core