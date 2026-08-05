#include "test_loop.h"
#include "../transient_test_file.h"

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#include <stdexcept>
#endif

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>

#include <job_secure_mem.h>
#include <job_ssl_options.h>
#include <job_x509_generator.h>

#include <clients/ssl_client.h>
#include <servers/ssl_server.h>

using namespace job::crypto;
using namespace job::net;
using namespace std::chrono_literals;

namespace {

[[nodiscard]] std::string transientPath(const std::string &name)
{
    static std::atomic<uint64_t> counter{0};

    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto index = counter.fetch_add(1, std::memory_order_relaxed);
    const std::string fileName = "job_ssl_client_server_" + std::to_string(stamp) + "_" + std::to_string(index) + "_" + name;

    return (std::filesystem::temp_directory_path() / fileName).string();
}

template<typename Predicate>
[[nodiscard]] bool waitUntil(Predicate &&predicate, std::chrono::milliseconds timeout = 2000ms)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (!predicate()) {
        if (std::chrono::steady_clock::now() >= deadline)
            return false;

        std::this_thread::sleep_for(1ms);
    }

    return true;
}

struct LocalIdentity {
#if defined(JOB_WINDOWS)
    TransientTestFile identity;
#else
    TransientTestFile certificate;
    TransientTestFile privateKey;
#endif

    LocalIdentity()
#if defined(JOB_WINDOWS)
        : identity(transientPath("identity.p12"))
#else
        : certificate(transientPath("certificate.pem")),
        privateKey(transientPath("private_key.pem"))
#endif
    {
    }
};

[[nodiscard]] bool generateLocalIdentity(LocalIdentity &identity)
{
    JobSslOptions options;

    options.setKeyType(JobSslOptions::KeyType::EC);
    options.setEcCurve(JobSslOptions::EcCurve::P256);
    options.setDigest(JobSslOptions::Digest::SHA256);
    options.setValidDays(1);
    options.setCommonName("localhost");
    options.setOrganization("JosephsOddBuilder Tests");
    options.setCountry("US");
    options.setDnsNames({"localhost"});
    options.setIpAddresses({"127.0.0.1", "::1"});

#if defined(JOB_WINDOWS)
    options.setEncoding(JobSslOptions::Encoding::PKCS12);

    JobSecureMem passphrase;
    return JobX509Generator::generate(options, identity.identity.path(), passphrase);
#else
    options.setEncoding(JobSslOptions::Encoding::PEM);
    return JobX509Generator::generate(options, identity.certificate.path(), identity.privateKey.path());
#endif
}

[[nodiscard]] JobSslContext::Ptr createServerContext(const LocalIdentity &identity)
{
    auto context = std::make_shared<JobSslContext>(JobSslContext::SslMode::Server);

    if (!context->isValid())
        return {};

    context->setVerifyMode(JobSslContext::VerifyMode::None);

#if defined(JOB_WINDOWS)
    if (!context->loadIdentityFile(identity.identity.path(), {}))
        return {};
#else
    if (!context->loadCertificateFile(identity.certificate.path(), JobSslContext::EncodingType::PEM))
        return {};

    if (!context->loadPrivateKeyFile(identity.privateKey.path(), JobSslContext::EncodingType::PEM, {}))
        return {};
#endif

    return context;
}

[[nodiscard]] JobSslContext::Ptr createClientContext()
{
    auto context = std::make_shared<JobSslContext>(JobSslContext::SslMode::Client);

    if (!context->isValid())
        return {};

    context->setVerifyMode(JobSslContext::VerifyMode::None);
    return context;
}



#ifdef JOB_TEST_BENCHMARKS



[[nodiscard]] bool runSslEchoBenchmark(const std::string &payload)
{
    TestLoop loop;
    LocalIdentity identity;

    if (!generateLocalIdentity(identity))
        return false;

    auto serverContext = createServerContext(identity);
    auto clientContext = createClientContext();

    if (!serverContext || !clientContext)
        return false;

    auto server = std::make_shared<SslServer>(loop.loop, serverContext);
    auto client = std::make_shared<SslClient>(loop.loop, clientContext);

    if (!server || !client)
        return false;

    std::atomic<bool> clientGotEcho{false};
    std::atomic<bool> clientDisconnected{false};
    std::atomic<bool> serverDisconnected{false};
    std::atomic<bool> socketError{false};
    std::atomic<bool> sslError{false};

    std::string receivedPayload;
    receivedPayload.reserve(payload.size());

    server->onClientMessage = [&](const SslClient::Ptr &connectedClient, const char *data, size_t size) {
        if (connectedClient->send(data, size) != static_cast<int64_t>(size))
            socketError.store(true, std::memory_order_relaxed);
    };

    server->onClientDisconnected = [&](const SslClient::Ptr &) {
        serverDisconnected.store(true, std::memory_order_relaxed);
    };

    server->onSocketError = [&](int) {
        socketError.store(true, std::memory_order_relaxed);
    };

    server->onSslError = [&](const SslClient::Ptr &, JobSslError::SslErrNo error, const std::string &) {
        if (JobSslError::isFatalSslError(error))
            sslError.store(true, std::memory_order_relaxed);
    };

    std::weak_ptr<SslClient> weakClient = client;

    client->onEncrypted = [weakClient, &payload, &socketError]() {
        const auto current = weakClient.lock();

        if (!current)
            return;

        if (current->send(payload) != static_cast<int64_t>(payload.size()))
            socketError.store(true, std::memory_order_relaxed);
    };

    client->onMessage = [weakClient, &payload, &receivedPayload, &clientGotEcho](const char *data, size_t size) {
        const auto current = weakClient.lock();

        if (!current)
            return;

        receivedPayload.append(data, size);

        if (receivedPayload.size() < payload.size())
            return;

        clientGotEcho.store(receivedPayload == payload, std::memory_order_relaxed);
        current->disconnect();
    };

    client->onDisconnect = [&]() {
        clientDisconnected.store(true, std::memory_order_relaxed);
    };

    client->onSocketError = [&](int) {
        socketError.store(true, std::memory_order_relaxed);
    };

    client->onSslError = [&](JobSslError::SslErrNo error, const std::string &) {
        if (JobSslError::isFatalSslError(error))
            sslError.store(true, std::memory_order_relaxed);
    };

    if (!server->start("127.0.0.1", 0))
        return false;

    if (server->port() == 0)
        return false;

    if (!client->connectToHost(JobIpAddr("127.0.0.1", server->port())))
        return false;

    const bool completed = waitUntil([&]() {
        return clientDisconnected.load(std::memory_order_relaxed) ||
               socketError.load(std::memory_order_relaxed) ||
               sslError.load(std::memory_order_relaxed);
    }, 4000ms);

    const bool serverShutdownCompleted = waitUntil([&]() {
        return serverDisconnected.load(std::memory_order_relaxed);
    }, 500ms);

    if (!completed) {
        JOB_LOG_ERROR(
            "[TLS][BENCHMARK] Client completion timeout: echo={} disconnected={} socketError={} sslError={} received={}/{}",
            clientGotEcho.load(std::memory_order_relaxed),
            clientDisconnected.load(std::memory_order_relaxed),
            socketError.load(std::memory_order_relaxed),
            sslError.load(std::memory_order_relaxed),
            receivedPayload.size(),
            payload.size()
            );
    }

    if (!serverShutdownCompleted) {
        JOB_LOG_ERROR(
            "[TLS][BENCHMARK] Server disconnect timeout: clientDisconnected={} serverDisconnected={}",
            clientDisconnected.load(std::memory_order_relaxed),
            serverDisconnected.load(std::memory_order_relaxed)
            );
    }

    const bool succeeded =
        completed &&
        serverShutdownCompleted &&
        clientGotEcho.load(std::memory_order_relaxed) &&
        !socketError.load(std::memory_order_relaxed) &&
        !sslError.load(std::memory_order_relaxed);

    client->onEncrypted = nullptr;
    client->onMessage = nullptr;
    client->onDisconnect = nullptr;
    client->onSocketError = nullptr;
    client->onSslError = nullptr;

    server->onClientMessage = nullptr;
    server->onClientDisconnected = nullptr;
    server->onSocketError = nullptr;
    server->onSslError = nullptr;

    client->disconnect();
    server->stop();

    return succeeded;
}

[[nodiscard]] bool requireSslEchoBenchmark(const std::string &payload)
{
    const bool succeeded = runSslEchoBenchmark(payload);

    if (!succeeded)
        throw std::runtime_error("SslClient/SslServer benchmark iteration failed");

    return succeeded;
}

#endif


} // namespace

TEST_CASE("SslClient and SslServer Full Echo Test", "[ssl_client_server][async][echo]")
{
    TestLoop loop;
    LocalIdentity identity;

    REQUIRE(generateLocalIdentity(identity));

    auto serverContext = createServerContext(identity);
    auto clientContext = createClientContext();

    REQUIRE(serverContext);
    REQUIRE(clientContext);

    std::atomic<bool> serverAccepted{false};
    std::atomic<bool> serverEncrypted{false};
    std::atomic<bool> serverGotMessage{false};
    std::atomic<bool> serverSawClientDisconnect{false};

    std::atomic<bool> clientConnected{false};
    std::atomic<bool> clientEncrypted{false};
    std::atomic<bool> clientGotEcho{false};
    std::atomic<bool> clientWasDisconnected{false};

    std::atomic<bool> socketError{false};
    std::atomic<bool> sslError{false};

    const std::string testMessage = "Hello, J.O.B. TLS!";

    auto server = std::make_shared<SslServer>(loop.loop, serverContext);

    server->onClientConnected = [&](const SslClient::Ptr &) {
        serverAccepted.store(true, std::memory_order_relaxed);
    };

    server->onClientEncrypted = [&](const SslClient::Ptr &) {
        serverEncrypted.store(true, std::memory_order_relaxed);
    };

    server->onClientMessage = [&](const SslClient::Ptr &client, const char *data, size_t size) {
        const std::string message(data, size);

        INFO("[TLS][Server] Received: " << message);

        serverGotMessage.store(true, std::memory_order_relaxed);

        if (client->send(data, size) != static_cast<int64_t>(size))
            socketError.store(true, std::memory_order_relaxed);
    };

    server->onClientDisconnected = [&](const SslClient::Ptr &) {
        serverSawClientDisconnect.store(true, std::memory_order_relaxed);
    };

    server->onSocketError = [&](int error) {
        INFO("[TLS][Server] Socket error: " << error);
        socketError.store(true, std::memory_order_relaxed);
    };

    server->onSslError = [&](const SslClient::Ptr &, JobSslError::SslErrNo error, const std::string &message) {
        if (!JobSslError::isFatalSslError(error))
            return;

        INFO("[TLS][Server] SSL error: " << message);
        sslError.store(true, std::memory_order_relaxed);
    };

    REQUIRE(server->start("127.0.0.1", 0));

    const uint16_t port = server->port();
    REQUIRE(port > 0);

    auto client = std::make_shared<SslClient>(loop.loop, clientContext);
    std::weak_ptr<SslClient> weakClient = client;

    client->onConnect = [&]() {
        clientConnected.store(true, std::memory_order_relaxed);
    };

    client->onEncrypted = [weakClient, &testMessage, &clientEncrypted, &socketError]() {
        const auto current = weakClient.lock();

        if (!current)
            return;

        clientEncrypted.store(true, std::memory_order_relaxed);

        if (current->send(testMessage) != static_cast<int64_t>(testMessage.size()))
            socketError.store(true, std::memory_order_relaxed);
    };

    client->onMessage = [weakClient, &testMessage, &clientGotEcho](const char *data, size_t size) {
        const auto current = weakClient.lock();

        if (!current)
            return;

        const std::string message(data, size);

        INFO("[TLS][Client] Received echo: " << message);

        if (message == testMessage)
            clientGotEcho.store(true, std::memory_order_relaxed);

        current->disconnect();
    };

    client->onDisconnect = [&]() {
        clientWasDisconnected.store(true, std::memory_order_relaxed);
    };

    client->onSocketError = [&](int error) {
        INFO("[TLS][Client] Socket error: " << error);
        socketError.store(true, std::memory_order_relaxed);
    };

    client->onSslError = [&](JobSslError::SslErrNo error, const std::string &message) {
        if (!JobSslError::isFatalSslError(error))
            return;

        INFO("[TLS][Client] SSL error: " << message);
        sslError.store(true, std::memory_order_relaxed);
    };

    REQUIRE(client->connectToHost(JobIpAddr("127.0.0.1", port)));

    const bool completed = waitUntil([&]() {
        return clientWasDisconnected.load(std::memory_order_relaxed) ||
               socketError.load(std::memory_order_relaxed) ||
               sslError.load(std::memory_order_relaxed);
    });

    const bool serverDisconnected = waitUntil([&]() {
        return serverSawClientDisconnect.load(std::memory_order_relaxed);
    }, 500ms);

    const bool hadSocketError = socketError.load(std::memory_order_relaxed);
    const bool hadSslError = sslError.load(std::memory_order_relaxed);

    const bool sawServerAccept = serverAccepted.load(std::memory_order_relaxed);
    const bool sawServerEncryption = serverEncrypted.load(std::memory_order_relaxed);
    const bool sawServerMessage = serverGotMessage.load(std::memory_order_relaxed);

    const bool sawClientConnect = clientConnected.load(std::memory_order_relaxed);
    const bool sawClientEncryption = clientEncrypted.load(std::memory_order_relaxed);
    const bool sawClientEcho = clientGotEcho.load(std::memory_order_relaxed);
    const bool sawClientDisconnect = clientWasDisconnected.load(std::memory_order_relaxed);

    JOB_LOG_INFO(
        "[TLS][TEST] Completed: {}, ServerAccepted: {}, ServerEncrypted: {}, ServerRead: {}, ClientConnected: {}, "
        "ClientEncrypted: {}, ClientEcho: {}, ClientDisconnect: {}, ServerDisconnect: {}, SocketError: {}, SslError: {}",
        completed,
        sawServerAccept,
        sawServerEncryption,
        sawServerMessage,
        sawClientConnect,
        sawClientEncryption,
        sawClientEcho,
        sawClientDisconnect,
        serverDisconnected,
        hadSocketError,
        hadSslError
        );

    /*
     * Remove callbacks that reference test-local state before initiating any
     * remaining shutdown operations.
     */
    client->onConnect = nullptr;
    client->onEncrypted = nullptr;
    client->onMessage = nullptr;
    client->onDisconnect = nullptr;
    client->onSocketError = nullptr;
    client->onSslError = nullptr;

    server->onClientConnected = nullptr;
    server->onClientEncrypted = nullptr;
    server->onClientMessage = nullptr;
    server->onClientDisconnected = nullptr;
    server->onSocketError = nullptr;
    server->onSslError = nullptr;

    client->disconnect();
    server->stop();

    REQUIRE(completed);
    REQUIRE_FALSE(hadSocketError);
    REQUIRE_FALSE(hadSslError);

    REQUIRE(sawServerAccept);
    REQUIRE(sawServerEncryption);
    REQUIRE(sawServerMessage);
    REQUIRE(serverDisconnected);

    REQUIRE(sawClientConnect);
    REQUIRE(sawClientEncryption);
    REQUIRE(sawClientEcho);
    REQUIRE(sawClientDisconnect);
}

// 2
/*
 * Block two: edge cases and failure behavior
 */

TEST_CASE("SslClient rejects sends before encryption", "[job_net][ssl_client][edge][send]")
{
    TestLoop loop;
    auto context = createClientContext();

    REQUIRE(context);

    SslClient client(loop.loop, context);

    REQUIRE_FALSE(client.isConnected());
    REQUIRE_FALSE(client.isEncrypted());
    REQUIRE(client.send("not connected") == -1);
    REQUIRE(client.send(nullptr, 1) == -1);
    REQUIRE(client.send(nullptr, 0) == -1);
}

TEST_CASE("SslServer cannot start without an SSL context", "[job_net][ssl_server][edge][context]")
{
    TestLoop loop;
    SslServer server(loop.loop, nullptr);

    REQUIRE_FALSE(server.start("127.0.0.1", 0));
    REQUIRE_FALSE(server.isRunning());
    REQUIRE(server.port() == 0);
}

TEST_CASE("SslClient reports a refused TCP connection", "[job_net][ssl_client][edge][connect][failure]")
{
    TestLoop loop;
    auto context = createClientContext();

    REQUIRE(context);

    SslClient client(loop.loop, context);

    std::atomic<bool> socketError{false};
    std::atomic<bool> disconnected{false};
    std::atomic<int> errorCode{0};

    client.onSocketError = [&](int error) {
        errorCode.store(error, std::memory_order_relaxed);
        socketError.store(true, std::memory_order_relaxed);
    };

    client.onDisconnect = [&]() {
        disconnected.store(true, std::memory_order_relaxed);
    };

    REQUIRE(client.connectToHost(JobIpAddr("127.0.0.1", 65534)));

    const bool completed = waitUntil([&]() {
        return socketError.load(std::memory_order_relaxed);
    });

    const bool sawSocketError = socketError.load(std::memory_order_relaxed);
    const bool sawDisconnect = disconnected.load(std::memory_order_relaxed);
    const int reportedError = errorCode.load(std::memory_order_relaxed);
    const bool connected = client.isConnected();
    const bool encrypted = client.isEncrypted();

    client.onSocketError = nullptr;
    client.onDisconnect = nullptr;
    client.disconnect();

    REQUIRE(completed);
    REQUIRE(sawSocketError);
    REQUIRE(reportedError != 0);
    REQUIRE_FALSE(connected);
    REQUIRE_FALSE(encrypted);

    static_cast<void>(sawDisconnect);
}

TEST_CASE("SslClient rejects an untrusted server certificate",
          "[job_net][ssl_client][ssl_server][edge][verification][failure]")
{
    TestLoop loop;
    LocalIdentity identity;

    REQUIRE(generateLocalIdentity(identity));

    auto serverContext = createServerContext(identity);
    auto clientContext = createClientContext();

    REQUIRE(serverContext);
    REQUIRE(clientContext);

    clientContext->setVerifyMode(JobSslContext::VerifyMode::Peer);

    auto server = std::make_shared<SslServer>(loop.loop, serverContext);
    auto client = std::make_shared<SslClient>(loop.loop, clientContext);

    REQUIRE(server);
    REQUIRE(client);

    std::atomic<bool> serverAccepted{false};
    std::atomic<bool> clientEncrypted{false};
    std::atomic<bool> verificationFailed{false};
    std::atomic<bool> socketError{false};

    JobSslError::SslErrNo reportedSslError = JobSslError::SslErrNo::None;
    std::string reportedSslErrorString;

    server->onClientConnected = [&](const SslClient::Ptr &) {
        serverAccepted.store(true, std::memory_order_relaxed);
    };

    server->onSocketError = [&](int) {
        socketError.store(true, std::memory_order_relaxed);
    };

    client->onEncrypted = [&]() {
        clientEncrypted.store(true, std::memory_order_relaxed);
    };

    client->onSocketError = [&](int) {
        socketError.store(true, std::memory_order_relaxed);
    };

    client->onSslError = [&](JobSslError::SslErrNo error, const std::string &message) {
        if (!JobSslError::isFatalSslError(error))
            return;

        reportedSslError = error;
        reportedSslErrorString = message;
        verificationFailed.store(true, std::memory_order_relaxed);
    };

    REQUIRE(server->start("127.0.0.1", 0));
    REQUIRE(server->port() > 0);
    REQUIRE(client->connectToHost(JobIpAddr("127.0.0.1", server->port())));

    const bool completed = waitUntil([&]() {
        return verificationFailed.load(std::memory_order_relaxed) ||
               clientEncrypted.load(std::memory_order_relaxed) ||
               socketError.load(std::memory_order_relaxed);
    });

    const bool accepted = serverAccepted.load(std::memory_order_relaxed);
    const bool encrypted = clientEncrypted.load(std::memory_order_relaxed);
    const bool failedVerification = verificationFailed.load(std::memory_order_relaxed);
    const bool hadSocketError = socketError.load(std::memory_order_relaxed);
    const bool clientReportsEncrypted = client->isEncrypted();

    INFO("[TLS][Client] Verification error code: " << static_cast<int>(reportedSslError));
    INFO("[TLS][Client] Verification error: " << reportedSslErrorString);

    client->onEncrypted = nullptr;
    client->onSocketError = nullptr;
    client->onSslError = nullptr;

    server->onClientConnected = nullptr;
    server->onSocketError = nullptr;

    client->disconnect();
    server->stop();

    REQUIRE(completed);
    REQUIRE(accepted);
    REQUIRE(failedVerification);
    REQUIRE_FALSE(encrypted);
    REQUIRE_FALSE(clientReportsEncrypted);
    REQUIRE_FALSE(hadSocketError);
    REQUIRE(reportedSslError == JobSslError::SslErrNo::CertificateVerifyFailed);
    REQUIRE_FALSE(reportedSslErrorString.empty());
}

TEST_CASE("SslServer stop disconnects active encrypted clients",
          "[job_net][ssl_client][ssl_server][edge][shutdown]")
{
    TestLoop loop;
    LocalIdentity identity;

    REQUIRE(generateLocalIdentity(identity));

    auto serverContext = createServerContext(identity);
    auto clientContext = createClientContext();

    REQUIRE(serverContext);
    REQUIRE(clientContext);

    auto server = std::make_shared<SslServer>(loop.loop, serverContext);
    auto client = std::make_shared<SslClient>(loop.loop, clientContext);

    REQUIRE(server);
    REQUIRE(client);

    std::atomic<bool> serverAccepted{false};
    std::atomic<bool> serverEncrypted{false};
    std::atomic<bool> clientEncrypted{false};
    std::atomic<bool> clientDisconnected{false};
    std::atomic<bool> socketError{false};
    std::atomic<bool> sslError{false};

    server->onClientConnected = [&](const SslClient::Ptr &) {
        serverAccepted.store(true, std::memory_order_relaxed);
    };

    server->onClientEncrypted = [&](const SslClient::Ptr &) {
        serverEncrypted.store(true, std::memory_order_relaxed);
    };

    server->onSocketError = [&](int) {
        socketError.store(true, std::memory_order_relaxed);
    };

    server->onSslError = [&](const SslClient::Ptr &, JobSslError::SslErrNo error, const std::string &) {
        if (JobSslError::isFatalSslError(error))
            sslError.store(true, std::memory_order_relaxed);
    };

    client->onEncrypted = [&]() {
        clientEncrypted.store(true, std::memory_order_relaxed);
    };

    client->onDisconnect = [&]() {
        clientDisconnected.store(true, std::memory_order_relaxed);
    };

    client->onSocketError = [&](int) {
        socketError.store(true, std::memory_order_relaxed);
    };

    client->onSslError = [&](JobSslError::SslErrNo error, const std::string &) {
        if (JobSslError::isFatalSslError(error))
            sslError.store(true, std::memory_order_relaxed);
    };

    REQUIRE(server->start("127.0.0.1", 0));
    REQUIRE(server->port() > 0);
    REQUIRE(client->connectToHost(JobIpAddr("127.0.0.1", server->port())));

    const bool handshakeCompleted = waitUntil([&]() {
        const bool connectionReady =
            serverAccepted.load(std::memory_order_relaxed) &&
            serverEncrypted.load(std::memory_order_relaxed) &&
            clientEncrypted.load(std::memory_order_relaxed);

        return connectionReady ||
               socketError.load(std::memory_order_relaxed) ||
               sslError.load(std::memory_order_relaxed);
    });

    const bool acceptedBeforeStop = serverAccepted.load(std::memory_order_relaxed);
    const bool serverEncryptedBeforeStop = serverEncrypted.load(std::memory_order_relaxed);
    const bool clientEncryptedBeforeStop = clientEncrypted.load(std::memory_order_relaxed);
    const bool connectedBeforeStop = client->isConnected();
    const bool hadSocketErrorBeforeStop = socketError.load(std::memory_order_relaxed);
    const bool hadSslErrorBeforeStop = sslError.load(std::memory_order_relaxed);

    server->stop();

    const bool disconnected = waitUntil([&]() {
        return clientDisconnected.load(std::memory_order_relaxed) || !client->isConnected();
    });

    const bool clientDisconnectedAfterStop = clientDisconnected.load(std::memory_order_relaxed);
    const bool connectedAfterStop = client->isConnected();
    const bool encryptedAfterStop = client->isEncrypted();
    const bool serverRunningAfterStop = server->isRunning();
    const bool hadSocketError = socketError.load(std::memory_order_relaxed);
    const bool hadSslError = sslError.load(std::memory_order_relaxed);

    client->onEncrypted = nullptr;
    client->onDisconnect = nullptr;
    client->onSocketError = nullptr;
    client->onSslError = nullptr;

    server->onClientConnected = nullptr;
    server->onClientEncrypted = nullptr;
    server->onSocketError = nullptr;
    server->onSslError = nullptr;

    client->disconnect();

    REQUIRE(handshakeCompleted);
    REQUIRE(acceptedBeforeStop);
    REQUIRE(serverEncryptedBeforeStop);
    REQUIRE(clientEncryptedBeforeStop);
    REQUIRE(connectedBeforeStop);
    REQUIRE_FALSE(hadSocketErrorBeforeStop);
    REQUIRE_FALSE(hadSslErrorBeforeStop);

    REQUIRE(disconnected);
    REQUIRE(clientDisconnectedAfterStop);
    REQUIRE_FALSE(serverRunningAfterStop);
    REQUIRE_FALSE(connectedAfterStop);
    REQUIRE_FALSE(encryptedAfterStop);
    REQUIRE_FALSE(hadSocketError);
    REQUIRE_FALSE(hadSslError);
}

/*
 * Block three: benchmarks and stress
 */

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("SslClient and SslServer encrypted echo performance", "[job_net][ssl_client][ssl_server][benchmark]")
{
    BENCHMARK("EC P-256 SslClient/SslServer lifecycle and echo")
    {
        return requireSslEchoBenchmark("benchmark");
    };

    BENCHMARK("EC P-256 SslClient/SslServer 1 KB echo")
    {
        const std::string payload(1024, 'J');
        return requireSslEchoBenchmark(payload);
    };

    BENCHMARK("EC P-256 SslClient/SslServer 4 KB echo")
    {
        const std::string payload(4096, 'B');
        return requireSslEchoBenchmark(payload);
    };

    BENCHMARK("EC P-256 SslClient/SslServer 16 KB echo")
    {
        const std::string payload(16 * 1024, 'T');
        return requireSslEchoBenchmark(payload);
    };
}

TEST_CASE("SslClient and SslServer repeated TLS lifecycles remain stable", "[job_net][ssl_client][ssl_server][stress]")
{
    constexpr size_t ITERATIONS = 50;

    for (size_t iteration = 0; iteration < ITERATIONS; ++iteration) {
        INFO("SSL client/server lifecycle iteration: " << iteration);
        REQUIRE(runSslEchoBenchmark("stress"));
    }
}

#endif
