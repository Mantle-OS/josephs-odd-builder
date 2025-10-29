#include <catch2/catch_test_macros.hpp>
#include <thread>
#include <chrono>

#include <tcp_socket.h>
#include <job_ipaddr.h>
#include <job_url.h>
#include <socket_error.h>

using namespace job::net;
using namespace job::threads;

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

TEST_CASE("TcpSocket handles invalid connect gracefully", "[tcp_socket][error]") {
    // TODO REFACTOR
    // TcpSocket sock;
    // JobUrl url("tcp://192.0.2.1:65000"); // RFC5737 TEST-NET-1, unroutable
    // REQUIRE_FALSE(sock.connectToHost(url));
    // REQUIRE(sock.lastError() != SocketErrors::SocketErrNo::None);
}

TEST_CASE("TcpSocket option toggling works", "[tcp_socket][options]") {
    TcpSocket sock;
    sock.setOption(ISocketIO::SocketOption::ReuseAddress, true);
    REQUIRE(sock.option(ISocketIO::SocketOption::ReuseAddress));
}

TEST_CASE("TcpSocket loopback echo", "[tcp][socket][async]") {

    auto loop = std::make_shared<AsyncEventLoop>();
    loop->start();

    // Server setup
    auto server = std::make_shared<TcpSocket>(loop);
    // bind to ephemeral port
    REQUIRE(server->bind("127.0.0.1", 0));
    REQUIRE(server->listen());

    uint16_t port = server->localPort();
    REQUIRE(port > 0);

    std::atomic<bool> gotEcho{false};

    // Client setup
    auto client = std::make_shared<TcpSocket>(loop);
    JobUrl url;
    url.setScheme("tcp");
    url.setHost("127.0.0.1");
    url.setPort(port);

    client->onConnect = [&] {
        const char *msg = "Hello";
        std::cout << "JOSEPH: CLIENT SEND DATA " << msg << "\n";
        client->write(msg, strlen(msg));
    };

    client->onRead = [&](const char *data, size_t len) {
        std::string recv(data, len);
        std::cout << "JOSEPH : CLIENT READ DATA " << recv << "\n";
        if (recv == "Hello")
            gotEcho = true;
        client->disconnect();
    };

    REQUIRE(client->connectToHost(url));

    // REALLY IMPORTANT
    // SERVER LOOP MUST HAPPEN AFTER THE client is up and registerd
    std::thread serverThread([&] {
        auto clientSock = server->accept();
        REQUIRE(clientSock); // line 85
        clientSock->onRead = [&](const char *data, size_t len) {
            std::cout << "JOSEPH SERVER GOT DATA " << data << "\n";
            clientSock->write(data, len); // echo back
        };
        clientSock->registerEvents();
    });

    client->registerEvents();

    // Wait a moment for async I/O
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    REQUIRE(gotEcho.load());

    serverThread.join();
    loop->stop();
}
