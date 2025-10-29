#include <catch2/catch_test_macros.hpp>
#include <thread>
#include <filesystem>
#include <atomic>
#include <cstring>

#include <unix_socket.h>
#include <job_url.h>

using namespace job::net;
using namespace job::threads;
namespace fs = std::filesystem;

// Utility: generate unique /tmp socket path
static std::string make_temp_sock_path(const std::string &base)
{
    std::string path = "/tmp/" + base + "_" + std::to_string(::getpid()) + ".sock";
    if (fs::exists(path))
        fs::remove(path);
    return path;
}

// ------------------------------------------------------------
//  Test 1: Basic bind / connect / message exchange
// ------------------------------------------------------------
TEST_CASE("UnixSocket connectToHost and message exchange", "[unix_socket][connect][send][recv]")
{
    std::atomic<bool> gotMessage{false};
    std::string message = "Hello Unix!";

    auto path = make_temp_sock_path("unix_connect");
    auto loop = std::make_shared<AsyncEventLoop>();
    loop->start();

    auto server = std::make_shared<UnixSocket>(loop);
    REQUIRE(server->bind(path));
    REQUIRE(server->listen(1));

    std::thread serverThread([&]() {
        std::shared_ptr<ISocketIO> clientSock;

        // Poll until a connection arrives
        for (int i = 0; i < 100 && !(clientSock = server->accept()); ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(5));

        REQUIRE(clientSock);

        clientSock->onRead = [&, clientSock](const char *data, size_t len) {
            std::string recv(data, len);
            REQUIRE(recv == message);
            gotMessage = true;
            clientSock->disconnect();
        };

        clientSock->registerEvents();
    });

    // Give listener a head start
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    JobUrl url("unix://" + path);
    auto client = std::make_shared<UnixSocket>(loop);
    REQUIRE(client->connectToHost(url));

    // Send after connect
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    ssize_t sent = client->write(message.c_str(), message.size());
    REQUIRE(sent == static_cast<ssize_t>(message.size()));

    // Wait for server to process
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    REQUIRE(gotMessage.load());

    // Cleanup
    client->disconnect();
    server->disconnect();
    loop->stop();
    serverThread.join();

    if (fs::exists(path))
        fs::remove(path);
}

// ------------------------------------------------------------
//  Test 2: Accept returns nullptr until client connects
// ------------------------------------------------------------
TEST_CASE("UnixSocket non-blocking accept loop", "[unix_socket][accept][nonblocking]")
{
    auto path = make_temp_sock_path("unix_nb_accept");
    auto server = std::make_shared<UnixSocket>();
    REQUIRE(server->bind(path));
    REQUIRE(server->listen(1));

    // Should be non-blocking; no client yet
    REQUIRE(server->accept() == nullptr);

    JobUrl url("unix://" + path);
    auto client = std::make_shared<UnixSocket>();
    REQUIRE(client->connectToHost(url));

    // Try again until a connection appears
    std::shared_ptr<ISocketIO> cSock;
    for (int i = 0; i < 100 && !(cSock = server->accept()); ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

    REQUIRE(cSock);
    client->disconnect();
    server->disconnect();

    if (fs::exists(path))
        fs::remove(path);
}

// ------------------------------------------------------------
//  Test 3: Read/write sanity and cleanup
// ------------------------------------------------------------
TEST_CASE("UnixSocket write/read and cleanup", "[unix_socket][io][cleanup]")
{
    auto path = make_temp_sock_path("unix_io_cleanup");
    auto server = std::make_shared<UnixSocket>();
    REQUIRE(server->bind(path));
    REQUIRE(server->listen(1));

    JobUrl url("unix://" + path);
    auto client = std::make_shared<UnixSocket>();
    REQUIRE(client->connectToHost(url));

    // Accept and echo
    std::shared_ptr<ISocketIO> conn;
    for (int i = 0; i < 50 && !(conn = server->accept()); ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    REQUIRE(conn);

    const char *msg = "Ping!";
    char buf[32]{};

    REQUIRE(client->write(msg, std::strlen(msg)) == 5);
    REQUIRE(conn->read(buf, sizeof(buf)) == 5);
    buf[5] = '\0';
    REQUIRE(std::string(buf) == msg);

    client->disconnect();
    server->disconnect();

    REQUIRE_FALSE(fs::exists(path)); // socket file unlinked by disconnect
}
