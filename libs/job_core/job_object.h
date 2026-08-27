#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "job_base_obj.h"
#include "job_connection.h"
#include "job_obj_annotation.h"
#include "job_obj_concept.h"

#include "jobcore_export.h"

namespace job::core {

class JOBCORE_EXPORT Object : public BaseObject {
public:
    using ObjectId = std::uint64_t;

    Object()
        : m_uid(++s_nextObjectId)
    {
    }

    virtual ~Object()
    {
        disconnectAll();
    }

    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;
    Object(Object&&) = delete;
    Object& operator=(Object&&) = delete;

    [[nodiscard]] ObjectId uid() const noexcept
    {
        return m_uid;
    }

    [[nodiscard]] virtual bool isValid() const noexcept = 0;

    [[nodiscard]] bool blockSignals(bool block) noexcept
    {
        return m_signalsBlocked.exchange(block, std::memory_order_acq_rel);
    }

    [[nodiscard]] bool signalsBlocked() const noexcept
    {
        return m_signalsBlocked.load(std::memory_order_acquire);
    }

    void registerConnection(const Connection& connection)
    {
        if (!connection)
            return;

        std::lock_guard<std::mutex> lock(m_connMutex);

        pruneConnectionsLocked();
        m_connections.push_back(connection);
    }

    void registerConnection(Connection&& connection)
    {
        if (!connection)
            return;

        std::lock_guard<std::mutex> lock(m_connMutex);

        pruneConnectionsLocked();
        m_connections.push_back(std::move(connection));
    }

    void disconnectAll()
    {
        std::vector<Connection> connections;

        {
            std::lock_guard<std::mutex> lock(m_connMutex);
            connections.swap(m_connections);
        }

        for (auto& connection : connections)
            connection.disconnect();
    }

    [[nodiscard]] std::size_t connectionCount() const
    {
        std::lock_guard<std::mutex> lock(m_connMutex);

        return static_cast<std::size_t>(
            std::count_if(
                m_connections.begin(),
                m_connections.end(),
                [](const Connection& connection) {
                    return connection.connected();
                }));
    }

    [[nodiscard]] const std::atomic<bool>* signalBlockState() const noexcept
    {
        return &m_signalsBlocked;
    }

private:
    void pruneConnectionsLocked()
    {
        std::erase_if(m_connections, [](const Connection& connection) {
            return !connection.connected();
        });
    }

    inline static std::atomic<ObjectId> s_nextObjectId{0};

    [[=NoSerialize{}]]
        [[=NoReset{}]]
        ObjectId m_uid{0};

    [[=NoSerialize{}]]
        [[=NoReset{}]]
        std::atomic<bool> m_signalsBlocked{false};

    [[=NoSerialize{}]]
        [[=NoReset{}]]
        mutable std::mutex m_connMutex;

    [[=NoSerialize{}]]
        [[=NoReset{}]]
        std::vector<Connection> m_connections;
};

// =============================================================================
// Free Factory Helpers
// =============================================================================

template <ObjectType T, typename... Args>
[[nodiscard]] std::shared_ptr<T> makeShared(Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template <ObjectType T, typename... Args>
[[nodiscard]] std::unique_ptr<T> makeUniq(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

} // namespace job::core