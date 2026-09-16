#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include "jobcore_export.h"

namespace job::core {

template <typename... Args>
class Signal;

enum class ConnectionFlag : std::uint8_t {
    None       = 0,
    Unique     = 1 << 0,
    SingleShot = 1 << 1
};

[[nodiscard]] constexpr ConnectionFlag operator|(ConnectionFlag lhs, ConnectionFlag rhs) noexcept
{
    using Type = std::underlying_type_t<ConnectionFlag>;
    return static_cast<ConnectionFlag>(static_cast<Type>(lhs) | static_cast<Type>(rhs));
}

[[nodiscard]] constexpr ConnectionFlag operator&(ConnectionFlag lhs, ConnectionFlag rhs) noexcept
{
    using Type = std::underlying_type_t<ConnectionFlag>;
    return static_cast<ConnectionFlag>(static_cast<Type>(lhs) & static_cast<Type>(rhs));
}

constexpr ConnectionFlag &operator|=(ConnectionFlag &lhs, ConnectionFlag rhs) noexcept
{
    lhs = lhs | rhs;
    return lhs;
}

[[nodiscard]] constexpr bool hasConnectionFlag(ConnectionFlag flags, ConnectionFlag flag) noexcept
{
    return (flags & flag) != ConnectionFlag::None;
}

class JOBCORE_EXPORT Connection {
public:
    using ConnectionId = std::uint64_t;

    Connection();
    ~Connection();

    Connection(const Connection &);
    Connection &operator=(const Connection &);
    Connection(Connection &&) noexcept;
    Connection &operator=(Connection &&) noexcept;

    [[nodiscard]] ConnectionId id() const noexcept;
    [[nodiscard]] ConnectionFlag flags() const noexcept;
    [[nodiscard]] bool isUnique() const noexcept;
    [[nodiscard]] bool isSingleShot() const noexcept;
    [[nodiscard]] bool connected() const noexcept;

    explicit operator bool() const noexcept;

    void disconnect();

private:
    using DisconnectHandler = std::move_only_function<void(ConnectionId)>;

    struct Control {
        explicit Control(DisconnectHandler handler);
        ~Control();

        Control(const Control &) = delete;
        Control &operator=(const Control &) = delete;
        Control(Control &&) = delete;
        Control &operator=(Control &&) = delete;

        void disconnect(ConnectionId id);

        DisconnectHandler disconnectHandler;
    };

    struct State {
        State(ConnectionId connectionId, ConnectionFlag connectionFlags, const std::shared_ptr<Control> &connectionControl);
        ~State();

        State(const State &) = delete;
        State &operator=(const State &) = delete;
        State(State &&) = delete;
        State &operator=(State &&) = delete;

        ConnectionId id{0};
        ConnectionFlag flags{ConnectionFlag::None};
        std::atomic<bool> connected{true};
        std::atomic<bool> singleShotClaimed{false};
        std::weak_ptr<Control> control;
    };

    explicit Connection(ConnectionId id, ConnectionFlag flags, const std::shared_ptr<Control> &control);

    void markDisconnected() noexcept;

    template <typename... Args>
    friend class Signal;

    std::shared_ptr<State> m_state;
};

} // namespace job::core