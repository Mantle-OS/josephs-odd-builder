#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "formats/tokenizer_shm.h"

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

namespace job::token::test {


// Block 1: Usage / Examples
TEST_CASE("TokenizerShm streams token IDs and packet frames across producer/consumer", "[token][formats][shm][example]")
{
    const std::string shmKey = "/job_token_example_shm";

    TokenizerShm producer;
    TokenizerShm consumer;

    REQUIRE(producer.openProducer(shmKey, 64 * 1024));
    REQUIRE(consumer.openConsumer(shmKey));

    CHECK(producer.isConnected());
    CHECK(consumer.isConnected());
    CHECK(producer.role() == TokenShmRole::Producer);
    CHECK(consumer.role() == TokenShmRole::Consumer);

    SECTION("Stream token ID batches") {
        std::vector<int32_t> sentTokens = {1, 15043, 29892, 590, 1024, 2};

        REQUIRE(producer.writeTokens(sentTokens));

        std::vector<int32_t> receivedTokens = consumer.readAvailableTokens();
        REQUIRE(receivedTokens.size() == sentTokens.size());
        CHECK(receivedTokens == sentTokens);
    }

    SECTION("Stream text chunks and EOS sentinel packets") {
        std::string_view chunk = "Hello from producer process";

        REQUIRE(producer.writeText(chunk));
        REQUIRE(producer.writeEos());

        // 1. Read text packet
        auto pktText = consumer.readNextPacket();
        REQUIRE(pktText.has_value());
        CHECK(pktText->type == TokenPacketType::Text);
        std::string textContent(reinterpret_cast<const char*>(pktText->payload.data()), pktText->payload.size());
        CHECK(textContent == chunk);

        // 2. Read EOS packet
        auto pktEos = consumer.readNextPacket();
        REQUIRE(pktEos.has_value());
        CHECK(pktEos->type == TokenPacketType::Eos);
        CHECK(pktEos->payload.empty());
    }

    producer.close();
    consumer.close();
}


// Block 2: Edge Cases
TEST_CASE("TokenizerShm enforces key formatting, non-blocking polling, and size bounds", "[token][formats][shm][edge_cases]")
{
    TokenizerShm producer;
    TokenizerShm consumer;

    SECTION("Keys must start with leading slash '/'") {
        CHECK_FALSE(producer.openProducer("invalid_key_without_slash", 4096));
        CHECK_FALSE(consumer.openConsumer("invalid_key_without_slash"));
    }

    SECTION("Zero size ring buffer fails validation") {
        CHECK_FALSE(producer.openProducer("/job_token_zero_size", 0));
    }

    SECTION("Consumer attaching to non-existent SHM returns false") {
        CHECK_FALSE(consumer.openConsumer("/job_token_non_existent_shm_key"));
    }

    SECTION("Non-blocking read returns nullopt when buffer is empty") {
        const std::string shmKey = "/job_token_nonblock_shm";
        REQUIRE(producer.openProducer(shmKey, 16 * 1024));
        REQUIRE(consumer.openConsumer(shmKey));

        consumer.setNonBlocking(true);

        auto pkt = consumer.readNextPacket();
        CHECK_FALSE(pkt.has_value());

        // Write single token and read back in non-blocking mode
        REQUIRE(producer.writeToken(42));
        auto validPkt = consumer.readNextPacket();
        REQUIRE(validPkt.has_value());
        CHECK(validPkt->type == TokenPacketType::Tokens);
        REQUIRE(validPkt->payload.size() == sizeof(int32_t));

        int32_t val = 0;
        std::memcpy(&val, validPkt->payload.data(), sizeof(int32_t));
        CHECK(val == 42);

        // Subsequent poll is empty again
        CHECK_FALSE(consumer.readNextPacket().has_value());

        producer.close();
        consumer.close();
    }

    SECTION("Writing when not connected returns false") {
        TokenizerShm closedDevice;
        CHECK_FALSE(closedDevice.writeToken(10));
        CHECK_FALSE(closedDevice.writeText("test"));
        CHECK_FALSE(closedDevice.writeEos());
    }

    SECTION("Writing empty tokens span returns true without writing header") {
        const std::string shmKey = "/job_token_empty_span_shm";
        REQUIRE(producer.openProducer(shmKey, 16 * 1024));
        REQUIRE(consumer.openConsumer(shmKey));
        consumer.setNonBlocking(true);

        CHECK(producer.writeTokens(std::span<const int32_t>{}));
        CHECK_FALSE(consumer.readNextPacket().has_value());

        producer.close();
        consumer.close();
    }
}

// ============================================================================
// Block 3: Benchmarks
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("Benchmark TokenizerShm throughput and IPC latency", "[token][formats][shm][benchmark]")
{
    const std::string shmKey = "/job_token_bench_shm";

    TokenizerShm producer;
    TokenizerShm consumer;

    REQUIRE(producer.openProducer(shmKey, 512 * 1024));
    REQUIRE(consumer.openConsumer(shmKey));
    consumer.setNonBlocking(true);

    BENCHMARK("Single token write + read IPC roundtrip") {
        REQUIRE(producer.writeToken(12345));
        return consumer.readNextPacket();
    };

    std::vector<int32_t> batch(1000, 42);

    BENCHMARK("Batch streaming 1,000 tokens through ring buffer") {
        REQUIRE(producer.writeTokens(batch));
        return consumer.readAvailableTokens();
    };

    producer.close();
    consumer.close();
}

TEST_CASE("Benchmark TokenizerShm batch scaling", "[token][formats][shm][benchmark][scaling]")
{
    const std::string shmKey = "/job_token_bench_scaling_shm";

    TokenizerShm producer;
    TokenizerShm consumer;

    REQUIRE(producer.openProducer(shmKey, 64 * 1024 * 1024));
    REQUIRE(consumer.openConsumer(shmKey));
    consumer.setNonBlocking(true);

    const std::vector<int32_t> batch1K(1'000, 42);         // ~4 KiB
    const std::vector<int32_t> batch4K(4'096, 42);         // ~16 KiB
    const std::vector<int32_t> batch16K(16'384, 42);       // ~64 KiB
    const std::vector<int32_t> batch64K(65'536, 42);       // ~256 KiB
    const std::vector<int32_t> batch256K(262'144, 42);     // ~1 MiB
    const std::vector<int32_t> batch1M(1'048'576, 42);     // ~4 MiB
    const std::vector<int32_t> batch4M(4'194'304, 42);     // ~16 MiB

    // Pre-allocated receive buffer for zero-alloc pure SHM throughput measurement
    std::vector<int32_t> rxBuffer(4'194'304);

    BENCHMARK("Batch 1K tokens (~4 KiB)") {
        (void)producer.writeTokens(batch1K);
        return consumer.readTokens(rxBuffer);
    };

    BENCHMARK("Batch 4K tokens (~16 KiB)") {
        (void)producer.writeTokens(batch4K);
        return consumer.readTokens(rxBuffer);
    };

    BENCHMARK("Batch 16K tokens (~64 KiB)") {
        (void)producer.writeTokens(batch16K);
        return consumer.readTokens(rxBuffer);
    };

    BENCHMARK("Batch 64K tokens (~256 KiB)") {
        (void)producer.writeTokens(batch64K);
        return consumer.readTokens(rxBuffer);
    };

    BENCHMARK("Batch 256K tokens (~1 MiB)") {
        (void)producer.writeTokens(batch256K);
        return consumer.readTokens(rxBuffer);
    };

    BENCHMARK("Batch 1M tokens (~4 MiB)") {
        (void)producer.writeTokens(batch1M);
        return consumer.readTokens(rxBuffer);
    };

    BENCHMARK("Batch 4M tokens (~16 MiB)") {
        (void)producer.writeTokens(batch4M);
        return consumer.readTokens(rxBuffer);
    };

    producer.close();
    consumer.close();
}

#endif




} // namespace job::token::test