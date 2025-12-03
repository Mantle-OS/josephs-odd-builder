#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <job_shared_memory.h>
#include <genome.h>
#include <aligned_allocator.h>

// We reuse the RawSerializer concept locally for this test
// to avoid depending on the full 'job_serializer' library stack for now.
#include <cstring>
#include <vector>
#include <span>

using namespace job::io;
using namespace job::ai::base;

// -----------------------------------------------------------------------------
// Helper: Raw Serialization for Genome
// This mimics what your code-generator would produce for a Genome.
// -----------------------------------------------------------------------------
class GenomeSerializer {
public:
    GenomeSerializer(char* buffer, size_t cap)
        : m_ptr(buffer), m_cap(cap), m_offset(0) {}

    bool write(const Genome& g) {
        // 1. Header
        if (!writePOD(g.header)) return false;

        // 2. Architecture (LayerGenes)
        uint32_t archCount = static_cast<uint32_t>(g.architecture.size());
        if (!writePOD(archCount)) return false;

        size_t archBytes = archCount * sizeof(LayerGene);
        if (!writeBytes(g.architecture.data(), archBytes)) return false;

        // 3. Weights (float vector)
        uint32_t weightCount = static_cast<uint32_t>(g.weights.size());
        if (!writePOD(weightCount)) return false;

        size_t weightBytes = weightCount * sizeof(float);
        if (!writeBytes(g.weights.data(), weightBytes)) return false;

        return true;
    }

    size_t bytesWritten() const { return m_offset; }

private:
    template<typename T>
    bool writePOD(const T& val) {
        if (m_offset + sizeof(T) > m_cap) return false;
        std::memcpy(m_ptr + m_offset, &val, sizeof(T));
        m_offset += sizeof(T);
        return true;
    }

    bool writeBytes(const void* src, size_t len) {
        if (m_offset + len > m_cap) return false;
        std::memcpy(m_ptr + m_offset, src, len);
        m_offset += len;
        return true;
    }

    char* m_ptr;
    size_t m_cap;
    size_t m_offset;
};

class GenomeDeserializer {
public:
    GenomeDeserializer(const char* buffer, size_t size)
        : m_ptr(buffer), m_size(size), m_offset(0) {}

    bool read(Genome& outG) {
        // 1. Header
        if (!readPOD(outG.header)) return false;

        // 2. Architecture
        uint32_t archCount = 0;
        if (!readPOD(archCount)) return false;

        outG.architecture.resize(archCount);
        size_t archBytes = archCount * sizeof(LayerGene);
        if (!readBytes(outG.architecture.data(), archBytes)) return false;

        // 3. Weights
        uint32_t weightCount = 0;
        if (!readPOD(weightCount)) return false;

        outG.weights.resize(weightCount);
        size_t weightBytes = weightCount * sizeof(float);
        if (!readBytes(outG.weights.data(), weightBytes)) return false;

        return true;
    }

private:
    template<typename T>
    bool readPOD(T& outVal) {
        if (m_offset + sizeof(T) > m_size) return false;
        std::memcpy(&outVal, m_ptr + m_offset, sizeof(T));
        m_offset += sizeof(T);
        return true;
    }

    bool readBytes(void* dst, size_t len) {
        if (m_offset + len > m_size) return false;
        std::memcpy(dst, m_ptr + m_offset, len);
        m_offset += len;
        return true;
    }

    const char* m_ptr;
    size_t m_size;
    size_t m_offset;
};

// -----------------------------------------------------------------------------
// TEST CASE
// -----------------------------------------------------------------------------

static void cleanup_shm(const std::string& key) {
    ::shm_unlink(key.c_str());
    std::string sem = key + "_sig";
    ::sem_unlink(sem.c_str());
}

TEST_CASE("AI Genome IPC: Zero-Copy Serialization", "[ai][ipc][genome]") {
    const std::string key = "/job_ai_genome_test";
    cleanup_shm(key);

    // 1. Setup Ring Buffer (Large enough for a big neural net)
    JobSharedMemory writer;
    writer.setKey(key);
    writer.setSize(1024 * 1024); // 1MB Ring
    writer.setMode(SharedMemoryMode::Write);
    REQUIRE(writer.openDevice());

    JobSharedMemory reader;
    reader.setKey(key);
    reader.setMode(SharedMemoryMode::Read);
    REQUIRE(reader.openDevice());

    // 2. Construct a Realistic Genome
    Genome original;
    original.header.uuid = 0xDEADBEEF;
    original.header.generation = 42;
    original.header.fitness = 0.99f;

    // Add Layer: Dense (Input 10 -> Output 20)
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
    for(size_t i=0; i<totalFloats; ++i) {
        original.weights[i] = static_cast<float>(i) * 0.1f;
    }

    // 3. Serialize -> Ring Buffer
    // We write into a scratch buffer first to mimic packet assembly
    char packetBuf[8192];
    GenomeSerializer ser(packetBuf, sizeof(packetBuf));
    REQUIRE(ser.write(original));

    ssize_t sent = writer.write(ser.bytesWritten() > 0 ? packetBuf : nullptr, ser.bytesWritten());
    REQUIRE(sent == static_cast<ssize_t>(ser.bytesWritten()));

    // 4. Read -> Deserialize
    char recvBuf[8192];
    ssize_t recvd = reader.read(recvBuf, sizeof(recvBuf));
    REQUIRE(recvd == sent);

    Genome received;
    GenomeDeserializer des(recvBuf, static_cast<size_t>(recvd));
    REQUIRE(des.read(received));

    // 5. Verify Integrity
    REQUIRE(received.header.uuid == original.header.uuid);
    REQUIRE(received.architecture.size() == 2);
    REQUIRE(received.weights.size() == original.weights.size());

    // Check Layer 1
    REQUIRE(received.architecture[0].type == LayerType::Dense);
    REQUIRE(received.architecture[0].inputs == 10);

    // Check Weights (Floating point exact match because memcpy)
    REQUIRE(received.weights[0] == 0.0f);
    REQUIRE(received.weights[totalFloats-1] == static_cast<float>(totalFloats-1) * 0.1f);

    // 6. BENCHMARK: Throughput of Genome Transfer
    BENCHMARK("Genome Roundtrip (Serialize+Transfer+Deserialize)") {
        GenomeSerializer bSer(packetBuf, sizeof(packetBuf));
        bSer.write(original);
        (void)writer.write(packetBuf, bSer.bytesWritten());
        (void)reader.read(recvBuf, sizeof(recvBuf));
        Genome bG;
        GenomeDeserializer bDes(recvBuf, bSer.bytesWritten());
        bDes.read(bG);
        return bG.header.uuid;
    };

    writer.closeDevice();
    reader.closeDevice();
    cleanup_shm(key);
}
