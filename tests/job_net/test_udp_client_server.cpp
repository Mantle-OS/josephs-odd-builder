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

    // Weak captures: these lambdas are stored as members on `server`/`client`
    // themselves, so capturing by value would make each hold a strong reference to
    // itself through its own callback — same leak pattern fixed on TcpServer/TcpClient
    // (and UnixServer/UnixClient) earlier in this session.
    std::weak_ptr<UdpServer> weakServer = server;
    server->onMessage = [weakServer, &serverGotMessage, &testMessage](const char* data, size_t len, const JobIpAddr& sender) {
        auto server = weakServer.lock();
        if (!server)
            return;
        std::string msg(data, len);
        if (msg == testMessage) {
            serverGotMessage.store(true);
            ssize_t written = server->sendTo(data, len, sender);
            JOB_LOG_INFO("[TEST][Server] Echoed {} bytes back to sender", written);
        }
    };
    REQUIRE(server->start(url));

    std::weak_ptr<UdpClient> weakClient = client;
    client->onConnect = [&]() {
        clientConnected.store(true);
        client->send(testMessage);
    };
    client->onMessage = [weakClient, &testMessage, &clientGotEcho](const char* data, size_t len) {
        auto client = weakClient.lock();
        if (!client)
            return;
        std::string msg(data, len);
        if (msg == testMessage)
            clientGotEcho.store(true);
    };
    uint16_t serverPort = server->port();
    // Numeric host — connectToHost(JobIpAddr) directly. No resolver set up on this
    // client, and both endpoints here are numeric IPs anyway, so there's nothing for
    // DNS to do (same reasoning as the TcpSocket/TcpClient tests).
    JobIpAddr clientAddr("127.0.0.1", serverPort);
    REQUIRE(client->connectToHost(clientAddr));
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