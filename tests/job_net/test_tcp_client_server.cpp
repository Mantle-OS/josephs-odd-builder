#include "test_loop.h"
#include <thread>
#include <atomic>
#include <cstring>

TEST_CASE("TcpClient and TcpServer Full Echo Test", "[tcp_client_server][async][echo]")
{
    TestLoop loop;

    std::atomic<bool> clientConnected{false};
    std::atomic<bool> clientGotEcho{false};
    std::atomic<bool> serverGotMessage{false};
    std::atomic<bool> clientWasDisconnected{false};
    std::atomic<bool> serverSawClientDisconnect{false};

    const std::string testMessage = "Hello, J.O.B.!";
    auto server = std::make_shared<TcpServer>(loop.loop);

    server->onClientConnected = [&](job::net::TcpServer::ClientPtr client) {
        client->onMessage = [client, &serverGotMessage](const char* data, size_t len) {
            std::string msg(data, len);
            serverGotMessage.store(true);
            client->send(data, len); // Echo back
        };

        client->onDisconnect = [&]() {
            serverSawClientDisconnect.store(true);
        };
    };

    REQUIRE(server->start("127.0.0.1", 0));
    uint16_t port = server->port();
    REQUIRE(port > 0);

    auto client = std::make_shared<TcpClient>(loop.loop);

    client->onConnect = [&]() {
        clientConnected.store(true);
        client->send(testMessage);
    };

    client->onMessage = [client, &testMessage, &clientGotEcho](const char* data, size_t len) {
        std::string msg(data, len);
        INFO("[TCP][Client] Received echo: " << msg);
        if (msg == testMessage)
            clientGotEcho.store(true);
        client->disconnect(); // We're done, close the connection
    };

    client->onDisconnect = [&]() {
        INFO("[TCP][Client] Disconnected.");
        clientWasDisconnected.store(true);
    };

    JobUrl url("tcp://127.0.0.1:" + std::to_string(port));
    REQUIRE(client->connectToHost(url));

    int retries = 0;
    while (!clientWasDisconnected.load() && retries < 200) { // 200ms timeout
        std::this_thread::sleep_for(1ms);
        retries++;
    }

    JOB_LOG_INFO("[TEST] Test finished. ServerRead: {}, ClientEcho: {}, ClientDisconnect: {}, ServerDisconnect: {}",
                 serverGotMessage.load(), clientGotEcho.load(), clientWasDisconnected.load(), serverSawClientDisconnect.load());

    REQUIRE(clientConnected.load() == true);
    REQUIRE(serverGotMessage.load() == true);
    REQUIRE(clientGotEcho.load() == true);
    REQUIRE(clientWasDisconnected.load() == true);

    retries = 0;
    while (!serverSawClientDisconnect.load() && retries < 50) {
        std::this_thread::sleep_for(1ms);
        retries++;
    }
    REQUIRE(serverSawClientDisconnect.load() == true);

    server->stop();
}
// CHECKPOINT: v1.0
