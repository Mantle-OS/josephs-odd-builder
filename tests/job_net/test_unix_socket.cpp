#include "test_loop.h"
#include <unistd.h>

#include <thread>
#include <atomic>
#include <cstring>
#include <filesystem>


namespace fs = std::filesystem;

TEST_CASE("UnixSocket basic bind/listen/cleanup", "[unix_socket][lifecycle]") {
    TestLoop loop;
    auto path = make_temp_sock_path("unix_lifecycle");
    auto server = std::make_shared<UnixSocket>(loop.loop);

    REQUIRE(server->bind(path));
    REQUIRE(server->fd() != -1);
    REQUIRE(fs::exists(path));

    REQUIRE(server->listen(1));
    REQUIRE(server->state() == ISocketIO::SocketState::Listening);

    server->disconnect();
    REQUIRE(server->state() == ISocketIO::SocketState::Closed);
    REQUIRE_FALSE(fs::exists(path));
}

TEST_CASE("UnixSocket async connect/accept/echo", "[unix_socket][async][echo]") {
    TestLoop loop;
    auto path = make_temp_sock_path("unix_echo");
    auto server = std::make_shared<UnixSocket>(loop.loop);
    auto client = std::make_shared<UnixSocket>(loop.loop);

    std::atomic<bool> gotEcho{false};
    std::atomic<bool> serverRead{false};
    std::shared_ptr<ISocketIO> clientConnection;

    REQUIRE(server->bind(path));
    REQUIRE(server->listen());
    REQUIRE(server->state() == ISocketIO::SocketState::Listening);

    JobUrl url("unix://" + path);

    server->onAccept = [&](std::shared_ptr<ISocketIO> accepted) {
        clientConnection = accepted;
        // IMPORTANT: Capture weak_ptr to avoid circular reference issues
        std::weak_ptr<ISocketIO> weakConn = clientConnection;
        clientConnection->onRead = [&, weakConn](const void*, size_t) {
            auto conn = weakConn.lock();
            if (!conn)
                return;

            char buf[32]{};
            auto n = conn->read(buf, sizeof(buf));
            if (n > 0) {
                std::string msg(buf, n);
                serverRead.store(true);
                conn->write(buf, n);
            }
        };

        clientConnection->onDisconnect = [&]() {
            clientConnection.reset();
        };
    };

    client->onConnect = [&]() {
        const char *msg = "Hello Unix";
        client->write(msg, strlen(msg));
    };

    std::weak_ptr<ISocketIO> weakClient = client;
    client->onRead = [&, weakClient](const void*, size_t) {
        auto c = weakClient.lock();
        if (!c)
            return;

        char buf[32]{};
        ssize_t n = c->read(buf, sizeof(buf));
        if (n > 0) {
            std::string msg(buf, n);
            if (msg == "Hello Unix") {
                gotEcho.store(true);
                INFO("[TEST] Echo verified!");
            }
            c->disconnect();
        }
    };

    REQUIRE(client->connectToHost(url));

    int retries = 0;
    while (!gotEcho.load() && retries < 200) {
        std::this_thread::sleep_for(1ms);
        retries++;
    }

    if (clientConnection) {
        clientConnection->disconnect();
        clientConnection.reset();
    }
    client->disconnect();
    server->disconnect();

    std::this_thread::sleep_for(10ms);

    REQUIRE(serverRead.load() == true);
    REQUIRE(gotEcho.load() == true);
}

TEST_CASE("UnixSocket non-blocking accept returns null", "[unix_socket][accept]") {
    TestLoop loop;
    auto path = make_temp_sock_path("unix_nb_accept");
    auto server = std::make_shared<UnixSocket>(loop.loop);
    REQUIRE(server->bind(path));
    REQUIRE(server->listen(1));

    REQUIRE(server->accept() == nullptr);
    server->disconnect();
}
// CHECKPOINT: v2.3
