#include <fstream>
#include <iostream>
#include <chrono>
#include <memory>
#include <array>

#include <meta>
#include <contracts>

// #include "obj_concept.h"

#include "ping.h"
#include "pong.h"
#include "ser_obj.h"
#include "ser_nested_obj.h"
#include "tmp_file.h"

#include "packed.h"
#include "ipc_mmap.h"
#include "ipc_shm.h"

#include "test_loop.h"
#include "ipc_tcp.h"
#include "ipc_unix.h"
#include "ipc_ssl.h"
#include "ipc_udp.h"


#include <iostream>
#include <iomanip>
#include <vector>
#include <span>

#include "ping.h"
#include "pong.h"
#include "ser_nested_obj.h"
#include "ser_obj.h"

#include <cassert>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <span>
#include <vector>

#include "ping.h"
#include "pong.h"
#include "ser_nested_obj.h"
#include "ser_obj.h"
#include "ipc_shm.h"

// =============================================================================
// 1. Core Signal/Slot & Scope Lifetime Tests
// =============================================================================
void runSignalSlotTests() {
    std::cout << "\n=== 1. Signal / Slot & Lifetime Tests ===\n";

    auto ping = makeUniq<Ping>();
    auto pong = makeUniq<Pong>();

    std::cout << "Ping UID: " << ping->uid() << " | Pong UID: " << pong->uid() << "\n";

    connect<&Ping::pingChanged, &Pong::handlePing>(*ping, *pong);
    connect<&Pong::pongChanged, &Ping::handlePong>(*pong, *ping);

    ping->emit(42);
    pong->emit(84);

    ping->debugJson();
    pong->debugYaml();

    // Scope disconnection check
    {
        auto scopedPong = makeUniq<Pong>();
        connect<&Ping::pingChanged, &Pong::handlePing>(*ping, *scopedPong);
        assert(ping->pingChanged.connectionCount() == 2);
    }
    assert(ping->pingChanged.connectionCount() == 1);
    std::cout << "Signal/Slot RAII disconnection verified.\n";
}

// =============================================================================
// 2. Reflection Serialization Tests (JSON / YAML / Binary)
// =============================================================================
void runSerializationTests() {
    std::cout << "\n=== 2. Reflection Serialization Tests ===\n";

    auto ser = makeUniq<SerObj>();
    ser->setName("RootJobEntity");
    ser->setCount(42);
    ser->setValue(13.37f);
    ser->setFloatList({1.0f, 2.5f, 5.25f, 10.125f});

    ser->nestedObject().setName("DirectChild");
    ser->nestedObject().setId(101);
    ser->nestedObject().setWeight(75.5f);
    ser->nestedObject().setEnabled(true);
    ser->nestedObject().setSomeEnum(SomeEnum::Car);

    auto childA = makeShared<SerNestedObj>();
    childA->setName("SharedWorker_0");
    childA->setId(201);
    childA->setWeight(12.3f);
    childA->setEnabled(true);
    childA->setSomeEnum(SomeEnum::Foo);

    auto childB = makeShared<SerNestedObj>();
    childB->setName("SharedWorker_1");
    childB->setId(202);
    childB->setWeight(98.7f);
    childB->setEnabled(false);
    childB->setSomeEnum(SomeEnum::Bar);

    ser->nestedObjects().push_back(std::move(childA));
    ser->nestedObjects().push_back(std::move(childB));

    ser->debugJson();
    ser->debugYaml();

    std::vector<uint8_t> binaryBuffer;
    ser->toBinary(binaryBuffer);
    std::cout << "Packed Binary Size: " << binaryBuffer.size() << " bytes\n";

    auto restored = makeUniq<SerObj>();
    std::span<const uint8_t> streamSpan(binaryBuffer);
    const bool ok = restored->fromBinary(streamSpan);

    assert(ok);
    assert(restored->name() == "RootJobEntity");
    assert(restored->nestedObjects().size() == 2);
    assert(restored->nestedObjects()[0]->name() == "SharedWorker_0");
    std::cout << "In-memory binary roundtrip verified.\n";

}

// =============================================================================
// Main Entrypoint
// =============================================================================
int main() {
    std::cout << "========================================================\n";
    std::cout << "     Joseph's Odd Builder - Reflection & IPC Suite     \n";
    std::cout << "========================================================\n";

    runSignalSlotTests();
    runSerializationTests();

    std::cout << "\nAll test suites completed successfully!\n";
    return 0;
}


#ifdef UDP_TEST
int main()
{
    TestLoop loop;

    constexpr std::size_t Messages = 100'000;

    Packed src;
    src.type = LayerType::Attention;
    src.activation = ActivationType::GELU;
    src.inputs = 4096;
    src.outputs = 8192;
    src.weightOffset = 128;
    src.weightCount = 16384;
    src.biasOffset = 16512;
    src.biasCount = 4096;
    src.auxiliaryData = 8;

    std::atomic<bool> clientConnected{false};
    std::atomic<bool> socketError{false};
    std::atomic<bool> finished{false};
    std::atomic<std::size_t> echoes{0};

    JobUrl url("udp://127.0.0.1:0");

    auto server = std::make_shared<UdpServer>(loop.loop);
    auto client = std::make_shared<UdpClient>(loop.loop);

    std::weak_ptr<UdpServer> weakServer = server;

    server->onMessage = [weakServer, &socketError](const char *data, std::size_t len, const JobIpAddr &sender) {
        auto server = weakServer.lock();
        if (!server)
            return;

        Packed packed;
        if (!PackedUdp::read(data, len, packed)) {
            socketError.store(true, std::memory_order_release);
            return;
        }

        const ssize_t written = server->sendTo(PackedUdp::data(packed), PackedUdp::size(), sender);
        if (written != static_cast<ssize_t>(PackedUdp::size()))
            socketError.store(true, std::memory_order_release);
    };

    const bool started = server->start(url);
    contract_assert(started);
    contract_assert(server->isRunning());
    contract_assert(server->port() > 0);

    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point start;
    Clock::time_point end;

    std::weak_ptr<UdpClient> weakClient = client;

    client->onConnect = [&]() {
        clientConnected.store(true, std::memory_order_release);

        start = Clock::now();

        const ssize_t written = client->send(PackedUdp::data(src), PackedUdp::size());
        if (written != static_cast<ssize_t>(PackedUdp::size()))
            socketError.store(true, std::memory_order_release);
    };

    client->onMessage = [weakClient, &src, &echoes, &finished, &socketError, &end](const char *data, std::size_t len) {
        auto client = weakClient.lock();
        if (!client)
            return;

        Packed packed;
        if (!PackedUdp::read(data, len, packed)) {
            socketError.store(true, std::memory_order_release);
            return;
        }

        asm volatile("" : : "g"(&packed) : "memory");

        const std::size_t count = echoes.fetch_add(1, std::memory_order_relaxed) + 1;

        if (count == Messages) {
            end = Clock::now();
            finished.store(true, std::memory_order_release);
            return;
        }

        const ssize_t written = client->send(PackedUdp::data(src), PackedUdp::size());
        if (written != static_cast<ssize_t>(PackedUdp::size()))
            socketError.store(true, std::memory_order_release);
    };

    const JobIpAddr clientAddr("127.0.0.1", server->port());
    const bool connecting = client->connectToHost(clientAddr);
    contract_assert(connecting);

    while (!finished.load(std::memory_order_acquire) &&
           !socketError.load(std::memory_order_acquire))
        std::this_thread::yield();

    contract_assert(clientConnected.load(std::memory_order_acquire));
    contract_assert(!socketError.load(std::memory_order_acquire));

    const std::size_t received = echoes.load(std::memory_order_acquire);
    contract_assert(received == Messages);

    const auto totalNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    const double totalSeconds = static_cast<double>(totalNs) / 1'000'000'000.0;
    const double packedPerSecond = static_cast<double>(Messages) / totalSeconds;
    const double oneWayMiBPerSecond = (packedPerSecond * sizeof(Packed)) / (1024.0 * 1024.0);
    const double roundTripMiBPerSecond = (packedPerSecond * sizeof(Packed) * 2.0) / (1024.0 * 1024.0);

    std::cout << "\nreflected Packed UDP:\n";
    dumpPacked(src);

    std::cout << "\nstructure size: " << sizeof(Packed) << " bytes\n";
    std::cout << Messages << " UDP echo transfers: " << totalNs << " ns\n";
    std::cout << "Packed/sec: " << packedPerSecond << '\n';
    std::cout << "one-way payload: " << oneWayMiBPerSecond << " MiB/s\n";
    std::cout << "roundtrip payload: " << roundTripMiBPerSecond << " MiB/s\n";
    std::cout << "echoes: " << received << '\n';

    if (client->isConnected())
        client->disconnect();

    if (server->isRunning())
        server->stop();

    return 0;
}
#endif




#ifdef SHM_TEST
int main()
{
    constexpr std::size_t Iterations = 1'000'000;

    Packed src;
    src.type            = LayerType::Attention;
    src.activation      = ActivationType::GELU;
    src.inputs          = 4096;
    src.outputs         = 8192;
    src.weightOffset    = 128;
    src.weightCount     = 16384;
    src.biasOffset      = 16512;
    src.biasCount       = 4096;
    src.auxiliaryData   = 8;

    SharedPacked shared;
    const bool opened = shared.open("/job-packed-shm");
    contract_assert(opened);

    Packed *mapped = shared.asPackedMut();
    contract_assert(mapped);

    using Clock = std::chrono::high_resolution_clock;

    const auto writeStart = Clock::now();
    for (std::size_t i = 0; i < Iterations; ++i) {
        *mapped = src;
        mapped->inputs = static_cast<std::uint32_t>(4096 + (i & 1));
        asm volatile("" : : "g"(mapped) : "memory");
    }
    const auto writeEnd = Clock::now();

    const Packed *read = shared.asPacked();
    contract_assert(read);

    std::uint64_t checksum = 0;

    const auto readStart = Clock::now();
    for (std::size_t i = 0; i < Iterations; ++i) {
        asm volatile("" : : : "memory");

        checksum += read->inputs;
        checksum += read->outputs;
        checksum += read->weightCount;
        checksum += read->biasCount;
        checksum += read->auxiliaryData;
    }
    const auto readEnd = Clock::now();

    contract_assert(read->type == LayerType::Attention);
    contract_assert(read->activation == ActivationType::GELU);
    contract_assert(read->inputs == 4097);
    contract_assert(read->outputs == 8192);
    contract_assert(read->weightOffset == 128);
    contract_assert(read->weightCount == 16384);
    contract_assert(read->biasOffset == 16512);
    contract_assert(read->biasCount == 4096);
    contract_assert(read->auxiliaryData == 8);
    contract_assert(checksum != 0);

    std::cout << "\nreflected Packed SHM:\n";
    dumpPacked(*read);

    const auto writeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(writeEnd - writeStart).count();
    const auto readNs = std::chrono::duration_cast<std::chrono::nanoseconds>(readEnd - readStart).count();

    std::cout << "\nstructure size: " << sizeof(Packed) << " bytes\n";
    std::cout << Iterations << " SHM writes: " << writeNs << " ns\n";
    std::cout << Iterations << " SHM reads:  " << readNs << " ns\n";
    std::cout << "write avg: " << static_cast<double>(writeNs) / Iterations << " ns\n";
    std::cout << "read avg:  " << static_cast<double>(readNs) / Iterations << " ns\n";
    std::cout << "checksum: " << checksum << '\n';

    shared.close();

    return 0;
}
#endif

#ifdef SSL_TEST
int main()
{
    TestLoop loop;

    constexpr std::size_t Messages = 100000;

    Packed src;
    src.type = LayerType::Attention;
    src.activation = ActivationType::GELU;
    src.inputs = 4096;
    src.outputs = 8192;
    src.weightOffset = 128;
    src.weightCount = 16384;
    src.biasOffset = 16512;
    src.biasCount = 4096;
    src.auxiliaryData = 8;

    std::string certificatePath;
    std::string privateKeyPath;

    {
        PackedSslCert cert;

        certificatePath = cert.certificatePath();
        privateKeyPath = cert.privateKeyPath();

        const bool generated = cert.generate();
        contract_assert(generated);
        contract_assert(std::filesystem::exists(certificatePath));
        contract_assert(std::filesystem::exists(privateKeyPath));

        auto serverContext = cert.createServerContext();
        auto clientContext = cert.createClientContext();

        contract_assert(serverContext);
        contract_assert(clientContext);

        std::atomic<bool> serverConnected{false};
        std::atomic<bool> serverEncrypted{false};
        std::atomic<bool> serverDisconnected{false};
        std::atomic<bool> clientConnected{false};
        std::atomic<bool> clientEncrypted{false};
        std::atomic<bool> clientDisconnected{false};
        std::atomic<bool> socketError{false};
        std::atomic<bool> sslError{false};
        std::atomic<bool> finished{false};
        std::atomic<std::size_t> echoes{0};

        auto serverReader = PackedSslReader::createShared();
        auto clientReader = PackedSslReader::createShared();

        auto server = std::make_shared<job::net::SslServer>(loop.loop, serverContext);
        std::weak_ptr<job::net::SslClient> serverClient;

        server->onClientConnected = [&](const job::net::SslClient::Ptr &client) {
            serverConnected.store(true, std::memory_order_release);
            serverClient = client;
        };

        server->onClientEncrypted = [&](const job::net::SslClient::Ptr &) {
            serverEncrypted.store(true, std::memory_order_release);
        };

        serverReader->setCallback([&](const Packed &packed) {
            const auto client = serverClient.lock();
            if (!client) {
                socketError.store(true, std::memory_order_release);
                return;
            }

            const int64_t sent = client->send(PackedSslReader::data(packed), PackedSslReader::size());
            if (sent != static_cast<int64_t>(PackedSslReader::size()))
                socketError.store(true, std::memory_order_release);
        });

        server->onClientMessage = [&](const job::net::SslClient::Ptr &, const char *data, std::size_t len) {
            serverReader->read(data, len);
        };

        server->onClientDisconnected = [&](const job::net::SslClient::Ptr &) {
            serverDisconnected.store(true, std::memory_order_release);
        };

        server->onSocketError = [&](int) {
            socketError.store(true, std::memory_order_release);
        };

        server->onSslError = [&](const job::net::SslClient::Ptr &, job::net::JobSslError::SslErrNo error, const std::string &) {
            if (job::net::JobSslError::isFatalSslError(error))
                sslError.store(true, std::memory_order_release);
        };

        const bool started = server->start("127.0.0.1", 0);
        contract_assert(started);
        contract_assert(server->isRunning());
        contract_assert(server->port() > 0);

        auto client = std::make_shared<job::net::SslClient>(loop.loop, clientContext);

        using Clock = std::chrono::high_resolution_clock;
        Clock::time_point start;
        Clock::time_point end;

        client->onConnect = [&]() {
            clientConnected.store(true, std::memory_order_release);
        };

        client->onEncrypted = [&]() {
            clientEncrypted.store(true, std::memory_order_release);

            start = Clock::now();

            const int64_t sent = client->send(PackedSslReader::data(src), PackedSslReader::size());
            if (sent != static_cast<int64_t>(PackedSslReader::size()))
                socketError.store(true, std::memory_order_release);
        };

        clientReader->setCallback([&](const Packed &packed) {
            asm volatile("" : : "g"(&packed) : "memory");

            const std::size_t count = echoes.fetch_add(1, std::memory_order_relaxed) + 1;

            if (count == Messages) {
                end = Clock::now();
                finished.store(true, std::memory_order_release);
                return;
            }

            const int64_t sent = client->send(PackedSslReader::data(src), PackedSslReader::size());
            if (sent != static_cast<int64_t>(PackedSslReader::size()))
                socketError.store(true, std::memory_order_release);
        });

        client->onMessage = [&](const char *data, std::size_t len) {
            clientReader->read(data, len);
        };

        client->onDisconnect = [&]() {
            clientDisconnected.store(true, std::memory_order_release);
        };

        client->onSocketError = [&](int) {
            socketError.store(true, std::memory_order_release);
        };

        client->onSslError = [&](job::net::JobSslError::SslErrNo error, const std::string &) {
            if (job::net::JobSslError::isFatalSslError(error))
                sslError.store(true, std::memory_order_release);
        };

        const bool connecting = client->connectToHost(job::net::JobIpAddr("127.0.0.1", server->port()));
        contract_assert(connecting);

        while (!finished.load(std::memory_order_acquire) &&
               !socketError.load(std::memory_order_acquire) &&
               !sslError.load(std::memory_order_acquire))
            std::this_thread::yield();

        contract_assert(!socketError.load(std::memory_order_acquire));
        contract_assert(!sslError.load(std::memory_order_acquire));
        contract_assert(serverConnected.load(std::memory_order_acquire));
        contract_assert(serverEncrypted.load(std::memory_order_acquire));
        contract_assert(clientConnected.load(std::memory_order_acquire));
        contract_assert(clientEncrypted.load(std::memory_order_acquire));

        const std::size_t received = echoes.load(std::memory_order_acquire);
        contract_assert(received == Messages);

        const auto totalNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        const double totalSeconds = static_cast<double>(totalNs) / 1'000'000'000.0;
        const double packedPerSecond = static_cast<double>(Messages) / totalSeconds;
        const double oneWayMiBPerSecond = (packedPerSecond * sizeof(Packed)) / (1024.0 * 1024.0);
        const double roundTripMiBPerSecond = (packedPerSecond * sizeof(Packed) * 2.0) / (1024.0 * 1024.0);

        std::cout << "\nreflected Packed SSL:\n";
        dumpPacked(src);

        std::cout << "\nstructure size: " << sizeof(Packed) << " bytes\n";
        std::cout << Messages << " SSL echo transfers: " << totalNs << " ns\n";
        std::cout << "Packed/sec: " << packedPerSecond << '\n';
        std::cout << "one-way payload: " << oneWayMiBPerSecond << " MiB/s\n";
        std::cout << "roundtrip payload: " << roundTripMiBPerSecond << " MiB/s\n";
        std::cout << "echoes: " << received << '\n';

        client->disconnect();

        int retries = 0;
        while (!clientDisconnected.load(std::memory_order_acquire) && retries < 2000) {
            std::this_thread::sleep_for(1ms);
            ++retries;
        }

        retries = 0;
        while (!serverDisconnected.load(std::memory_order_acquire) && retries < 2000) {
            std::this_thread::sleep_for(1ms);
            ++retries;
        }

        contract_assert(clientDisconnected.load(std::memory_order_acquire));
        contract_assert(serverDisconnected.load(std::memory_order_acquire));

        server->stop();
    }

    contract_assert(!std::filesystem::exists(certificatePath));
    contract_assert(!std::filesystem::exists(privateKeyPath));

    return 0;
}
#endif

#ifdef UNIX_TEST
int main()
{
    TestLoop loop;

    constexpr std::size_t Messages = 10'000;

    const std::string path = make_temp_sock_path("packed_unix");

    Packed src;
    src.type = LayerType::Attention;
    src.activation = ActivationType::GELU;
    src.inputs = 4096;
    src.outputs = 8192;
    src.weightOffset = 128;
    src.weightCount = 16384;
    src.biasOffset = 16512;
    src.biasCount = 4096;
    src.auxiliaryData = 8;

    std::atomic<bool> clientConnected{false};
    std::atomic<bool> clientDisconnected{false};
    std::atomic<bool> serverSawDisconnect{false};
    std::atomic<bool> finished{false};
    std::atomic<std::size_t> echoes{0};

    auto server = PackedUnixServer::createShared(loop.loop);

    server->onPacked = [&](job::net::UnixClient::Ptr client, const Packed &packed) {
        server->send(client, packed);
    };

    server->onClientDisconnected = [&](job::net::UnixClient::Ptr) {
        serverSawDisconnect.store(true, std::memory_order_release);
    };

    const bool started = server->start(path, 0);
    contract_assert(started);
    contract_assert(server->isRunning());

    auto client = PackedUnixClient::createShared(loop.loop);

    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point start;
    Clock::time_point end;

    client->onConnect = [&]() {
        clientConnected.store(true, std::memory_order_release);

        start = Clock::now();
        client->send(src);
    };

    client->onPacked = [&](const Packed &packed) {
        asm volatile("" : : "g"(&packed) : "memory");

        const std::size_t count = echoes.fetch_add(1, std::memory_order_relaxed) + 1;

        if (count == Messages) {
            end = Clock::now();
            finished.store(true, std::memory_order_release);
            return;
        }

        client->send(src);
    };

    client->onDisconnect = [&]() {
        clientDisconnected.store(true, std::memory_order_release);
    };

    const bool connecting = client->connect(path);
    contract_assert(connecting);

    while (!finished.load(std::memory_order_acquire))
        std::this_thread::yield();

    const auto totalNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    const double totalSeconds = static_cast<double>(totalNs) / 1'000'000'000.0;
    const double packedPerSecond = static_cast<double>(Messages) / totalSeconds;
    const double oneWayMiBPerSecond = (packedPerSecond * sizeof(Packed)) / (1024.0 * 1024.0);
    const double roundTripMiBPerSecond = (packedPerSecond * sizeof(Packed) * 2.0) / (1024.0 * 1024.0);

    const std::size_t received = echoes.load(std::memory_order_acquire);
    contract_assert(received == Messages);

    std::cout << "\nreflected Packed Unix async:\n";
    dumpPacked(src);

    std::cout << "\nstructure size: " << sizeof(Packed) << " bytes\n";
    std::cout << Messages << " Unix echo transfers: " << totalNs << " ns\n";
    std::cout << "Packed/sec: " << packedPerSecond << '\n';
    std::cout << "one-way payload: " << oneWayMiBPerSecond << " MiB/s\n";
    std::cout << "roundtrip payload: " << roundTripMiBPerSecond << " MiB/s\n";
    std::cout << "echoes: " << received << '\n';

    client->disconnect();

    int retries = 0;
    while (!clientDisconnected.load(std::memory_order_acquire) && retries < 2000) {
        std::this_thread::sleep_for(1ms);
        ++retries;
    }

    retries = 0;
    while (!serverSawDisconnect.load(std::memory_order_acquire) && retries < 2000) {
        std::this_thread::sleep_for(1ms);
        ++retries;
    }

    contract_assert(clientDisconnected.load(std::memory_order_acquire));
    contract_assert(serverSawDisconnect.load(std::memory_order_acquire));

    server->stop();

    return 0;
}
#endif



#ifdef TCP_TEST
int main()
{
    TestLoop loop;

    constexpr std::size_t Messages = 10'000;

    Packed src;
    src.type = LayerType::Attention;
    src.activation = ActivationType::GELU;
    src.inputs = 4096;
    src.outputs = 8192;
    src.weightOffset = 128;
    src.weightCount = 16384;
    src.biasOffset = 16512;
    src.biasCount = 4096;
    src.auxiliaryData = 8;

    std::atomic<bool> clientConnected{false};
    std::atomic<bool> clientDisconnected{false};
    std::atomic<bool> serverSawDisconnect{false};
    std::atomic<std::size_t> echoes{0};

    auto server = std::make_shared<TcpServer>(loop.loop);

    server->onClientConnected = [&](TcpClient::Ptr client) {
        auto reader = std::make_shared<PackedTcpReader>();
        std::weak_ptr<TcpClient> weakClient = client;

        reader->setCallback([weakClient](const Packed &packed) {
            auto client = weakClient.lock();
            if (!client)
                return;

            client->send(PackedTcpReader::data(packed), PackedTcpReader::size());
        });

        client->onMessage = [reader](const char *data, std::size_t len) {
            reader->read(data, len);
        };

        client->onDisconnect = [&]() {
            serverSawDisconnect.store(true, std::memory_order_release);
        };
    };

    const bool started = server->start("127.0.0.1", 0);
    contract_assert(started);

    const std::uint16_t port = server->port();
    contract_assert(port > 0);

    auto client = std::make_shared<TcpClient>(loop.loop);
    auto reader = std::make_shared<PackedTcpReader>();

    reader->setCallback([&echoes](const Packed &packed) {
        asm volatile("" : : "g"(&packed) : "memory");
        echoes.fetch_add(1, std::memory_order_release);
    });

    client->onConnect = [&]() {
        clientConnected.store(true, std::memory_order_release);
    };

    client->onMessage = [reader](const char *data, std::size_t len) {
        reader->read(data, len);
    };

    client->onDisconnect = [&]() {
        clientDisconnected.store(true, std::memory_order_release);
    };

    const JobIpAddr address("127.0.0.1", port);
    const bool connecting = client->connectToHost(address);
    contract_assert(connecting);

    while (!clientConnected.load(std::memory_order_acquire))
        std::this_thread::yield();

    echoes.store(0, std::memory_order_relaxed);

    using Clock = std::chrono::high_resolution_clock;
    const auto start = Clock::now();

    for (std::size_t i = 0; i < Messages; ++i) {
        while (!client->send(PackedTcpReader::data(src), PackedTcpReader::size()))
            std::this_thread::yield();
    }

    while (echoes.load(std::memory_order_acquire) != Messages)
        std::this_thread::yield();

    const auto end = Clock::now();

    const auto totalNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    const double totalSeconds = static_cast<double>(totalNs) / 1'000'000'000.0;
    const double packedPerSecond = static_cast<double>(Messages) / totalSeconds;
    const double oneWayMiBPerSecond = (packedPerSecond * sizeof(Packed)) / (1024.0 * 1024.0);
    const double roundTripMiBPerSecond = (packedPerSecond * sizeof(Packed) * 2.0) / (1024.0 * 1024.0);

    const std::size_t received = echoes.load(std::memory_order_acquire);

    contract_assert(received == Messages);

    std::cout << "\nreflected Packed TCP flood:\n";
    dumpPacked(src);

    std::cout << "\nstructure size: " << sizeof(Packed) << " bytes\n";
    std::cout << Messages << " TCP echo transfers: " << totalNs << " ns\n";
    std::cout << "Packed/sec: " << packedPerSecond << '\n';
    std::cout << "one-way payload: " << oneWayMiBPerSecond << " MiB/s\n";
    std::cout << "roundtrip payload: " << roundTripMiBPerSecond << " MiB/s\n";
    std::cout << "echoes: " << received << '\n';

    client->disconnect();

    int retries = 0;
    while (!clientDisconnected.load(std::memory_order_acquire) && retries < 2000) {
        std::this_thread::sleep_for(1ms);
        ++retries;
    }

    retries = 0;
    while (!serverSawDisconnect.load(std::memory_order_acquire) && retries < 2000) {
        std::this_thread::sleep_for(1ms);
        ++retries;
    }

    contract_assert(clientDisconnected.load(std::memory_order_acquire));
    contract_assert(serverSawDisconnect.load(std::memory_order_acquire));

    server->stop();

    return 0;
}
#endif





#if IPC_MMAP
int main()
{
    const std::string path = "/tmp/job-packed-mmap.bin";
    constexpr int iterations = 1'000'000;

    TmpFile tmp(path, sizeof(Packed), '\0');

    Packed src;
    src.type = LayerType::Attention;
    src.activation = ActivationType::GELU;
    src.inputs = 4096;
    src.outputs = 8192;
    src.weightOffset = 128;
    src.weightCount = 16384;
    src.biasOffset = 16512;
    src.biasCount = 4096;
    src.auxiliaryData = 8;

    {
        MappedFile file(tmp.path(), sizeof(Packed));
        Packed *mapped = file.asPackedMut();

        using Clock = std::chrono::high_resolution_clock;

        const auto writeStart = Clock::now();
        for (int i = 0; i < iterations; ++i) {
            *mapped = src;
            mapped->inputs = static_cast<std::uint32_t>(4096 + (i & 1));
            asm volatile("" : : "g"(mapped) : "memory");
        }
        const auto writeEnd = Clock::now();

        std::uint64_t checksum = 0;
        const Packed *read = file.asPacked();

        const auto readStart = Clock::now();
        for (int i = 0; i < iterations; ++i) {
            asm volatile("" : : : "memory"); // agressive as fuck lol

            checksum += read->inputs;
            checksum += read->outputs;
            checksum += read->weightCount;
            checksum += read->biasCount;
            checksum += read->auxiliaryData;
        }
        const auto readEnd = Clock::now();

        contract_assert(read->type == LayerType::Attention);
        contract_assert(read->activation == ActivationType::GELU);
        contract_assert(read->inputs == 4097);
        contract_assert(read->outputs == 8192);
        contract_assert(read->weightOffset == 128);
        contract_assert(read->weightCount == 16384);
        contract_assert(read->biasOffset == 16512);
        contract_assert(read->biasCount == 4096);
        contract_assert(read->auxiliaryData == 8);
        contract_assert(checksum != 0);

        std::cout << "\nreflected Packed:\n";
        dumpPacked(*read);

        const auto writeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(writeEnd - writeStart).count();
        const auto readNs = std::chrono::duration_cast<std::chrono::nanoseconds>(readEnd - readStart).count();

        std::cout << "\nstructure size: " << sizeof(Packed) << " bytes\n";
        std::cout << iterations << " mapped writes: " << writeNs << " ns\n";
        std::cout << iterations << " mapped reads:  " << readNs << " ns\n";
        std::cout << "write avg: " << static_cast<double>(writeNs) / iterations << " ns\n";
        std::cout << "read avg:  " << static_cast<double>(readNs) / iterations << " ns\n";
        std::cout << "checksum: " << checksum << '\n';
    }

    return 0;
}
#endif


// int main()
// {
//     Packed src;
//     src.type = LayerType::Attention;
//     src.activation = ActivationType::GELU;
//     src.inputs = 4096;
//     src.outputs = 4096;
//     src.weightOffset = 128;
//     src.weightCount = 16384;
//     src.biasOffset = 16512;
//     src.biasCount = 4096;
//     src.auxiliaryData = 8;

//     static_assert(sizeof(Packed) == 32);
//     static_assert(std::is_trivially_copyable_v<Packed>);
//     static_assert(std::is_standard_layout_v<Packed>);

//     const std::string path = "/tmp/job-packed-test.bin";
//     constexpr int iterations = 100;


//     using Clock = std::chrono::high_resolution_clock;
//     const auto writeStart = Clock::now();
//     for (int i = 0; i < iterations; ++i) {
//         if (!Serializer::save(src, path))
//             return 1;
//     }
//     const auto writeEnd = Clock::now();
//     Packed dst{};
//     const auto readStart = Clock::now();
//     for (int i = 0; i < iterations; ++i)
//         dst = Serializer::load(path);
//     const auto readEnd = Clock::now();

//     contract_assert(dst.type == LayerType::Attention);
//     contract_assert(dst.activation == ActivationType::GELU);
//     contract_assert(dst.inputs == 4096);
//     contract_assert(dst.outputs == 4096);
//     contract_assert(dst.weightOffset == 128);
//     contract_assert(dst.weightCount == 16384);
//     contract_assert(dst.biasOffset == 16512);
//     contract_assert(dst.biasCount == 4096);
//     contract_assert(dst.auxiliaryData == 8);

//     const auto writeUs =
//         std::chrono::duration_cast<std::chrono::microseconds>(writeEnd - writeStart).count();

//     const auto readUs =
//         std::chrono::duration_cast<std::chrono::microseconds>(readEnd - readStart).count();

//     std::cout << "structure size: " << sizeof(Packed) << " bytes\n";
//     std::cout << "100 binary writes: " << writeUs << " us\n";
//     std::cout << "100 binary reads:  " << readUs << " us\n";
//     std::cout << "write avg: " << static_cast<double>(writeUs) / iterations << " us\n";
//     std::cout << "read avg:  " << static_cast<double>(readUs) / iterations << " us\n";

//     std::remove(path.c_str());

//     return 0;
// }

// int main()
// {
//     Pong pong;
//     {
//         Ping ping;
//         connect<&Ping::pingChanged, &Pong::handlePing>(ping, pong);
//         for(int i = 0; i <= 10; ++i )
//             ping.emit(i);
//     }

//     return 0;
// }
// divide(10, 0);










