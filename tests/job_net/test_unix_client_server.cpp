#include <unistd.h>

#include <thread>
#include <atomic>
#include <cstring>
#include <filesystem>

#include "test_loop.h"
namespace fs = std::filesystem;

TEST_CASE("UnixClient and UnixServer Full Echo Test", "[unix_client_server][async][echo]")
{
    TestLoop loop;
    auto path = make_temp_sock_path("unix_cs_echo");
    auto server = std::make_shared<UnixServer>(loop.loop);

    const std::string testMessage = "Hello, Unix Server!";
    std::atomic<bool> clientConnected{false};
    std::atomic<bool> clientGotEcho{false};
    std::atomic<bool> serverGotMessage{false};
    std::atomic<bool> clientWasDisconnected{false};
    std::atomic<bool> serverSawClientDisconnect{false};

    server->onClientConnected = [&](UnixClient::Ptr client) {
        JOB_LOG_INFO("[UNIX][Server] Client connected!");

        client->onMessage = [client, &serverGotMessage](const char* data, size_t len) {
            std::string msg(data, len);
            JOB_LOG_INFO("[UNIX][Server] Received '{}', echoing...", msg);
            serverGotMessage.store(true);
            client->send(data, len); // Echo back
        };

        client->onDisconnect = [&]() {
            JOB_LOG_INFO("[UNIX][Server] Client disconnected.");
            serverSawClientDisconnect.store(true);
        };
    };

    REQUIRE(server->start(path, 0));
    REQUIRE(server->isRunning());
    REQUIRE(fs::exists(path));

    auto client = std::make_shared<UnixClient>(loop.loop);

    client->onConnect = [&]() {
        JOB_LOG_INFO("[UNIX][Client] Connected! Sending message...");
        clientConnected.store(true);
        client->send(testMessage);
    };

    client->onMessage = [client, &testMessage, &clientGotEcho](const char* data, size_t len) {
        std::string msg(data, len);
        JOB_LOG_INFO("[UNIX][Client] Received echo: '{}'", msg);
        if (msg == testMessage) {
            clientGotEcho.store(true);
        }
        client->disconnect(); // We're done
    };

    client->onDisconnect = [&]() {
        JOB_LOG_INFO("[UNIX][Client] Disconnected.");
        clientWasDisconnected.store(true);
    };

    JobUrl url("unix://" + path);
    REQUIRE(client->connectToHost(url));

    int retries = 0;
    while (!clientWasDisconnected.load() && retries < 200) { // 200ms timeout
        std::this_thread::sleep_for(1ms);
        retries++;
    }

    JOB_LOG_INFO("[UNIX] Test finished. ServerRead: {}, ClientEcho: {}, ClientDisconnect: {}, ServerDisconnect: {}",
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
    REQUIRE_FALSE(fs::exists(path));
}
// CHECKPOINT: v1.1
