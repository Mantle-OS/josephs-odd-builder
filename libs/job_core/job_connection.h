#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

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

constexpr ConnectionFlag& operator|=(ConnectionFlag &lhs, ConnectionFlag rhs) noexcept
{
    lhs = lhs | rhs;
    return lhs;
}

[[nodiscard]] constexpr bool hasConnectionFlag(ConnectionFlag flags, ConnectionFlag flag) noexcept
{
    return (flags & flag) != ConnectionFlag::None;
}

class Connection {
public:
    using ConnectionId = std::uint64_t;

    Connection() = default;
    ~Connection() = default;

    Connection(const Connection&) = default;
    Connection &operator=(const Connection&) = default;
    Connection(Connection&&) noexcept = default;
    Connection &operator=(Connection&&) noexcept = default;

    [[nodiscard]] ConnectionId id() const noexcept
    {
        return m_state ? m_state->id : 0;
    }

    [[nodiscard]] ConnectionFlag flags() const noexcept
    {
        return m_state ? m_state->flags : ConnectionFlag::None;
    }

    [[nodiscard]] bool isUnique() const noexcept
    {
        return hasConnectionFlag(flags(), ConnectionFlag::Unique);
    }

    [[nodiscard]] bool isSingleShot() const noexcept
    {
        return hasConnectionFlag(flags(), ConnectionFlag::SingleShot);
    }

    [[nodiscard]] bool connected() const noexcept
    {
        return m_state && m_state->connected.load(std::memory_order_acquire);
    }

    explicit operator bool() const noexcept
    {
        return connected();
    }

    void disconnect()
    {
        if (!m_state)
            return;

        if (!m_state->connected.load(std::memory_order_acquire))
            return;

        const auto control = m_state->control.lock();

        if (!control) {
            m_state->connected.store(false, std::memory_order_release);
            return;
        }

        control->disconnect(m_state->id);
    }

private:
    using DisconnectHandler = std::move_only_function<void(ConnectionId)>;
    struct Control {
        explicit Control(DisconnectHandler handler) :
            disconnectHandler(std::move(handler))
        {
        }

        ~Control() = default;

        Control(const Control&) = delete;
        Control &operator=(const Control&) = delete;
        Control(Control&&) = delete;
        Control &operator=(Control&&) = delete;

        void disconnect(ConnectionId id)
        {
            if (disconnectHandler)
                disconnectHandler(id);
        }

        DisconnectHandler disconnectHandler;
    };

    struct State {
        State(ConnectionId connectionId, ConnectionFlag connectionFlags, const std::shared_ptr<Control>& connectionControl) :
            id(connectionId),
            flags(connectionFlags),
            control(connectionControl)
        {

        }

        ~State() = default;

        State(const State&) = delete;
        State& operator=(const State&) = delete;
        State(State&&) = delete;
        State& operator=(State&&) = delete;

        ConnectionId id{0};
        ConnectionFlag flags{ConnectionFlag::None};
        std::atomic<bool> connected{true};
        std::atomic<bool> singleShotClaimed{false};
        std::weak_ptr<Control> control;
    };

    explicit Connection(ConnectionId id, ConnectionFlag flags, const std::shared_ptr<Control>& control) :
        m_state(std::make_shared<State>(id, flags, control))
    {
    }

    void markDisconnected() noexcept
    {
        if (m_state)
            m_state->connected.store(false, std::memory_order_release);
    }

    template <typename... Args>
    friend class Signal;

    std::shared_ptr<State> m_state;
};

} // namespace job::core