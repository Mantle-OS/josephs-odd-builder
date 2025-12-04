#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <unistd.h>
#include <thread>
#include <atomic>
#include <cstring>
#include <filesystem>
#include <vector>

// Your Architecture Headers
#include <servers/unix_socket_server.h>
#include <clients/unix_socket_client.h>
#include <genome.h>

using namespace job::net;
using namespace job::ai::evo;
using namespace job::ai::layers;
namespace fs = std::filesystem;

class RawGenomeSerializer {
public:
    RawGenomeSerializer(char *buffer, size_t cap) :
        m_ptr(buffer),
        m_cap(cap),
        m_offset(0)
    {

    }

    bool write(const Genome &g)
    {
        if (!writePOD(g.header))
            return false;

        uint32_t archCount = static_cast<uint32_t>(g.architecture.size());
        if (!writePOD(archCount))
            return false;

        size_t archBytes = archCount * sizeof(LayerGene);
        if (!writeBytes(g.architecture.data(), archBytes))
            return false;

        uint32_t weightCount = static_cast<uint32_t>(g.weights.size());
        if (!writePOD(weightCount))
            return false;

        size_t weightBytes = weightCount * sizeof(float);
        if (!writeBytes(g.weights.data(), weightBytes))
            return false;

        return true;
    }
    size_t bytesWritten() const
    {
        return m_offset;
    }

private:
    template<typename T>
    bool writePOD(const T &val)
    {
        if (m_offset + sizeof(T) > m_cap)
            return false;

        std::memcpy(m_ptr + m_offset, &val, sizeof(T));
        m_offset += sizeof(T);
        return true;
    }
    bool writeBytes(const void *src, size_t len)
    {
        if (m_offset + len > m_cap) return false;
        std::memcpy(m_ptr + m_offset, src, len);
        m_offset += len;
        return true;
    }
    char *m_ptr;
    size_t m_cap;
    size_t m_offset;
};

class RawGenomeDeserializer {
public:
    RawGenomeDeserializer(const char* buffer, size_t size) :
        m_ptr(buffer),
        m_size(size),
        m_offset(0)
    {

    }

    bool read(Genome &outG)
    {
        if (!readPOD(outG.header))
            return false;

        uint32_t archCount = 0;
        if (!readPOD(archCount))
            return false;

        outG.architecture.resize(archCount);
        size_t archBytes = archCount * sizeof(LayerGene);
        if (!readBytes(outG.architecture.data(), archBytes))
            return false;

        uint32_t weightCount = 0;
        if (!readPOD(weightCount))
            return false;

        outG.weights.resize(weightCount);
        size_t weightBytes = weightCount * sizeof(float);
        if (!readBytes(outG.weights.data(), weightBytes))
            return false;

        return true;
    }

private:
    template<typename T>
    bool readPOD(T &outVal)
    {
        if (m_offset + sizeof(T) > m_size)
            return false;

        std::memcpy(&outVal, m_ptr + m_offset, sizeof(T));
        m_offset += sizeof(T);
        return true;
    }
    bool readBytes(void* dst, size_t len)
    {
        if (m_offset + len > m_size)
            return false;
        std::memcpy(dst, m_ptr + m_offset, len);
        m_offset += len;
        return true;
    }
    const char *m_ptr;
    size_t m_size;
    size_t m_offset;
};

Genome createTestGenome()
{
    Genome g;
    g.header.uuid = 0xCAFEBABE;
    g.header.fitness = 42.0f;

    // Dense Layer
    LayerGene l1{};
    l1.type = LayerType::Dense;
    l1.inputs = 128;
    l1.outputs = 64;
    l1.weight_count = 128*64;
    g.architecture.push_back(l1);

    // Fill weights
    g.weights.resize(l1.weight_count);
    for(size_t i=0; i<g.weights.size(); ++i)
        g.weights[i] = i * 0.01f;

    return g;
}

using namespace job::net;
using namespace job::threads;
using namespace std::chrono_literals;

struct TestLoop {
    std::shared_ptr<JobIoAsyncThread> loop;
    TestLoop()
    {
        job::core::JobLogger::instance().setLevel(job::core::LogLevel::Info);
        loop = std::make_shared<JobIoAsyncThread>();
        loop->start();
        REQUIRE(loop->isRunning());
    }

    ~TestLoop()
    {
        if(loop->isRunning())
            loop->stop();
        REQUIRE_FALSE(loop->isRunning());
    }
};

inline static std::string make_temp_sock_path(const std::string &base)
{
    std::string path = "/tmp/" + base + "_" + std::to_string(::getpid()) + ".sock";
    if (std::filesystem::exists(path))
        std::filesystem::remove(path);
    return path;
}

TEST_CASE("Unix Socket: AI Genome Transfer", "[unix][ai][genome]")
{
    TestLoop loop;
    auto path = make_temp_sock_path("unix_ai_genome");

    // 1. Prepare Data
    Genome original = createTestGenome();

    // FIX: Buffer size increased to 64KB (Weights alone are 32KB)
    char sendBuffer[65536];
    RawGenomeSerializer ser(sendBuffer, sizeof(sendBuffer));

    REQUIRE(ser.write(original));
    size_t totalSize = ser.bytesWritten();

    // 2. Setup Server
    auto server = std::make_shared<UnixServer>(loop.loop);

    std::atomic<bool> packetReceived{false};
    std::atomic<bool> serverSawDisconnect{false};

    server->onClientConnected = [&](UnixClient::UnixClientPtr client) {
        auto accumulator = std::make_shared<std::vector<char>>();
        accumulator->reserve(totalSize);

        client->onMessage = [client, accumulator, totalSize, &packetReceived](const char* data, size_t len) {
            accumulator->insert(accumulator->end(), data, data + len);

            if (accumulator->size() >= totalSize) {
                // Deserialize & Verify logic here if needed
                packetReceived.store(true);
                accumulator->clear(); // Clear for next benchmark iteration
            }
        };

        client->onDisconnect = [&serverSawDisconnect]() {
            serverSawDisconnect.store(true);
        };
    };

    REQUIRE(server->start(path, 0));

    // 3. Setup Client
    auto client = std::make_shared<UnixClient>(loop.loop);
    std::atomic<bool> clientConnected{false};
    std::atomic<bool> clientDisconnected{false};

    client->onConnect = [&clientConnected]() {
        clientConnected.store(true);
    };

    client->onDisconnect = [&clientDisconnected]() {
        clientDisconnected.store(true);
    };

    JobUrl url("unix://" + path);
    REQUIRE(client->connectToHost(url));

    while (!clientConnected.load())
        std::this_thread::yield();

    // 4. BENCHMARK
    // We keep the connection open to measure throughput/latency without handshake overhead
    BENCHMARK("Unix Socket Roundtrip (Kernel Latency)") {
        packetReceived.store(false);
        client->send(sendBuffer, totalSize);

        // Spin-wait for kernel to deliver data
        while(!packetReceived.load()) {
            std::this_thread::yield();
        }
        return packetReceived.load();
    };

    // 5. CLEANUP (The Critical Anti-Deadlock Dance)
    // We must disconnect client and wait for server to process it BEFORE stopping server.
    client->disconnect();

    int retries = 0;
    while (!serverSawDisconnect.load() && retries < 2000) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        retries++;
    }
    REQUIRE(serverSawDisconnect.load());

    // Now it is safe to stop, as m_clients list inside server should be clean
    server->stop();
    fs::remove(path);
}
