#include "test_loop.h"
#include <thread>
#include <atomic>
#include <cstring>
TEST_CASE("UdpSocket basic bind", "[udp_socket][bind]") {
    TestLoop loop;
    auto sock = UdpSocket::create(loop.loop);
    JobIpAddr bindAddr("127.0.0.1", 0);
    REQUIRE(sock->bind(bindAddr));
    REQUIRE(sock->fd() != -1);
    // A bound-but-not-connect()'d UDP socket has no default peer — SocketState::Bound
    // is correct here now, distinct from Connected (fixed earlier this session; the
    // old assertion here was asserting the bug's behavior, not the intended one).
    REQUIRE(sock->state() == ISocketIO::SocketState::Bound);
    uint16_t port = sock->localPort();
    REQUIRE(port > 0);
    REQUIRE(sock->localAddress() == "127.0.0.1");
    sock->disconnect();
    REQUIRE(sock->state() == ISocketIO::SocketState::Closed);
}
TEST_CASE("UdpSocket connectToHost", "[udp_socket][connect]") {
    TestLoop loop;
    auto sock = UdpSocket::create(loop.loop);
    // Numeric host — connectToHost(JobIpAddr) directly, same reasoning as the TcpSocket
    // tests: no resolver set up here, and ISocketIO::connectToHost(JobUrl) now
    // unconditionally requires one.
    JobIpAddr addr("127.0.0.1", 9999);
    REQUIRE(sock->connectToHost(addr));
    REQUIRE(sock->fd() != -1);
    REQUIRE(sock->state() == ISocketIO::SocketState::Connected);
    REQUIRE(sock->peerAddress() == "127.0.0.1");
    REQUIRE(sock->peerPort() == 9999);
    sock->disconnect();
}
TEST_CASE("UdpSocket sendTo / recvFrom (async echo)", "[udp_socket][async][echo]") {
    TestLoop loop;
    auto server = UdpSocket::create(loop.loop);
    auto client = UdpSocket::create(loop.loop);
    REQUIRE(server->bind("127.0.0.1", 0));
    uint16_t port = server->localPort();
    REQUIRE(port > 0);
    JobIpAddr serverAddr("127.0.0.1", port);
    std::atomic<bool> serverGotMsg{false};
    std::atomic<bool> clientGotEcho{false};
    const std::string msg = "Hello UDP!";
    server->onRead = [&](const char*, size_t) {
        char buf[64]{};
        JobIpAddr sender;
        ssize_t n = server->recvFrom(buf, sizeof(buf), sender);
        if (n > 0) {
            REQUIRE(std::string(buf, n) == msg);
            serverGotMsg.store(true);
            server->sendTo(buf, n, sender);
        }
    };
    REQUIRE(client->bind("127.0.0.1", 0));
    client->onRead = [&](const char*, size_t) {
        char buf[64]{};
        // We must use recvFrom since the client isn't "connected"
        JobIpAddr sender;
        ssize_t n = client->recvFrom(buf, sizeof(buf), sender);
        if (n > 0) {
            if (std::string(buf, n) == msg)
                clientGotEcho.store(true);
            client->disconnect();
        }
    };
    client->sendTo(msg.c_str(), msg.length(), serverAddr);
    int retries = 0;
    while ((!serverGotMsg.load() || !clientGotEcho.load()) && retries < 100) {
        std::this_thread::sleep_for(1ms);
        retries++;
    }
    REQUIRE(serverGotMsg.load() == true);
    REQUIRE(clientGotEcho.load() == true);
    server->disconnect();
}
TEST_CASE("UdpSocket option toggling", "[udp_socket][options]") {
    TestLoop loop;
    auto sock = UdpSocket::create(loop.loop);
    REQUIRE(sock->bind("127.0.0.1", 0));
    sock->setOption(ISocketIO::SocketOption::ReuseAddress, true);
    REQUIRE(sock->option(ISocketIO::SocketOption::ReuseAddress));
    sock->setOption(ISocketIO::SocketOption::Broadcast, true);
    REQUIRE(sock->option(ISocketIO::SocketOption::Broadcast));
    sock->disconnect();
}