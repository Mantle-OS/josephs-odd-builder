#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>


class Object
{
public:
    using ConnectionId = std::uint64_t;


    Object() = default;

    virtual ~Object()
    {
        disconnectAll();
    }


    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;

    Object(Object&&) = delete;
    Object& operator=(Object&&) = delete;


    ConnectionId registerConnection(
        std::function<void()> disconnect)
    {
        const ConnectionId id = m_nextConnectionId++;
        m_connections.push_back(Connection{
            .id = id,
            .disconnect = std::move(disconnect)
        });

        return id;
    }


    void disconnect(ConnectionId id)
    {
        const auto it = std::find_if( m_connections.begin(), m_connections.end(), [id](const Connection& connection) {
            return connection.id == id;
        });

        if (it == m_connections.end())
            return;

        if (it->disconnect)
            it->disconnect();

        m_connections.erase(it);
    }


    void disconnectAll()
    {
        for (auto &connection : m_connections) {
            if (connection.disconnect)
                connection.disconnect();
        }

        m_connections.clear();
    }


private:
    struct Connection
    {
        ConnectionId id{0};
        std::function<void()> disconnect;
    };


    std::vector<Connection> m_connections;
    ConnectionId m_nextConnectionId{1};
};