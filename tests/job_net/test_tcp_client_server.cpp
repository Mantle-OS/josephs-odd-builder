#include "test_loop.h"

#include <atomic>
#include <cstring>
#include <thread>

#include <ctx/job_fifo_ctx.h>
#include <resolve/job_resolver.h>

using job::threads::FifoScheduler;

TEST_CASE("TcpClient and TcpServer Full Echo Test", "[tcp_client_server][async][echo]")
{
    TestLoop loop;

    std::atomic<bool> clientConnected{false};
    std::atomic<bool> clientGotEcho{false};
    std::atomic<bool> serverGotMessage{false};
    std::atomic<bool> clientWasDisconnected{false};
    std::atomic<bool> serverSawClientDisconnect{false};
    std::atomic<bool> clientWriteOk{false};
    std::atomic<bool> serverWriteOk{false};

    const std::string testMessage = "Hello, J.O.B.!";

    auto server = std::make_shared<TcpServer>(loop.loop);

    server->onClientConnected = [&](job::net::TcpClient::Ptr client) {
        client->onMessage = [client, &serverGotMessage, &serverWriteOk](const char *data, size_t len) {
            std::string msg(data, len);
            serverGotMessage.store(true);

            const NetIoResult result = client->send(data, len);
            serverWriteOk.store(result.status == NetIoStatus::Ok && result.bytes == len);
        };

        client->onDisconnect = [&]() {
            serverSawClientDisconnect.store(true);
        };
    };

    REQUIRE(server->start("127.0.0.1", 0));

    const uint16_t port = server->port();
    REQUIRE(port > 0);

    auto client = std::make_shared<TcpClient>(loop.loop);

    // Weak capture: this lambda is stored as client->onMessage, so capturing
    // client by value would make it hold a strong reference to itself through
    // its own callback.
    std::weak_ptr<TcpClient> weakClient = client;

    client->onConnect = [&]() {
        clientConnected.store(true);

        const NetIoResult result = client->send(testMessage);
        clientWriteOk.store(result.status == NetIoStatus::Ok && result.bytes == testMessage.size());
    };

    client->onMessage = [weakClient, &testMessage, &clientGotEcho](const char *data, size_t len) {
        auto client = weakClient.lock();
        if (!client)
            return;

        std::string msg(data, len);

        INFO("[TCP][Client] Received echo: " << msg);

        if (msg == testMessage)
            clientGotEcho.store(true);

        client->disconnect();
    };

    client->onDisconnect = [&]() {
        INFO("[TCP][Client] Disconnected.");
        clientWasDisconnected.store(true);
    };

    // Numeric host — connectToHost(JobIpAddr) directly. No resolver involved;
    // the JobUrl/resolver path gets its own dedicated coverage below.
    JobIpAddr addr("127.0.0.1", port);

    REQUIRE(client->connectToHost(addr));

    int retries = 0;

    while (!clientWasDisconnected.load() && retries < 200) {
        std::this_thread::sleep_for(1ms);
        ++retries;
    }

    JOB_LOG_INFO("[TEST] Test finished. ServerRead: {}, ClientEcho: {}, ClientDisconnect: {}, ServerDisconnect: {}",
                 serverGotMessage.load(),
                 clientGotEcho.load(),
                 clientWasDisconnected.load(),
                 serverSawClientDisconnect.load());

    REQUIRE(clientConnected.load());
    REQUIRE(clientWriteOk.load());
    REQUIRE(serverGotMessage.load());
    REQUIRE(serverWriteOk.load());
    REQUIRE(clientGotEcho.load());
    REQUIRE(clientWasDisconnected.load());

    retries = 0;

    while (!serverSawClientDisconnect.load() && retries < 50) {
        std::this_thread::sleep_for(1ms);
        ++retries;
    }

    REQUIRE(serverSawClientDisconnect.load());

    server->stop();
}

TEST_CASE("TcpClient connects via JobUrl through a real JobResolver", "[tcp_client_server][resolver][async]")
{
    TestLoop loop;

    // Small fixed pool — DNS resolution is latency-bound, not throughput-bound,
    // so a couple of threads is plenty for this test's single lookup.
    JobFifoCtx resolverCtx(2);
    auto resolver = JobResolver::create(loop.loop, resolverCtx.pool);

    std::atomic<bool> clientConnected{false};
    std::atomic<bool> clientGotEcho{false};
    std::atomic<bool> serverGotMessage{false};
    std::atomic<bool> clientWasDisconnected{false};
    std::atomic<bool> serverSawClientDisconnect{false};
    std::atomic<bool> clientWriteOk{false};
    std::atomic<bool> serverWriteOk{false};

    const std::string testMessage = "Hello via URL!";

    auto server = std::make_shared<TcpServer>(loop.loop);

    server->onClientConnected = [&](job::net::TcpClient::Ptr client) {
        client->onMessage = [client, &serverGotMessage, &serverWriteOk](const char *data, size_t len) {
            serverGotMessage.store(true);

            const NetIoResult result = client->send(data, len);
            serverWriteOk.store(result.status == NetIoStatus::Ok && result.bytes == len);
        };

        client->onDisconnect = [&]() {
            serverSawClientDisconnect.store(true);
        };
    };

    // [[BACKLOG]] Bound to "::" (dual-stack wildcard), not "127.0.0.1", as a
    // stopgap for a real gap in ISocketIO::connectToHost(JobUrl)'s candidate
    // loop: it only handles synchronous per-candidate rejection, not an async
    // failure on one candidate falling through to the next.
    //
    // "localhost" resolves to both ::1 and 127.0.0.1. If IPv6 sorts first and
    // only IPv4 has a listener, the ::1 attempt starts async, connectToHost()
    // sees true and stops trying further candidates, and the attempt then fails
    // later with nothing left to fall back to.
    //
    // Binding dual-stack here sidesteps it by making sure whichever family
    // resolves first actually has something listening.
    //
    // Real fix: connectToHost(JobUrl) needs to retain the remaining candidate
    // list and advance on the active attempt's asynchronous onError, not only
    // on the initial synchronous return value.
    REQUIRE(server->start("::", 0));

    const uint16_t port = server->port();
    REQUIRE(port > 0);

    auto client = std::make_shared<TcpClient>(loop.loop, resolver);

    std::weak_ptr<TcpClient> weakClient = client;

    client->onConnect = [&]() {
        clientConnected.store(true);

        const NetIoResult result = client->send(testMessage);
        clientWriteOk.store(result.status == NetIoStatus::Ok && result.bytes == testMessage.size());
    };

    client->onMessage = [weakClient, &testMessage, &clientGotEcho](const char *data, size_t len) {
        auto client = weakClient.lock();
        if (!client)
            return;

        std::string msg(data, len);

        INFO("[TCP][Client] Received echo: " << msg);

        if (msg == testMessage)
            clientGotEcho.store(true);

        client->disconnect();
    };

    client->onDisconnect = [&]() {
        clientWasDisconnected.store(true);
    };

    JobUrl url("tcp://localhost:" + std::to_string(port));

    REQUIRE(client->connectToHost(url));

    int retries = 0;

    while (!clientWasDisconnected.load() && retries < 500) {
        std::this_thread::sleep_for(1ms);
        ++retries;
    }

    REQUIRE(clientConnected.load());
    REQUIRE(clientWriteOk.load());
    REQUIRE(serverGotMessage.load());
    REQUIRE(serverWriteOk.load());
    REQUIRE(clientGotEcho.load());
    REQUIRE(clientWasDisconnected.load());

    retries = 0;

    while (!serverSawClientDisconnect.load() && retries < 50) {
        std::this_thread::sleep_for(1ms);
        ++retries;
    }

    REQUIRE(serverSawClientDisconnect.load());

    server->stop();
}