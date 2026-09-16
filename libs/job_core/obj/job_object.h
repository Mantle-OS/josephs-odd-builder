#pragma once

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

class JOBCORE_EXPORT Object : public BaseObject
{
public:
    using Ptr      = std::shared_ptr<Object>;
    using WPtr     = std::weak_ptr<Object>;
    using UPtr     = std::unique_ptr<Object>;
    using ObjectId = std::uint64_t;

    Object();
    virtual ~Object();

    Object(const Object &) = delete;
    Object &operator=(const Object &) = delete;
    Object(Object &&) = delete;
    Object &operator=(Object &&) = delete;

    template <typename T = Object, typename... Args>
        requires std::derived_from<T, Object> && std::constructible_from<T, Args...>
    [[nodiscard]] static std::shared_ptr<T> createShared(Args &&...args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    template <typename T = Object, typename... Args>
        requires std::derived_from<T, Object> && std::constructible_from<T, Args...>
    [[nodiscard]] static std::unique_ptr<T> createUniq(Args &&...args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    [[nodiscard]] ObjectId uid() const noexcept;

    [[nodiscard]] bool blockSignals(bool block) noexcept;
    [[nodiscard]] bool signalsBlocked() const noexcept;

    void registerConnection(const Connection &connection);
    void registerConnection(Connection &&connection);
    void disconnectAll();

    [[nodiscard]] std::size_t connectionCount() const;

    [[nodiscard]] const std::atomic<bool> *signalBlockState() const noexcept;

private:
    void pruneConnectionsLocked();

    static std::atomic<ObjectId> s_nextObjectId;

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
[[nodiscard]] std::shared_ptr<T> makeShared(Args &&...args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template <ObjectType T, typename... Args>
[[nodiscard]] std::unique_ptr<T> makeUniq(Args &&...args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

} // namespace job::core