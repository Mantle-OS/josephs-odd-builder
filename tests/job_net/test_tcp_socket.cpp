#include "test_loop.h"
#include <thread>
#include <atomic>
#include <cstring>

TEST_CASE("TcpSocket basic creation and cleanup", "[tcp_socket][lifecycle]") {
    TestLoop loop;
    TcpSocket sock(loop.loop);

    REQUIRE(sock.fd() == -1);
    REQUIRE(sock.state() == ISocketIO::SocketState::Unconnected);

    sock.disconnect();
    REQUIRE(sock.state() == ISocketIO::SocketState::Closed);
}

TEST_CASE("TcpSocket bind and listen on localhost", "[tcp_socket][bind][listen]") {
    TestLoop loop;
    TcpSocket server(loop.loop);

    JobIpAddr addr("127.0.0.1", 0);
    REQUIRE(server.bind(addr));
    REQUIRE(server.fd() != -1);
    REQUIRE(server.listen(1));
    REQUIRE(server.state() == ISocketIO::SocketState::Listening);
    REQUIRE(server.localPort() > 0);

    server.disconnect();
    REQUIRE(server.state() == ISocketIO::SocketState::Closed);
}

TEST_CASE("TcpSocket handles async connect failure", "[tcp_socket][error][async]") {
    TestLoop loop;
    TcpSocket sock(loop.loop);

    std::atomic<bool> error_fired{false};
    std::atomic<int> error_code{0};
    sock.onError = [&](int err) {
        INFO("onError callback fired with code: " << err << " (" << strerror(err) << ")");
        error_code.store(err);
        error_fired.store(true);
    };
    JobUrl url("tcp://127.0.0.1:65534");

    REQUIRE(sock.connectToHost(url));
    REQUIRE(sock.state() == ISocketIO::SocketState::Connecting);

    int retries = 0;
    while (!error_fired.load() && retries < 200) {
        std::this_thread::sleep_for(1ms);
        retries++;
    }
    REQUIRE(error_fired.load() == true);
    REQUIRE(sock.state() == ISocketIO::SocketState::Closed);
    REQUIRE(error_code.load() == ECONNREFUSED);
}

TEST_CASE("TcpSocket option toggling works", "[tcp_socket][options]") {
    TestLoop loop;
    auto sock = std::make_shared<TcpSocket>(loop.loop);

    REQUIRE(sock->bind("127.0.0.1", 0));

    sock->setOption(ISocketIO::SocketOption::ReuseAddress, true);
    REQUIRE(sock->option(ISocketIO::SocketOption::ReuseAddress));

    sock->setOption(ISocketIO::SocketOption::KeepAlive, true);
    REQUIRE(sock->option(ISocketIO::SocketOption::KeepAlive));
}

TEST_CASE("TcpSocket loopback echo (v2.0)", "[tcp_socket][async][echo]") {
    TestLoop loop;
    std::atomic<bool> gotEcho{false};
    std::atomic<bool> serverRead{false};

    std::shared_ptr<ISocketIO> clientConnection;

    auto server = std::make_shared<TcpSocket>(loop.loop);
    auto client = std::make_shared<TcpSocket>(loop.loop);

    REQUIRE(server->bind("127.0.0.1", 0));
    REQUIRE(server->listen());
    uint16_t port = server->localPort();
    REQUIRE(port > 0);

    JobUrl url("tcp://127.0.0.1:" + std::to_string(port));

    server->onConnect = [&]() {
        clientConnection = server->accept();
        if (clientConnection) {
            clientConnection->onRead = [&](const char*, size_t) {
                char buf[32]{};
                ssize_t n = clientConnection->read(buf, sizeof(buf));
                if (n > 0) {
                    serverRead.store(true);
                    clientConnection->write(buf, n); // Echo
                }
            };
            clientConnection->onDisconnect = [&]() {
                INFO("[TCP Server:] Client disconnected.");
            };
        }
    };

    client->onConnect = [&]() {
        const char *msg = "Hello";
        client->write(msg, strlen(msg));
    };

    client->onRead = [&](const char*, size_t) {
        char buf[32]{};
        ssize_t n = client->read(buf, sizeof(buf));
        if (n > 0) {
            if (std::string(buf, n) == "Hello")
                gotEcho.store(true);

            client->disconnect();
        }
    };

    REQUIRE(client->connectToHost(url));

    int retries = 0;
    while (!gotEcho.load() && retries < 200) { // 200ms timeout
        std::this_thread::sleep_for(1ms);
        retries++;
    }
    REQUIRE(serverRead.load() == true);
    REQUIRE(gotEcho.load() == true);

    server->disconnect();
}
// CHECKPOINT: v2.0
