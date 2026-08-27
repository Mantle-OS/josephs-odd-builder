#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>
#include <vector>

#include "job_connection.h"
#include "job_obj_concept.h"

namespace job::core {

template <typename T>
struct SignalConnection;

template <typename... Args>
class Signal {
public:
    using ConnectionId = Connection::ConnectionId;
    using Callback     = std::function<void(Args...)>;

private:
    struct SignalEntry {
        std::shared_ptr<Connection::State> state;
        Callback callback;
        const void* receiverIdentity{nullptr};
        const void* slotIdentity{nullptr};
    };

    using ConnectionList = std::vector<SignalEntry>;
    using Snapshot       = std::shared_ptr<const ConnectionList>;

    struct SignalState {
        SignalState()
            : connections(std::make_shared<const ConnectionList>())
        {
        }

        ~SignalState() = default;

        SignalState(const SignalState&) = delete;
        SignalState& operator=(const SignalState&) = delete;
        SignalState(SignalState&&) = delete;
        SignalState& operator=(SignalState&&) = delete;

        std::atomic<Snapshot> connections;
        std::mutex writeMutex;
        std::atomic<std::size_t> connectionCount{0};
        std::atomic<std::size_t> singleShotCount{0};
        ConnectionId nextConnectionId{1};
        bool alive{true};
    };

public:
    Signal()
        : m_state(std::make_shared<SignalState>()),
          m_control(std::make_shared<Connection::Control>(
              [weakState = std::weak_ptr<SignalState>{m_state}](ConnectionId connectionId) {
                  if (const auto state = weakState.lock())
                      disconnectState(state, connectionId);
              }))
    {
    }

    ~Signal()
    {
        invalidateState();
        m_control.reset();
    }

    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;
    Signal(Signal&&) = delete;
    Signal& operator=(Signal&&) = delete;

    [[nodiscard]] Connection connect(Callback callback, ConnectionFlag flags = ConnectionFlag::None)
    {
        return connectImpl(std::move(callback), flags, nullptr, nullptr);
    }

    void disconnect(ConnectionId id)
    {
        disconnectState(m_state, id);
    }

    void disconnect(const Connection& connection)
    {
        disconnect(connection.id());
    }

    void disconnectAll()
    {
        disconnectAllState(m_state);
    }

    void emit(Args... args) const
    {
        if (m_state->connectionCount.load(std::memory_order_relaxed) == 0)
            return;

        if (m_blockState && m_blockState->load(std::memory_order_relaxed))
            return;

        const Snapshot connections = m_state->connections.load(std::memory_order_acquire);

        if (m_state->singleShotCount.load(std::memory_order_relaxed) == 0) {
            for (const auto& entry : *connections) {
                if (entry.callback)
                    entry.callback(args...);
            }

            return;
        }

        emitWithSingleShot(connections, args...);
    }

    void operator()(Args... args) const
    {
        emit(std::forward<Args>(args)...);
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return m_state->connectionCount.load(std::memory_order_relaxed) == 0;
    }

    [[nodiscard]] std::size_t connectionCount() const noexcept
    {
        return m_state->connectionCount.load(std::memory_order_relaxed);
    }

private:
    void bindBlockState(const std::atomic<bool>* blockState) noexcept
    {
        m_blockState = blockState;
    }

    [[nodiscard]] Connection connectImpl(
        Callback callback,
        ConnectionFlag flags,
        const void* receiverIdentity,
        const void* slotIdentity)
    {
        std::lock_guard<std::mutex> lock(m_state->writeMutex);

        if (!m_state->alive)
            return {};

        const Snapshot current = m_state->connections.load(std::memory_order_acquire);

        if (hasConnectionFlag(flags, ConnectionFlag::Unique)) {
            if (!receiverIdentity || !slotIdentity)
                return {};

            const auto duplicate = std::find_if(
                current->begin(),
                current->end(),
                [receiverIdentity, slotIdentity](const SignalEntry& entry) {
                    return entry.receiverIdentity == receiverIdentity &&
                           entry.slotIdentity == slotIdentity;
                });

            if (duplicate != current->end())
                return {};
        }

        const ConnectionId id = m_state->nextConnectionId++;
        Connection connection(id, flags, m_control);

        auto next = std::make_shared<ConnectionList>(*current);

        next->push_back(SignalEntry{
            .state = connection.m_state,
            .callback = std::move(callback),
            .receiverIdentity = receiverIdentity,
            .slotIdentity = slotIdentity
        });

        if (hasConnectionFlag(flags, ConnectionFlag::SingleShot))
            m_state->singleShotCount.fetch_add(1, std::memory_order_release);

        publish(m_state, std::move(next));
        m_state->connectionCount.fetch_add(1, std::memory_order_release);

        return connection;
    }

    static void publish(const std::shared_ptr<SignalState>& state, std::shared_ptr<ConnectionList> connections)
    {
        Snapshot snapshot = std::move(connections);
        state->connections.store(std::move(snapshot), std::memory_order_release);
    }

    static void disconnectState(const std::shared_ptr<SignalState>& state, ConnectionId id)
    {
        std::lock_guard<std::mutex> lock(state->writeMutex);

        if (!state->alive)
            return;

        const Snapshot current = state->connections.load(std::memory_order_acquire);

        const auto it = std::find_if(current->begin(), current->end(), [id](const SignalEntry& entry) {
            return entry.state && entry.state->id == id;
        });

        if (it == current->end())
            return;

        const auto disconnectedState = it->state;
        const bool singleShot =
            disconnectedState &&
            hasConnectionFlag(disconnectedState->flags, ConnectionFlag::SingleShot);

        auto next = std::make_shared<ConnectionList>();
        next->reserve(current->size() - 1);

        for (const auto& entry : *current) {
            if (!entry.state || entry.state->id != id)
                next->push_back(entry);
        }

        publish(state, std::move(next));
        state->connectionCount.fetch_sub(1, std::memory_order_release);

        if (singleShot)
            state->singleShotCount.fetch_sub(1, std::memory_order_release);

        if (disconnectedState)
            disconnectedState->connected.store(false, std::memory_order_release);
    }

    static void disconnectAllState(const std::shared_ptr<SignalState>& state)
    {
        std::lock_guard<std::mutex> lock(state->writeMutex);

        if (!state->alive)
            return;

        const Snapshot current = state->connections.load(std::memory_order_acquire);

        if (current->empty())
            return;

        state->connections.store(std::make_shared<const ConnectionList>(), std::memory_order_release);
        state->connectionCount.store(0, std::memory_order_release);
        state->singleShotCount.store(0, std::memory_order_release);

        for (const auto& entry : *current) {
            if (entry.state)
                entry.state->connected.store(false, std::memory_order_release);
        }
    }

    void emitWithSingleShot(const Snapshot& connections, Args... args) const
    {
        for (const auto& entry : *connections) {
            if (!entry.callback)
                continue;

            if (!entry.state ||
                !hasConnectionFlag(entry.state->flags, ConnectionFlag::SingleShot)) {
                entry.callback(args...);
                continue;
            }

            bool expected = false;

            if (!entry.state->singleShotClaimed.compare_exchange_strong(
                    expected,
                    true,
                    std::memory_order_acq_rel,
                    std::memory_order_relaxed)) {
                continue;
            }

            disconnectState(m_state, entry.state->id);
            entry.callback(args...);
        }
    }

    void invalidateState() noexcept
    {
        std::lock_guard<std::mutex> lock(m_state->writeMutex);

        m_state->alive = false;

        const Snapshot connections = m_state->connections.load(std::memory_order_acquire);

        for (const auto& entry : *connections) {
            if (entry.state)
                entry.state->connected.store(false, std::memory_order_release);
        }

        m_state->connections.store(std::make_shared<const ConnectionList>(), std::memory_order_release);
        m_state->connectionCount.store(0, std::memory_order_release);
        m_state->singleShotCount.store(0, std::memory_order_release);
    }

    template <typename T>
    friend struct SignalConnection;

    std::shared_ptr<SignalState> m_state;
    std::shared_ptr<Connection::Control> m_control;
    const std::atomic<bool>* m_blockState{nullptr};
};

// =============================================================================
// Reflected Object Connection
// =============================================================================

template <typename... Args>
struct SignalConnection<Signal<Args...>> {
    template <auto SlotMember, ObjectType Receiver>
    [[nodiscard]] static Connection bind(
        Signal<Args...>& signal,
        Receiver& receiver,
        const std::atomic<bool>* blockState,
        ConnectionFlag flags = ConnectionFlag::None)
    {
        static_assert(std::is_member_function_pointer_v<decltype(SlotMember)>);
        static_assert(std::is_invocable_v<decltype(SlotMember), Receiver&, Args...>);

        static constexpr unsigned char SlotIdentity = 0;

        signal.bindBlockState(blockState);

        Connection connection = signal.connectImpl(
            [&receiver](Args... args) {
                std::invoke(SlotMember, receiver, std::forward<Args>(args)...);
            },
            flags,
            std::addressof(receiver),
            std::addressof(SlotIdentity));

        if (connection)
            receiver.registerConnection(connection);

        return connection;
    }
};

template <auto SignalMember, auto SlotMember, ObjectType Sender, ObjectType Receiver>
[[nodiscard]] Connection connect(
    Sender& sender,
    Receiver& receiver,
    ConnectionFlag flags = ConnectionFlag::None)
{
    using SignalMemberPointer = decltype(SignalMember);
    using SlotMemberPointer   = decltype(SlotMember);

    static_assert(std::is_member_object_pointer_v<SignalMemberPointer>);
    static_assert(std::is_member_function_pointer_v<SlotMemberPointer>);

    auto& signal = sender.*SignalMember;

    using ReflectedSignalType = std::remove_cvref_t<decltype(signal)>;

    static_assert(SignalType<ReflectedSignalType>);

    return SignalConnection<ReflectedSignalType>::template bind<SlotMember>(
        signal,
        receiver,
        sender.signalBlockState(),
        flags);
}

} // namespace job::core