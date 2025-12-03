#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <job_shared_memory.h>

#include <string>
#include <thread>
#include <vector>
#include <cstring>
#include <sys/mman.h>
#include <semaphore.h>
#include <atomic>

using namespace job::io;
using namespace std::chrono_literals;

static void shm_cleanup(const std::string &key)
{
    if (!key.empty()) {
        ::shm_unlink(key.c_str());
        std::string semName = key + "_sig";
        ::sem_unlink(semName.c_str());
    }
}

TEST_CASE("JobSharedMemory: Single Producer Single Consumer (SPSC) Usage",
          "[io][shared_memory][usage]")
{
    const std::string key = "/job_shm_usage";
    shm_cleanup(key);

    SECTION("Scenario: Basic string passing between two 'processes'")
    {
        // 1. Setup Writer (Process A)
        JobSharedMemory writer;
        writer.setKey(key);
        writer.setSize(1024);
        writer.setMode(SharedMemoryMode::Write);
        REQUIRE(writer.openDevice());

        // 2. Setup Reader (Process B)
        JobSharedMemory reader;
        reader.setKey(key);
        reader.setMode(SharedMemoryMode::Read);
        REQUIRE(reader.openDevice());

        // 3. Exchange Data
        const std::string msg = "Hello World";
        ssize_t written = writer.write(msg.data(), msg.size());
        REQUIRE(written == static_cast<ssize_t>(msg.size()));

        char buf[64] = {0};
        ssize_t read = reader.read(buf, sizeof(buf));
        REQUIRE(read == static_cast<ssize_t>(msg.size()));
        REQUIRE(std::string(buf, read) == msg);
    }
    // RAII: writer/reader destroyed here automatically

    SECTION("Scenario: Ring Buffer Wrap-Around (Streaming Data)")
    {
        // Clean slate for this section
        shm_cleanup(key);

        JobSharedMemory w;
        w.setKey(key);
        w.setSize(16); // Tiny buffer to force wrapping
        w.setMode(SharedMemoryMode::Write);
        REQUIRE(w.openDevice());

        JobSharedMemory r;
        r.setKey(key);
        r.setMode(SharedMemoryMode::Read);
        REQUIRE(r.openDevice());

        const std::string data1 = "ABCDEFGHIJ"; // 10 bytes
        const std::string data2 = "0123456789"; // 10 bytes

        // Fill head
        REQUIRE(w.write(data1.data(), data1.size()) == 10);

        // Consumer drains it to make room
        char buf[32];
        REQUIRE(r.read(buf, 10) == 10);
        REQUIRE(std::strncmp(buf, data1.c_str(), 10) == 0);

        // Write wraps around the end of the 16-byte buffer
        REQUIRE(w.write(data2.data(), data2.size()) == 10);

        // Read handles the wrapped data transparently
        REQUIRE(r.read(buf, 10) == 10);
        REQUIRE(std::strncmp(buf, data2.c_str(), 10) == 0);
    }
}

TEST_CASE("JobSharedMemory: Threaded Communication with Backpressure",
          "[io][shared_memory][usage][threaded]")
{
    const std::string key = "/job_shm_threaded";
    shm_cleanup(key);

    constexpr int kMessageCount = 500;
    const std::string payload = "packet";

    JobSharedMemory writer;
    writer.setKey(key);
    writer.setSize(128);
    writer.setMode(SharedMemoryMode::Write);
    writer.setNonBlocking(false);
    REQUIRE(writer.openDevice());

    JobSharedMemory reader;
    reader.setKey(key);
    reader.setMode(SharedMemoryMode::Read);
    REQUIRE(reader.openDevice());

    std::atomic<int> receivedCount{0};

    std::thread readerThread([&]() {
        char buf[64];
        std::string accum;
        while (receivedCount < kMessageCount) {
            ssize_t n = reader.read(buf, sizeof(buf));
            if (n > 0) {
                accum.append(buf, n);
                // Process stream
                while (accum.size() >= payload.size()) {
                    accum.erase(0, payload.size());
                    receivedCount++;
                }
            } else {
                std::this_thread::yield();
            }
        }
    });

    // Writer Loop
    for (int i = 0; i < kMessageCount; ++i) {
        ssize_t res = 0;
        // BACKPRESSURE PATTERN:
        // If write returns 0 (ENOSPC), we yield and retry.
        while ((res = writer.write(payload.data(), payload.size())) == 0) {
            std::this_thread::yield();
        }
        REQUIRE(res == static_cast<ssize_t>(payload.size()));
    }

    readerThread.join();
    REQUIRE(receivedCount == kMessageCount);

    shm_cleanup(key);
}


TEST_CASE("JobSharedMemory: Edge Cases and Error Handling",
          "[io][shared_memory][edge]")
{
    const std::string key = "/job_shm_edge";
    shm_cleanup(key);

    SECTION("Invalid Key Format")
    {
        JobSharedMemory shm;
        shm.setKey("bad_key_no_slash");
        shm.setSize(1024);
        shm.setMode(SharedMemoryMode::Write);

        REQUIRE_FALSE(shm.openDevice());
        REQUIRE(shm.getLastError() == SharedMemoryErrors::Key);
    }

    SECTION("Zero Size Creation")
    {
        JobSharedMemory shm;
        shm.setKey(key);
        shm.setSize(0); // Invalid
        shm.setMode(SharedMemoryMode::Write);

        REQUIRE_FALSE(shm.openDevice());
        REQUIRE(shm.getLastError() == SharedMemoryErrors::Size);
    }

    SECTION("Permissions: Reader trying to Write")
    {
        // create valid segment first
        {
            JobSharedMemory creator;
            creator.setKey(key);
            creator.setSize(256);
            creator.setMode(SharedMemoryMode::Write);
            REQUIRE(creator.openDevice());
        }

        JobSharedMemory reader;
        reader.setKey(key);
        reader.setMode(SharedMemoryMode::Read); // Read only
        REQUIRE(reader.openDevice());

        ssize_t res = reader.write("fail", 4);
        REQUIRE(res == -1);
        REQUIRE(reader.getLastError() == SharedMemoryErrors::Perm);
    }

    SECTION("Buffer Full (ENOSPC)")
    {
        JobSharedMemory shm;
        shm.setKey(key);
        shm.setSize(8); // Tiny
        shm.setMode(SharedMemoryMode::Write);
        REQUIRE(shm.openDevice());

        REQUIRE(shm.write("12345678", 8) == 8);

        // Next write should fail gracefully, not crash
        errno = 0;
        REQUIRE(shm.write("X", 1) == 0);
        REQUIRE(errno == ENOSPC);
        REQUIRE(shm.getLastError() == SharedMemoryErrors::OOR);
    }
}


TEST_CASE("JobSharedMemory: Benchmarks", "[io][shared_memory][bench]")
{
    const std::string key = "/job_shm_bench";
    shm_cleanup(key);

    JobSharedMemory writer;
    writer.setKey(key);
    writer.setSize(1024 * 64); // 64KB
    writer.setMode(SharedMemoryMode::Write);
    writer.setNonBlocking(false);
    REQUIRE(writer.openDevice());

    JobSharedMemory reader;
    reader.setKey(key);
    reader.setMode(SharedMemoryMode::Read);
    reader.setNonBlocking(false);
    REQUIRE(reader.openDevice());

    char data[1024];
    std::memset(data, 'A', sizeof(data));
    char readBuf[1024];

    BENCHMARK("Roundtrip 1KB Data (Write + Read)") {
        // We capture 'w' to satisfy [[nodiscard]]
        ssize_t w = writer.write(data, sizeof(data));
        // We cast to void to satisfy -Wunused-variable
        (void)w;

        return reader.read(readBuf, sizeof(readBuf));
    };

    shm_cleanup(key);
}

TEST_CASE("JobSharedMemory: Stress Test (High Volume)",
          "[io][shared_memory][stress]")
{
    // Push 100MB of data through a 4KB ring buffer
    const std::string key = "/job_shm_stress";
    shm_cleanup(key);

    constexpr size_t kTotalBytes = 100 * 1024 * 1024; // 100 MB
    constexpr size_t kChunkSize = 256;
    constexpr size_t kRingSize = 4096;

    JobSharedMemory w;
    w.setKey(key);
    w.setSize(kRingSize);
    w.setMode(SharedMemoryMode::Write);
    REQUIRE(w.openDevice());

    JobSharedMemory r;
    r.setKey(key);
    r.setMode(SharedMemoryMode::Read);
    REQUIRE(r.openDevice());

    std::thread producer([&]() {
        std::vector<char> chunk(kChunkSize, 'X');
        size_t written = 0;
        while(written < kTotalBytes) {
            if (w.write(chunk.data(), chunk.size()) > 0) {
                written += chunk.size();
            } else {
                std::this_thread::yield();
            }
        }
    });

    std::thread consumer([&]() {
        std::vector<char> buf(kChunkSize);
        size_t readTotal = 0;
        while(readTotal < kTotalBytes) {
            ssize_t n = r.read(buf.data(), buf.size());
            if (n > 0) {
                readTotal += n;
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    SUCCEED("Transferred 100MB successfully");

    shm_cleanup(key);
}
