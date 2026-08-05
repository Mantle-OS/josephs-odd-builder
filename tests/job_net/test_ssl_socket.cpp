#include "test_loop.h"
#include "../transient_test_file.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <thread>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_secure_mem.h>
#include <job_ssl_options.h>
#include <job_x509_generator.h>

#include <resolve/job_ssl_context.h>
#include <ssl_socket.h>

using namespace job::crypto;
using namespace job::net;
using namespace std::chrono_literals;

namespace {

[[nodiscard]] std::string transientPath(const std::string &name)
{
    static std::atomic<uint64_t> counter{0};

    const auto stamp = std::chrono::steady_clock::now()
                           .time_since_epoch()
                           .count();

    const std::string fileName = "job_ssl_" + std::to_string(stamp) + "_"
                                 + std::to_string(counter.fetch_add(1, std::memory_order_relaxed)) + "_" + name;

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

[[nodiscard]] bool isFatalSslError(JobSslError::SslErrNo error) noexcept
{
    switch (error) {
    case JobSslError::SslErrNo::WantRead:
    case JobSslError::SslErrNo::WantWrite:
    case JobSslError::SslErrNo::WantAccept:
    case JobSslError::SslErrNo::WantConnect:
        return false;

    default:
        return true;
    }
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
    {
        return {};
    }

    if (!context->loadPrivateKeyFile(identity.privateKey.path(), JobSslContext::EncodingType::PEM, {}))
    {
        return {};
    }
#endif

    return context;
}

[[nodiscard]] JobSslContext::Ptr createClientContext(JobSslContext::VerifyMode verifyMode = JobSslContext::VerifyMode::None)
{
    auto context = std::make_shared<JobSslContext>(JobSslContext::SslMode::Client);

    if (!context->isValid())
        return {};

    context->setVerifyMode(verifyMode);
    return context;
}

struct TlsEchoResult {
    bool serverAccepted{false};
    bool serverEncrypted{false};
    bool clientEncrypted{false};
    bool serverRead{false};
    bool clientRead{false};
    bool clientDisconnected{false};
    std::string serverPayload;
    std::string clientPayload;
};

[[nodiscard]] bool runLocalTlsEcho(const std::string &message, TlsEchoResult *result = nullptr)
{
    TestLoop loop;
    LocalIdentity identity;

    if (!generateLocalIdentity(identity))
        return false;

    JobSslContext::Ptr serverContext = createServerContext(identity);
    JobSslContext::Ptr clientContext = createClientContext();

    if (!serverContext || !clientContext)
        return false;

    auto listener = TcpSocket::create(loop.loop);
    auto transport = TcpSocket::create(loop.loop);

    if (!listener || !transport)
        return false;

    if (!listener->bind("127.0.0.1", 0))
        return false;

    if (!listener->listen())
        return false;

    const uint16_t port = listener->localPort();

    if (port == 0)
        return false;

    std::atomic<bool> serverAccepted{false};
    std::atomic<bool> serverEncrypted{false};
    std::atomic<bool> clientEncrypted{false};
    std::atomic<bool> serverRead{false};
    std::atomic<bool> clientRead{false};
    std::atomic<bool> clientDisconnected{false};
    std::atomic<bool> failed{false};

    std::string serverPayload;
    std::string clientPayload;

    SslSocket::Ptr serverSsl;
    SslSocket::Ptr clientSsl;

    listener->onConnect = [&]() {
        ISocketIO::Ptr accepted = listener->accept();

        if (!accepted) {
            failed.store(true);
            return;
        }

        serverAccepted.store(true);

        serverSsl = SslSocket::create(std::move(accepted), serverContext);

        if (!serverSsl || serverSsl->state() == SslSocket::State::Error) {
            failed.store(true);
            return;
        }

        serverSsl->onEncrypted = [&]() {
            serverEncrypted.store(true);
        };

        serverSsl->onRead = [&](const char *, size_t) {
            std::array<char, 4096> buffer{};

            const int64_t count = serverSsl->read(buffer.data(), buffer.size());

            if (count <= 0)
                return;

            serverPayload.assign(buffer.data(), static_cast<size_t>(count));

            serverRead.store(true);

            const int64_t written = serverSsl->write(serverPayload.data(), serverPayload.size());

            if (written != static_cast<int64_t>(serverPayload.size()))
                failed.store(true);
        };

        serverSsl->onSslError = [&](JobSslError::SslErrNo error, const std::string &) {
            if (isFatalSslError(error))
                failed.store(true);
        };

        if (!serverSsl->startHandshake())
            failed.store(true);
    };

    clientSsl = SslSocket::create(transport, clientContext);

    if (!clientSsl)
        return false;

    clientSsl->onEncrypted = [&]() {
        clientEncrypted.store(true);

        const int64_t written = clientSsl->write(message.data(), message.size());

        if (written != static_cast<int64_t>(message.size()))
            failed.store(true);
    };

    clientSsl->onRead = [&](const char *, size_t) {
        std::array<char, 4096> buffer{};

        const int64_t count = clientSsl->read(buffer.data(), buffer.size());

        if (count <= 0)
            return;

        clientPayload.assign(buffer.data(), static_cast<size_t>(count));

        clientRead.store(true);
    };

    clientSsl->onDisconnect = [&]() {
        clientDisconnected.store(true);
    };

    clientSsl->onSslError = [&](JobSslError::SslErrNo error, const std::string &) {
        if (isFatalSslError(error))
            failed.store(true);
    };

    const JobIpAddr address("127.0.0.1", port);

    if (!clientSsl->connectToHost(address))
        return false;

    const bool completed = waitUntil([&]() { return clientRead.load() || failed.load(); });

    const bool succeeded = completed && !failed.load()
                           && serverAccepted.load()
                           && serverEncrypted.load()
                           && clientEncrypted.load()
                           && serverRead.load()
                           && clientRead.load()
                           && serverPayload == message
                           && clientPayload == message;

    if (result) {
        result->serverAccepted = serverAccepted.load();
        result->serverEncrypted = serverEncrypted.load();
        result->clientEncrypted = clientEncrypted.load();
        result->serverRead = serverRead.load();
        result->clientRead = clientRead.load();
        result->clientDisconnected = clientDisconnected.load();
        result->serverPayload = serverPayload;
        result->clientPayload = clientPayload;
    }

    listener->onConnect = nullptr;

    clientSsl->onEncrypted = nullptr;
    clientSsl->onRead = nullptr;
    clientSsl->onWrite = nullptr;
    clientSsl->onDisconnect = nullptr;
    clientSsl->onSocketError = nullptr;
    clientSsl->onSslError = nullptr;

    if (serverSsl) {
        serverSsl->onEncrypted = nullptr;
        serverSsl->onRead = nullptr;
        serverSsl->onWrite = nullptr;
        serverSsl->onDisconnect = nullptr;
        serverSsl->onSocketError = nullptr;
        serverSsl->onSslError = nullptr;
    }

    clientSsl->disconnect();

    if (serverSsl)
        serverSsl->disconnect();

    listener->disconnect();

    static_cast<void>(waitUntil(
        [&]() {
            const bool clientClosed = clientSsl->state() == SslSocket::State::Closed || clientSsl->state() == SslSocket::State::Error;

            const bool serverClosed = !serverSsl || serverSsl->state() == SslSocket::State::Closed || serverSsl->state() == SslSocket::State::Error;

            return clientClosed && serverClosed;
        },
        500ms));

    return succeeded;
}

} // namespace

/*
 * Block one: usage and examples
 *
 * These tests show the normal lifecycle:
 *
 *     TCP transport
 *         -> SslSocket
 *         -> TLS handshake
 *         -> encrypted read/write
 *         -> orderly disconnect
 */

TEST_CASE("SslSocket creates around a TCP transport and SSL context", "[job_net][ssl_socket][usage][lifecycle]")
{
    TestLoop loop;

    auto transport = TcpSocket::create(loop.loop);
    auto context = createClientContext();

    REQUIRE(transport);
    REQUIRE(context);

    auto socket = SslSocket::create(transport, context);

    REQUIRE(socket);
    REQUIRE(socket->socket() == transport);
    REQUIRE(socket->context() == context);
    REQUIRE(socket->state() == SslSocket::State::WaitingForTransport);
    REQUIRE_FALSE(socket->isEncrypted());
    REQUIRE_FALSE(socket->isOpen());
    REQUIRE(socket->socketState() == ISocketIO::SocketState::Unconnected);

    socket->disconnect();

    REQUIRE(socket->state() == SslSocket::State::Closed);
    REQUIRE_FALSE(socket->isEncrypted());
}

TEST_CASE("SslSocket performs a local encrypted echo", "[job_net][ssl_socket][usage][loopback][echo]")
{
    TlsEchoResult result;

    REQUIRE(runLocalTlsEcho("Hello over JOB TLS", &result));

    REQUIRE(result.serverAccepted);
    REQUIRE(result.serverEncrypted);
    REQUIRE(result.clientEncrypted);
    REQUIRE(result.serverRead);
    REQUIRE(result.clientRead);
    REQUIRE(result.serverPayload == "Hello over JOB TLS");
    REQUIRE(result.clientPayload == "Hello over JOB TLS");
}

TEST_CASE("SslSocket transfers a binary payload without treating it as text", "[job_net][ssl_socket][usage][binary]")
{
    const std::string payload{
        '\x00',
        '\x01',
        '\x02',
        '\x7f',
        static_cast<char>(0x80),
        static_cast<char>(0xfe),
        static_cast<char>(0xff),
        '\x00'
    };

    TlsEchoResult result;

    REQUIRE(runLocalTlsEcho(payload, &result));

    REQUIRE(result.serverPayload.size() == payload.size());
    REQUIRE(result.clientPayload.size() == payload.size());
    REQUIRE(result.serverPayload == payload);
    REQUIRE(result.clientPayload == payload);
}

TEST_CASE("SslSocket exposes the encrypted connection addresses", "[job_net][ssl_socket][usage][address]")
{
    TestLoop loop;
    LocalIdentity identity;

    REQUIRE(generateLocalIdentity(identity));

    auto serverContext = createServerContext(identity);
    auto clientContext = createClientContext();

    REQUIRE(serverContext);
    REQUIRE(clientContext);

    auto listener = TcpSocket::create(loop.loop);
    auto clientTransport = TcpSocket::create(loop.loop);

    REQUIRE(listener);
    REQUIRE(clientTransport);
    REQUIRE(listener->bind("127.0.0.1", 0));
    REQUIRE(listener->listen());

    const uint16_t port = listener->localPort();

    REQUIRE(port > 0);

    std::atomic<bool> clientEncrypted{false};
    std::atomic<bool> serverEncrypted{false};
    std::atomic<bool> failed{false};

    SslSocket::Ptr serverSsl;

    listener->onConnect = [&]() {
        ISocketIO::Ptr accepted = listener->accept();

        if (!accepted) {
            failed.store(true);
            return;
        }

        serverSsl = SslSocket::create(std::move(accepted), serverContext);

        if (!serverSsl || serverSsl->state() == SslSocket::State::Error) {
            failed.store(true);
            return;
        }

        serverSsl->onEncrypted = [&]() {
            serverEncrypted.store(true);
        };

        serverSsl->onSslError = [&](JobSslError::SslErrNo error, const std::string &) {
            if (JobSslError::isFatalSslError(error))
                failed.store(true);
        };

        if (!serverSsl->startHandshake())
            failed.store(true);
    };

    auto clientSsl = SslSocket::create(clientTransport, clientContext);

    REQUIRE(clientSsl);

    clientSsl->onEncrypted = [&]() {
        clientEncrypted.store(true);
    };

    clientSsl->onSslError = [&](JobSslError::SslErrNo error, const std::string &) {
        if (JobSslError::isFatalSslError(error))
            failed.store(true);
    };

    REQUIRE(clientSsl->connectToHost(JobIpAddr("127.0.0.1", port)));

    const bool handshakeCompleted = waitUntil([&]() {
        return (clientEncrypted.load() && serverEncrypted.load()) || failed.load();
    });

    const bool hadFailure = failed.load();
    const bool clientWasEncrypted = clientEncrypted.load();
    const bool serverWasEncrypted = serverEncrypted.load();

    REQUIRE(handshakeCompleted);
    REQUIRE_FALSE(hadFailure);
    REQUIRE(clientWasEncrypted);
    REQUIRE(serverWasEncrypted);
    REQUIRE(serverSsl);

    const std::string clientPeerAddress = clientSsl->peerAddress();
    const uint16_t clientPeerPort = clientSsl->peerPort();
    const uint16_t clientLocalPort = clientSsl->localPort();
    const uint16_t serverPeerPort = serverSsl->peerPort();
    const uint16_t serverLocalPort = serverSsl->localPort();

    listener->onConnect = nullptr;
    listener->disconnect();

    clientSsl->onEncrypted = nullptr;
    clientSsl->onSslError = nullptr;
    serverSsl->onEncrypted = nullptr;
    serverSsl->onSslError = nullptr;

    clientSsl->disconnect();
    serverSsl->disconnect();

    const bool shutdownCompleted = waitUntil([&]() {
        const auto clientState = clientSsl->state();
        const auto serverState = serverSsl->state();

        const bool clientClosed =
            clientState == SslSocket::State::Closed ||
            clientState == SslSocket::State::Error;

        const bool serverClosed =
            serverState == SslSocket::State::Closed ||
            serverState == SslSocket::State::Error;

        return clientClosed && serverClosed;
    }, 2000ms);

    REQUIRE(clientPeerAddress == "127.0.0.1");
    REQUIRE(clientPeerPort == port);
    REQUIRE(clientLocalPort > 0);
    REQUIRE(serverPeerPort == clientLocalPort);
    REQUIRE(serverLocalPort == port);
    REQUIRE(shutdownCompleted);
}

/*
 * Block two: edge cases and failure behavior
 */

TEST_CASE("SslSocket rejects a handshake before TCP is connected", "[job_net][ssl_socket][edge][state]")
{
    TestLoop loop;

    auto transport = TcpSocket::create(loop.loop);
    auto context = createClientContext();
    auto socket = SslSocket::create(transport, context);

    REQUIRE(socket);
    REQUIRE(socket->state() == SslSocket::State::WaitingForTransport);

    REQUIRE_FALSE(socket->startHandshake());

    REQUIRE(socket->state() == SslSocket::State::Error);
    REQUIRE(socket->lastError() == JobSslError::SslErrNo::InvalidState);
    REQUIRE_FALSE(socket->lastErrorString().empty());
}


// ADD test here for the socket error handlings

TEST_CASE("SslSocket forwards TCP socket options", "[job_net][ssl_socket][edge][options]")
{
    TestLoop loop;

    auto transport = TcpSocket::create(loop.loop);
    auto context = createClientContext();
    auto socket = SslSocket::create(transport, context);

    REQUIRE(socket);

    REQUIRE(transport->bind("127.0.0.1", 0));

    socket->setOption(ISocketIO::SocketOption::ReuseAddress, true);

    REQUIRE(socket->option(ISocketIO::SocketOption::ReuseAddress));

    socket->setOption(ISocketIO::SocketOption::KeepAlive, true);

    REQUIRE(socket->option(ISocketIO::SocketOption::KeepAlive));
}

TEST_CASE("SslSocket reports TCP connection failure", "[job_net][ssl_socket][edge][connect][failure]")
{
    TestLoop loop;

    auto transport = TcpSocket::create(loop.loop);
    auto context = createClientContext();
    auto socket = SslSocket::create(transport, context);

    REQUIRE(socket);

    std::atomic<bool> socketError{false};
    std::atomic<int> socketErrorCode{0};

    socket->onSocketError = [&](int error) {
        socketErrorCode.store(error);
        socketError.store(true);
    };

    REQUIRE(socket->connectToHost(JobIpAddr("127.0.0.1", 65534)));

    REQUIRE(waitUntil([&]() { return socketError.load(); }));

    REQUIRE(socketError.load());
    REQUIRE(socketErrorCode.load() != 0);
    REQUIRE(socket->state() == SslSocket::State::Error);
    REQUIRE_FALSE(socket->isEncrypted());
}

TEST_CASE("SslSocket rejects a plain TCP peer during TLS handshake", "[job_net][ssl_socket][edge][handshake][failure]")
{
    TestLoop loop;

    auto listener = TcpSocket::create(loop.loop);
    auto transport = TcpSocket::create(loop.loop);
    auto context = createClientContext();

    REQUIRE(listener->bind("127.0.0.1", 0));
    REQUIRE(listener->listen());

    const uint16_t port = listener->localPort();

    REQUIRE(port > 0);

    std::shared_ptr<ISocketIO> accepted;
    std::atomic<bool> sslError{false};
    std::atomic<bool> acceptedConnection{false};

    listener->onConnect = [&]() {
        accepted = listener->accept();

        if (!accepted)
            return;

        acceptedConnection.store(true);

        accepted->onRead = [&](const char *, size_t) {
            std::array<char, 4096> buffer{};
            const int64_t count = accepted->read(buffer.data(), buffer.size());

            if (count > 0) {
                static constexpr char INVALID_TLS_RESPONSE[] = "HTTP/1.1 400 Bad Request\r\n" "Content-Length: 0\r\n" "\r\n";

                accepted->write(INVALID_TLS_RESPONSE, sizeof(INVALID_TLS_RESPONSE) - 1);
            }
        };
    };

    auto socket = SslSocket::create(transport, context);

    socket->onSslError = [&](JobSslError::SslErrNo error, const std::string &) {
        if (isFatalSslError(error))
            sslError.store(true);
    };

    REQUIRE(socket->connectToHost(JobIpAddr("127.0.0.1", port)));

    REQUIRE(waitUntil([&]() { return sslError.load() || socket->state() == SslSocket::State::Error; }));

    REQUIRE(acceptedConnection.load());
    REQUIRE_FALSE(socket->isEncrypted());
    REQUIRE(socket->state() == SslSocket::State::Error);
    REQUIRE_FALSE(socket->lastErrorString().empty());

    listener->onConnect = nullptr;
    socket->onSslError = nullptr;

    if (accepted)
        accepted->onRead = nullptr;

    socket->disconnect();

    if (accepted)
        accepted->disconnect();

    listener->disconnect();
}

TEST_CASE("SslSocket rejects an untrusted self-signed certificate", "[job_net][ssl_socket][edge][verification][failure]")
{
    TestLoop loop;
    LocalIdentity identity;

    REQUIRE(generateLocalIdentity(identity));

    auto serverContext = createServerContext(identity);
    auto clientContext = createClientContext(JobSslContext::VerifyMode::Peer);

    REQUIRE(serverContext);
    REQUIRE(clientContext);

    auto listener = TcpSocket::create(loop.loop);
    auto transport = TcpSocket::create(loop.loop);

    REQUIRE(listener->bind("127.0.0.1", 0));
    REQUIRE(listener->listen());

    const uint16_t port = listener->localPort();

    REQUIRE(port > 0);

    std::atomic<bool> serverAccepted{false};
    std::atomic<bool> serverHandshakeStarted{false};
    std::atomic<bool> serverSetupFailed{false};
    std::atomic<bool> clientSslError{false};
    std::atomic<bool> clientEncrypted{false};

    SslSocket::Ptr serverSsl;

    listener->onConnect = [&]() {
        ISocketIO::Ptr accepted = listener->accept();

        if (!accepted) {
            serverSetupFailed.store(true);
            return;
        }

        serverAccepted.store(true);

        serverSsl = SslSocket::create(std::move(accepted), serverContext);

        if (!serverSsl) {
            serverSetupFailed.store(true);
            return;
        }

        serverSsl->onSslError = [](JobSslError::SslErrNo, const std::string &) {};

        serverHandshakeStarted.store(serverSsl->startHandshake());
    };

    auto clientSsl = SslSocket::create(transport, clientContext);

    REQUIRE(clientSsl);

    clientSsl->onEncrypted = [&]() {
        clientEncrypted.store(true);
    };

    clientSsl->onSslError = [&](JobSslError::SslErrNo error, const std::string &) {
        if (isFatalSslError(error))
            clientSslError.store(true);
    };

    REQUIRE(clientSsl->connectToHost(JobIpAddr("127.0.0.1", port)));

    REQUIRE(waitUntil([&]() {
        return serverSetupFailed.load() || clientSslError.load() || clientEncrypted.load()
        || clientSsl->state() == SslSocket::State::Error;
    }));

    REQUIRE_FALSE(serverSetupFailed.load());
    REQUIRE(serverAccepted.load());
    REQUIRE(serverHandshakeStarted.load());
    REQUIRE_FALSE(clientEncrypted.load());

    const bool verificationFailed = clientSslError.load() || clientSsl->state() == SslSocket::State::Error;

    REQUIRE(verificationFailed);
    REQUIRE_FALSE(clientSsl->isEncrypted());

    listener->onConnect = nullptr;
    clientSsl->onEncrypted = nullptr;
    clientSsl->onSslError = nullptr;

    if (serverSsl)
        serverSsl->onSslError = nullptr;

    clientSsl->disconnect();

    if (serverSsl)
        serverSsl->disconnect();

    listener->disconnect();
}

TEST_CASE("SslSocket disconnect is safe before connection", "[job_net][ssl_socket][edge][disconnect]")
{
    TestLoop loop;

    auto transport = TcpSocket::create(loop.loop);
    auto context = createClientContext();
    auto socket = SslSocket::create(transport, context);

    REQUIRE(socket);

    socket->disconnect();

    REQUIRE(socket->state() == SslSocket::State::Closed);
    REQUIRE_FALSE(socket->isOpen());
    REQUIRE_FALSE(socket->isEncrypted());

    socket->disconnect();

    REQUIRE(socket->state() == SslSocket::State::Closed);
}

/*
 * Block three: benchmarks and stress
 */

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("SslSocket local TLS performance", "[job_net][ssl_socket][benchmark]")
{
    BENCHMARK("EC P-256 local TLS handshake and echo") {
        return runLocalTlsEcho("benchmark");
    };

    BENCHMARK("EC P-256 TLS 1 KB echo") {
        const std::string payload(1024, 'J');
        return runLocalTlsEcho(payload);
    };

    BENCHMARK("EC P-256 TLS 4 KB echo") {
        const std::string payload(4096, 'B');
        return runLocalTlsEcho(payload);
    };
}

TEST_CASE("SslSocket repeated local TLS lifecycle remains stable", "[job_net][ssl_socket][stress]")
{
    constexpr size_t ITERATIONS = 50;

    for (size_t iteration = 0; iteration < ITERATIONS; ++iteration) {
        INFO("TLS lifecycle iteration: " << iteration);
        REQUIRE(runLocalTlsEcho("stress"));
    }
}

#endif