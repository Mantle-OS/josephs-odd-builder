#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include <clients/unix_socket_client.h>
#include <servers/unix_socket_server.h>
#include <job_url.h>

#include "packed.h"

class PackedUnixReader
{
public:
    using Ptr = std::shared_ptr<PackedUnixReader>;
    using WPtr = std::weak_ptr<PackedUnixReader>;
    using UPtr = std::unique_ptr<PackedUnixReader>;
    using Callback = std::function<void(const Packed &)>;

    PackedUnixReader() = default;
    ~PackedUnixReader() = default;

    PackedUnixReader(const PackedUnixReader &) = delete;
    PackedUnixReader &operator=(const PackedUnixReader &) = delete;
    PackedUnixReader(PackedUnixReader &&) = delete;
    PackedUnixReader &operator=(PackedUnixReader &&) = delete;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<PackedUnixReader>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<PackedUnixReader>();
    }

    void setCallback(Callback callback)
    {
        m_callback = std::move(callback);
    }

    void read(const char *data, std::size_t len)
    {
        while (len > 0) {
            const std::size_t remaining = sizeof(Packed) - m_offset;
            const std::size_t count = std::min(remaining, len);

            std::memcpy(m_buffer.data() + m_offset, data, count);

            m_offset += count;
            data += count;
            len -= count;

            if (m_offset != sizeof(Packed))
                continue;

            Packed packed;
            std::memcpy(&packed, m_buffer.data(), sizeof(Packed));
            m_offset = 0;

            if (m_callback)
                m_callback(packed);
        }
    }

    void reset() noexcept
    {
        m_offset = 0;
    }

    [[nodiscard]] static const char *data(const Packed &packed) noexcept
    {
        return reinterpret_cast<const char *>(&packed);
    }

    [[nodiscard]] static constexpr std::size_t size() noexcept
    {
        return sizeof(Packed);
    }

private:
    std::array<char, sizeof(Packed)> m_buffer{};
    std::size_t m_offset{0};
    Callback m_callback;
};

class PackedUnixClient
{
public:
    using Ptr = std::shared_ptr<PackedUnixClient>;
    using WPtr = std::weak_ptr<PackedUnixClient>;
    using UPtr = std::unique_ptr<PackedUnixClient>;
    using PackedCallback = std::function<void(const Packed &)>;

    explicit PackedUnixClient(job::threads::JobIoAsyncThread::Ptr loop) :
        m_client(job::net::UnixClient::create(std::move(loop))),
        m_reader(PackedUnixReader::createShared())
    {
        m_reader->setCallback([this](const Packed &packed) {
            if (onPacked)
                onPacked(packed);
        });

        m_client->onConnect = [this]() {
            if (onConnect)
                onConnect();
        };

        m_client->onMessage = [this](const char *data, std::size_t len) {
            m_reader->read(data, len);
        };

        m_client->onDisconnect = [this]() {
            if (onDisconnect)
                onDisconnect();
        };

        m_client->onError = [this](int error) {
            if (onError)
                onError(error);
        };
    }

    ~PackedUnixClient() = default;

    PackedUnixClient(const PackedUnixClient &) = delete;
    PackedUnixClient &operator=(const PackedUnixClient &) = delete;
    PackedUnixClient(PackedUnixClient &&) = delete;
    PackedUnixClient &operator=(PackedUnixClient &&) = delete;

    [[nodiscard]] static Ptr createShared(job::threads::JobIoAsyncThread::Ptr loop)
    {
        return std::make_shared<PackedUnixClient>(std::move(loop));
    }

    [[nodiscard]] static UPtr createUniq(job::threads::JobIoAsyncThread::Ptr loop)
    {
        return std::make_unique<PackedUnixClient>(std::move(loop));
    }

    [[nodiscard]] bool connect(const std::string &path)
    {
        return m_client->connectToHost(job::net::JobUrl("unix://" + path));
    }

    void disconnect()
    {
        m_client->disconnect();
    }

    [[nodiscard]] ssize_t send(const Packed &packed)
    {
        return m_client->send(PackedUnixReader::data(packed), PackedUnixReader::size());
    }

    [[nodiscard]] bool isConnected() const noexcept
    {
        return m_client->isConnected();
    }

    std::function<void()> onConnect;
    PackedCallback onPacked;
    std::function<void()> onDisconnect;
    std::function<void(int)> onError;

private:
    job::net::UnixClient::Ptr m_client;
    PackedUnixReader::Ptr m_reader;
};

class PackedUnixServer
{
public:
    using Ptr = std::shared_ptr<PackedUnixServer>;
    using WPtr = std::weak_ptr<PackedUnixServer>;
    using UPtr = std::unique_ptr<PackedUnixServer>;
    using PackedCallback = std::function<void(job::net::UnixClient::Ptr, const Packed &)>;

    explicit PackedUnixServer(job::threads::JobIoAsyncThread::Ptr loop) :
        m_server(std::make_shared<job::net::UnixServer>(std::move(loop)))
    {
        m_server->onClientConnected = [this](job::net::UnixClient::Ptr client) {
            auto reader = PackedUnixReader::createShared();

            reader->setCallback([this, client](const Packed &packed) {
                if (onPacked)
                    onPacked(client, packed);
            });

            client->onMessage = [reader](const char *data, std::size_t len) {
                reader->read(data, len);
            };

            client->onDisconnect = [this, client]() {
                if (onClientDisconnected)
                    onClientDisconnected(client);
            };

            if (onClientConnected)
                onClientConnected(client);
        };
    }

    ~PackedUnixServer() = default;

    PackedUnixServer(const PackedUnixServer &) = delete;
    PackedUnixServer &operator=(const PackedUnixServer &) = delete;
    PackedUnixServer(PackedUnixServer &&) = delete;
    PackedUnixServer &operator=(PackedUnixServer &&) = delete;

    [[nodiscard]] static Ptr createShared(job::threads::JobIoAsyncThread::Ptr loop)
    {
        return std::make_shared<PackedUnixServer>(std::move(loop));
    }

    [[nodiscard]] static UPtr createUniq(job::threads::JobIoAsyncThread::Ptr loop)
    {
        return std::make_unique<PackedUnixServer>(std::move(loop));
    }

    [[nodiscard]] bool start(const std::string &path, int backlog = 5)
    {
        return m_server->start(path, backlog);
    }

    void stop()
    {
        m_server->stop();
    }

    [[nodiscard]] bool isRunning() const noexcept
    {
        return m_server->isRunning();
    }

    [[nodiscard]] std::string path() const noexcept
    {
        return m_server->path();
    }

    [[nodiscard]] static ssize_t send(const job::net::UnixClient::Ptr &client, const Packed &packed)
    {
        if (!client)
            return -1;

        return client->send(PackedUnixReader::data(packed), PackedUnixReader::size());
    }

    std::function<void(job::net::UnixClient::Ptr)> onClientConnected;
    PackedCallback onPacked;
    std::function<void(job::net::UnixClient::Ptr)> onClientDisconnected;

private:
    job::net::UnixServer::Ptr m_server;
};