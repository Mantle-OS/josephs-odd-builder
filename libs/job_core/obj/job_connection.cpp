#include "job_connection.h"

namespace job::core {

Connection::Connection() = default;

Connection::~Connection() = default;

Connection::Connection(const Connection &) = default;

Connection &Connection::operator=(const Connection &) = default;

Connection::Connection(Connection &&) noexcept = default;

Connection &Connection::operator=(Connection &&) noexcept = default;

Connection::ConnectionId Connection::id() const noexcept
{
    return m_state ? m_state->id : 0;
}

ConnectionFlag Connection::flags() const noexcept
{
    return m_state ? m_state->flags : ConnectionFlag::None;
}

bool Connection::isUnique() const noexcept
{
    return hasConnectionFlag(flags(), ConnectionFlag::Unique);
}

bool Connection::isSingleShot() const noexcept
{
    return hasConnectionFlag(flags(), ConnectionFlag::SingleShot);
}

bool Connection::connected() const noexcept
{
    return m_state && m_state->connected.load(std::memory_order_acquire);
}

Connection::operator bool() const noexcept
{
    return connected();
}

void Connection::disconnect()
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

Connection::Control::Control(DisconnectHandler handler) :
    disconnectHandler(std::move(handler))
{
}

Connection::Control::~Control() = default;

void Connection::Control::disconnect(ConnectionId id)
{
    if (disconnectHandler)
        disconnectHandler(id);
}

Connection::State::State(ConnectionId connectionId,
                         ConnectionFlag connectionFlags,
                         const std::shared_ptr<Control> &connectionControl) :
    id(connectionId),
    flags(connectionFlags),
    control(connectionControl)
{
}

Connection::State::~State() = default;

Connection::Connection(ConnectionId id, ConnectionFlag flags, const std::shared_ptr<Control> &control) :
    m_state(std::make_shared<State>(id, flags, control))
{
}

void Connection::markDisconnected() noexcept
{
    if (m_state)
        m_state->connected.store(false, std::memory_order_release);
}

} // namespace job::core