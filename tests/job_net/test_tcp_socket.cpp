#include <catch2/catch_test_macros.hpp>
#include <thread>
#include <chrono>

#include <tcp_socket.h>
#include <job_ipaddr.h>
#include <job_url.h>
#include <socket_error.h>

using namespace job::net;

TEST_CASE("TcpSocket basic creation and cleanup", "[tcp_socket][lifecycle]") {
    TcpSocket sock;
    REQUIRE(sock.fd() >= 0);
    REQUIRE(sock.state() == ISocketIO::SocketState::Unconnected);
    sock.disconnect();
    REQUIRE(sock.state() == ISocketIO::SocketState::Closed);
}

TEST_CASE("TcpSocket bind and listen on localhost", "[tcp_socket][bind][listen]") {
    TcpSocket server;
    JobIpAddr addr("127.0.0.1", 0); // port 0 → ephemeral
    REQUIRE(server.bind(addr));

    REQUIRE(server.listen(1));
    REQUIRE(server.state() == ISocketIO::SocketState::Connected);
    server.disconnect();
}

// TEST_CASE("TcpSocket client connects to server (loopback)", "[tcp_socket][connect][loopback]") {
//     TcpSocket server;
//     JobIpAddr addr("127.0.0.1", 0);
//     REQUIRE(server.bind(addr));
//     REQUIRE(server.listen(1));

//     // Extract assigned port
//     uint16_t port = server.localPort();
//     REQUIRE(port > 0);

//     std::atomic<bool> clientConnected{false};

//     // Spawn client in background
//     std::thread clientThread([&]() {
//         std::this_thread::sleep_for(std::chrono::milliseconds(50));
//         TcpSocket client;
//         JobUrl url(std::string("tcp://127.0.0.1:") + std::to_string(port));
//         REQUIRE(client.connectToHost(url));
//         clientConnected = true;
//         client.disconnect();
//     });

//     auto accepted = server.accept();
//     REQUIRE(accepted != nullptr);
//     REQUIRE(accepted->state() == ISocketIO::SocketState::Connected);

//     clientThread.join();
//     server.disconnect();
//     REQUIRE(clientConnected.load());
// }

TEST_CASE("TcpSocket handles invalid connect gracefully", "[tcp_socket][error]") {
    TcpSocket sock;
    JobUrl url("tcp://192.0.2.1:65000"); // RFC5737 TEST-NET-1, unroutable
    REQUIRE_FALSE(sock.connectToHost(url));
    REQUIRE(sock.lastError() != SocketErrors::SocketErrNo::None);
}

TEST_CASE("TcpSocket option toggling works", "[tcp_socket][options]") {
    TcpSocket sock;
    sock.setOption(ISocketIO::SocketOption::ReuseAddress, true);
    REQUIRE(sock.option(ISocketIO::SocketOption::ReuseAddress));
}

// TEST_CASE("TcpSocket read/write roundtrip", "[tcp_socket][io]") {
//     TcpSocket server;
//     JobIpAddr addr("127.0.0.1", 0);
//     REQUIRE(server.bind(addr));
//     REQUIRE(server.listen(1));

//     uint16_t port = server.localPort();
//     REQUIRE(port > 0);

//     std::thread clientThread([&]() {
//         TcpSocket client;
//         JobUrl url(std::string("tcp://127.0.0.1:") + std::to_string(port));
//         REQUIRE(client.connectToHost(url));

//         const std::string msg = "HelloJOB";
//         ssize_t sent = client.write(msg.data(), msg.size());
//         REQUIRE(sent == static_cast<ssize_t>(msg.size()));
//         client.disconnect();
//     });

//     auto conn = server.accept();
//     REQUIRE(conn);
//     char buf[64] = {};
//     ssize_t received = conn->read(buf, sizeof(buf));
//     REQUIRE(received > 0);
//     REQUIRE(std::string(buf, received) == "HelloJOB");

//     clientThread.join();
//     conn->disconnect();
//     server.disconnect();
// }
