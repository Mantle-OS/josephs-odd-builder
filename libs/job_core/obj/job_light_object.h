#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "job_connection.h"
#include "jobcore_export.h"

namespace job::core {

class JOBCORE_EXPORT LightObject
{
public:
    using Ptr      = std::shared_ptr<LightObject>;
    using WPtr     = std::weak_ptr<LightObject>;
    using UPtr     = std::unique_ptr<LightObject>;
    using ObjectId = std::uint64_t;

    LightObject();
    virtual ~LightObject();

    LightObject(const LightObject &) = delete;
    LightObject &operator=(const LightObject &) = delete;
    LightObject(LightObject &&) = delete;
    LightObject &operator=(LightObject &&) = delete;

    template <typename T = LightObject, typename... Args>
        requires std::derived_from<T, LightObject> && std::constructible_from<T, Args...>
    [[nodiscard]] static std::shared_ptr<T> createShared(Args &&...args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    template <typename T = LightObject, typename... Args>
        requires std::derived_from<T, LightObject> && std::constructible_from<T, Args...>
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

    static std::atomic<ObjectId>    s_nextObjectId;
    ObjectId                        m_uid{0};
    std::atomic<bool>               m_signalsBlocked{false};
    mutable std::mutex              m_connMutex;
    std::vector<Connection>         m_connections;
};

} // namespace job::core