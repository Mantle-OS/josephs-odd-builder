#include <catch2/catch_test_macros.hpp>
#include <tcp_server.h>
#include <tcp_client.h>

#include <string>
#include <atomic>

using namespace job::net;

TEST_CASE("TcpServer and TcpClient echo test", "[tcp][server][client]") {

    std::atomic<bool> gotResponse{false};
    const std::string msg = "Hello, Mantle!";

    TcpServer server;
    TcpClient client;

    ///////////////////////////////
    // Server setup
    REQUIRE(server.start("127.0.0.1", 0));
    std::cout << "[SERVER] UP 127.0.0.1:" << server.port() << "\n";
    uint16_t port = server.port();
    REQUIRE(port != 0);

    server.onClientConnected = [&](TcpServer::ClientPtr c) {
        std::cout << "[SERVER] Client connected from "
                  << c->isConnected() << std::endl;
    };

    server.onClientMessage = [&](TcpServer::ClientPtr c, const char *data, size_t len) {
        std::string recv(data, len);
        std::cout << "[SERVER] Server received: " << recv << std::endl;
        REQUIRE(c->send(recv) == static_cast<ssize_t>(recv.size())); // echo back
    };

    server.onClientDisconnected = [&](TcpServer::ClientPtr) {
        std::cout << "[SERVER] Client disconnected." << std::endl;
    };


    ///////////////////////////////
    // Client setup
    std::cout << "[CLIENT] connectToHost 127.0.0.1:" << port << "\n";
    REQUIRE(client.connectToHost("127.0.0.1", port));
    std::cout << "[CLIENT] UP \n";

    // Connect client ->  server
    client.onConnect = [&]() {
        std::cout << "[CLIENT] connected to server. Sending Test message \n";
        REQUIRE(client.send(msg) == static_cast<ssize_t>(msg.size()));
        std::cout << "[CLIENT] Test message Sent" << std::endl;
    };

    client.onMessage = [&](const char *data, size_t len) {
        std::string recv(data, len);
        std::cout << "[CLIENT] Got: " << recv << std::endl;
        REQUIRE(recv == msg);
        gotResponse = true;
    };

    client.onDisconnect = [&]() {
        std::cout << "[CLIENT] disconnected cleanly." << std::endl;
    };

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(gotResponse.load());  // Ensure the response was received

    server.stop();
}
