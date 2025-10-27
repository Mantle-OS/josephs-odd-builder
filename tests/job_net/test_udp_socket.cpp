#include <catch2/catch_test_macros.hpp>
#include <thread>
#include <cstring>

#include <udp_socket.h>
#include <job_ipaddr.h>

using namespace job::net;

ssize_t msg_ssize(const char *msg)
{
    return static_cast<ssize_t>(std::strlen(msg));
}

TEST_CASE("UdpSocket basic send/receive loopback", "[udp_socket][loopback]") {
    UdpSocket server;
    UdpSocket client;

    // Bind the server to localhost:0 (kernel assigns port)
    JobIpAddr bindAddr("127.0.0.1", 0);
    REQUIRE(server.bind(bindAddr));

    // Determine the actual bound port
    std::string localAddr = server.localAddress();
    uint16_t port = server.localPort();
    REQUIRE(port != 0);

    // Client connects to that port
    JobUrl url("udp://" + localAddr + ":" + std::to_string(port));
    REQUIRE(client.connectToHost(url));

    const char *msg = "Hello UDP!";
    char recvBuf[128]{};

    // Spawn a listener thread to simulate asynchronous recvFrom()
    std::thread listener([&]() {
        JobIpAddr sender;
        ssize_t n = server.recvFrom(recvBuf, sizeof(recvBuf), sender);
        REQUIRE(n > 0);
        recvBuf[n] = '\0';
        REQUIRE(std::string(recvBuf) == msg);
        REQUIRE(sender.isValid());
        REQUIRE(sender.isLoopback());
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Client sends to server
    ssize_t sent = client.write(msg, std::strlen(msg));
    REQUIRE(sent == msg_ssize(msg));

    listener.join();

    server.disconnect();
    client.disconnect();

    REQUIRE(server.state() == ISocketIO::SocketState::Closed);
    REQUIRE(client.state() == ISocketIO::SocketState::Closed);
}

TEST_CASE("UdpSocket sendTo and recvFrom explicit", "[udp_socket][sendto_recvfrom]") {
    UdpSocket sockA;
    UdpSocket sockB;

    REQUIRE(sockA.bind("127.0.0.1", 0));
    REQUIRE(sockB.bind("127.0.0.1", 0));

    JobIpAddr addrB("127.0.0.1", sockB.localPort());
    const char *payload = "Datagram Test";
    char buffer[128]{};

    // Send from A to B
    ssize_t sent = sockA.sendTo(payload, std::strlen(payload), addrB);
    REQUIRE(sent == msg_ssize(payload));

    // Receive on B
    JobIpAddr sender;
    ssize_t recvd = sockB.recvFrom(buffer, sizeof(buffer), sender);
    REQUIRE(recvd == msg_ssize(payload));
    buffer[recvd] = '\0';
    REQUIRE(std::string(buffer) == payload);

    // Validate metadata
    REQUIRE(sender.isValid());
    REQUIRE(sender.isLoopback());
    REQUIRE(sockB.state() == ISocketIO::SocketState::Connected);
}

TEST_CASE("UdpSocket option toggles and error handling", "[udp_socket][options]") {
    UdpSocket sock;

    REQUIRE(sock.bind("127.0.0.1", 0));

    sock.setOption(ISocketIO::SocketOption::ReuseAddress, true);
    REQUIRE(sock.option(ISocketIO::SocketOption::ReuseAddress));

    sock.setOption(ISocketIO::SocketOption::Broadcast, true);
    REQUIRE(sock.option(ISocketIO::SocketOption::Broadcast));

    sock.setOption(ISocketIO::SocketOption::NonBlocking, true);

    sock.disconnect();
    REQUIRE(sock.state() == ISocketIO::SocketState::Closed);
}

TEST_CASE("UdpSocket handles bad reads gracefully", "[udp_socket][errors]") {
    UdpSocket sock;
    char buf[16];
    REQUIRE(sock.read(buf, sizeof(buf)) == -1);
    REQUIRE(sock.lastError() != SocketErrors::SocketErrNo::None);
}
