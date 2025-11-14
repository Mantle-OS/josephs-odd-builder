#include "test_loop.h"
#include <thread>
#include <atomic>
#include <cstring>

TEST_CASE("UdpClient and UdpServer Full Echo Test", "[udp_client_server][async][echo]")
{
    TestLoop loop;
    JobUrl url("udp://127.0.0.1:0");
    const std::string testMessage = "Hello, UDP J.O.B.!";

    auto server = std::make_shared<UdpServer>(loop.loop);
    auto client = std::make_shared<UdpClient>(loop.loop);

    // test factors
    std::atomic<bool> clientConnected{false};
    std::atomic<bool> clientGotEcho{false};
    std::atomic<bool> serverGotMessage{false};

    server->onMessage = [server, &serverGotMessage, &testMessage](const char* data, size_t len, const JobIpAddr& sender) {
        std::string msg(data, len);
        if (msg == testMessage) {
            serverGotMessage.store(true);
            ssize_t written = server->sendTo(data, len, sender);
            JOB_LOG_INFO("[TEST][Server] Echoed {} bytes back to sender", written);
        }
    };

    REQUIRE(server->start(url));
    client->onConnect = [&]() {
        clientConnected.store(true);
        client->send(testMessage);
    };

    client->onMessage = [client, &testMessage, &clientGotEcho](const char* data, size_t len) {
        std::string msg(data, len);
        if (msg == testMessage)
            clientGotEcho.store(true);
    };

    uint16_t serverPort = server->port();
    JobUrl clientUrl("udp://127.0.0.1:" + std::to_string(serverPort));
    REQUIRE(client->connectToHost(clientUrl));
    int retries = 0;
    while ((!serverGotMessage.load() || !clientGotEcho.load()) && retries < 100) {
        std::this_thread::sleep_for(1ms);
        retries++;
    }


    REQUIRE(clientConnected.load() == true);
    REQUIRE(serverGotMessage.load() == true);
    REQUIRE(clientGotEcho.load() == true);

    if(client->isConnected())
        client->disconnect();

    if(server->isRunning())
        server->stop();

}
// CHECKPOINT: v1.0
