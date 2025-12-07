#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <cstring>
#include <thread>
#include <vector>
#include <span>

#include <job_shared_memory.h>

#include <aligned_allocator.h>
#include <genome.h>
using namespace job::io;
using namespace job::ai::comp;
using namespace job::ai::evo;
using namespace job::ai::layers;

class GenomeSerializer {
public:
    GenomeSerializer(char *buffer, size_t cap):
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
        if (m_offset + len > m_cap)
            return false;

        std::memcpy(m_ptr + m_offset, src, len);
        m_offset += len;
        return true;
    }

    char *m_ptr;
    size_t m_cap;
    size_t m_offset;
};
// copy paste time
class GenomeDeserializer {
public:
    GenomeDeserializer(const char *buffer, size_t size):
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

    bool readBytes(void *dst, size_t len)
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


#ifdef JOB_TEST_BENCHMARKS
// could do this with the class itsself .,... whatever
static void cleanup_shm(const std::string &key)
{
    ::shm_unlink(key.c_str());
    std::string sem = key + "_sig";
    ::sem_unlink(sem.c_str());
}
TEST_CASE("AI Genome IPC: Zero-Copy Serialization", "[ai][ipc][genome]")
{
    const std::string key = "/job_ai_genome_test";
    cleanup_shm(key);

    JobSharedMemory writer;
    writer.setKey(key);
    writer.setSize(1024 * 1024); // 1MB Ring
    writer.setMode(SharedMemoryMode::Write);
    REQUIRE(writer.openDevice());

    JobSharedMemory reader;
    reader.setKey(key);
    reader.setMode(SharedMemoryMode::Read);
    REQUIRE(reader.openDevice());

    // "Realistic Genome"
    Genome original;
    original.header.uuid = 0xDEADBEEF;
    original.header.generation = 42;
    original.header.fitness = 0.99f;

    // Dense (Input 10 -> Output 20)
    LayerGene l1{};
    l1.type = LayerType::Dense;
    l1.activation = ActivationType::ReLU;
    l1.inputs = 10;
    l1.outputs = 20;
    l1.weight_count = 10 * 20; // 200 weights
    l1.bias_count = 20;        // 20 biases
    original.architecture.push_back(l1);

    // Add Layer: SparseMoE (Input 20 -> Output 10, 4 Experts)
    LayerGene l2{};
    l2.type = LayerType::SparseMoE;
    l2.activation = ActivationType::GELU;
    l2.inputs = 20;
    l2.outputs = 10;
    l2.weight_count = 20 * 10 * 4; // 800 weights
    l2.bias_count = 10;
    l2.auxiliary_data = 4; // 4 Experts
    original.architecture.push_back(l2);

    // Fill Weights with deterministic garbage
    size_t totalFloats = 200 + 20 + 800 + 10;
    original.weights.resize(totalFloats);
    for(size_t i=0; i<totalFloats; ++i)
        original.weights[i] = static_cast<job::core::real_t>(i) * 0.1f;

    // Serialize -> Ring Buffer
    // write into a scratch buffer first to mimic packet assembly
    char packetBuf[8192];
    GenomeSerializer ser(packetBuf, sizeof(packetBuf));
    REQUIRE(ser.write(original));

    ssize_t sent = writer.write(ser.bytesWritten() > 0 ?
                                    packetBuf :
                                    nullptr, ser.bytesWritten());
    REQUIRE(sent == static_cast<ssize_t>(ser.bytesWritten()));

    // read -> Deserdsialize
    char recvBuf[8192];
    ssize_t recvd = reader.read(recvBuf, sizeof(recvBuf));
    REQUIRE(recvd == sent);

    Genome received;
    GenomeDeserializer des(recvBuf, static_cast<size_t>(recvd));
    REQUIRE(des.read(received));

    // do the damn tests
    REQUIRE(received.header.uuid == original.header.uuid);
    REQUIRE(received.architecture.size() == 2);
    REQUIRE(received.weights.size() == original.weights.size());

    // Check layer 1
    REQUIRE(received.architecture[0].type == LayerType::Dense);
    REQUIRE(received.architecture[0].inputs == 10);

    // Check Weights (Floating point exact match because memcpy)
    REQUIRE(received.weights[0] == 0.0f);
    REQUIRE(received.weights[totalFloats-1] == static_cast<job::core::real_t>(totalFloats-1) * 0.1f);

    // BENCHMARK: Throughput of Genome Transfer
    BENCHMARK("Genome Roundtrip (Serialize+Transfer+Deserialize)") {
        GenomeSerializer bSer(packetBuf, sizeof(packetBuf));
        bSer.write(original); // Assume success for speed
        const auto bytes = bSer.bytesWritten();

        ssize_t w = 0;
        while ((w = writer.write(packetBuf, bytes)) == 0) {
            std::this_thread::yield(); // Backpressure: Let reader run
        }

        char recvBuf[8192]; // Stack allocation (L1 Cache hot)
        ssize_t r = 0;
        while ((r = reader.read(recvBuf, sizeof(recvBuf))) <= 0) {
            std::this_thread::yield(); // Backpressure: Let writer run
        }
        Genome bG;
        GenomeDeserializer bDes(recvBuf, static_cast<size_t>(r));
        bDes.read(bG);

        return bG.header.uuid;
    };

    writer.closeDevice();
    reader.closeDevice();
    cleanup_shm(key);
}

#endif
