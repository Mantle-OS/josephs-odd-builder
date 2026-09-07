#include <unistd.h>

#include <atomic>
#include <cstring>
#include <filesystem>
#include <thread>

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
    std::atomic<bool> clientWriteOk{false};
    std::atomic<bool> serverWriteOk{false};

    server->onClientConnected = [&](UnixClient::Ptr client) {
        JOB_LOG_INFO("[UNIX][Server] Client connected!");

        client->onMessage = [client, &serverGotMessage, &serverWriteOk](const char *data, size_t len) {
            const std::string msg(data, len);

            JOB_LOG_INFO("[UNIX][Server] Received '{}', echoing...", msg);

            serverGotMessage.store(true);

            const NetIoResult result = client->send(data, len);
            serverWriteOk.store(result.status == NetIoStatus::Ok && result.bytes == len);
        };

        client->onDisconnect = [&]() {
            JOB_LOG_INFO("[UNIX][Server] Client disconnected.");
            serverSawClientDisconnect.store(true);
        };
    };

    REQUIRE(server->start(path, 0));
    REQUIRE(server->isRunning());
    REQUIRE(fs::exists(path));

    auto client = UnixClient::create(loop.loop);
    std::weak_ptr<UnixClient> weakClient = client;

    client->onConnect = [&]() {
        JOB_LOG_INFO("[UNIX][Client] Connected! Sending message...");

        clientConnected.store(true);

        const NetIoResult result = client->send(testMessage);
        clientWriteOk.store(result.status == NetIoStatus::Ok && result.bytes == testMessage.size());
    };

    client->onMessage = [weakClient, &testMessage, &clientGotEcho](const char *data, size_t len) {
        auto client = weakClient.lock();
        if (!client)
            return;

        const std::string msg(data, len);

        JOB_LOG_INFO("[UNIX][Client] Received echo: '{}'", msg);

        if (msg == testMessage)
            clientGotEcho.store(true);

        client->disconnect();
    };

    client->onDisconnect = [&]() {
        JOB_LOG_INFO("[UNIX][Client] Disconnected.");
        clientWasDisconnected.store(true);
    };

    JobUrl url("unix://" + path);

    REQUIRE(client->connectToHost(url));

    int retries = 0;

    while (!clientWasDisconnected.load() && retries < 200) {
        std::this_thread::sleep_for(1ms);
        ++retries;
    }

    JOB_LOG_INFO("[UNIX] Test finished. ServerRead: {}, ClientEcho: {}, ClientDisconnect: {}, ServerDisconnect: {}",
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

    REQUIRE_FALSE(fs::exists(path));
}