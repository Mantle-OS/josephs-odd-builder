#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

template <typename... Args>
class Signal
{
public:
    using ConnectionId = std::uint64_t;
    using Callback = std::function<void(Args...)>;

    Signal() = default;
    Signal(const Signal&) = delete;
    Signal &operator=(const Signal&) = delete;
    Signal(Signal&&) = delete;
    Signal &operator=(Signal&&) = delete;


    ConnectionId connect(Callback callback)
    {
        const ConnectionId id = m_nextConnectionId++;
        m_callbacks.push_back(Connection{
            .id = id,
            .callback = std::move(callback)
        });

        return id;
    }


    void disconnect(ConnectionId id)
    {
        const auto it = std::find_if(m_callbacks.begin(), m_callbacks.end(), [id](const Connection& connection){
            return connection.id == id;
        });

        if (it == m_callbacks.end())
            return;

        m_callbacks.erase(it);
    }


    void disconnectAll()
    {
        m_callbacks.clear();
    }


    void emit(Args... args){
        for (auto& connection : m_callbacks) {
            if (connection.callback)
                connection.callback(std::forward<Args>(args)...);
        }
    }


    [[nodiscard]] bool empty() const
    {
        return m_callbacks.empty();
    }


    [[nodiscard]] std::size_t connectionCount() const
    {
        return m_callbacks.size();
    }


private:
    struct Connection
    {
        ConnectionId    id{0};
        Callback        callback;
    };
    std::vector<Connection> m_callbacks;
    ConnectionId m_nextConnectionId{1};
};

