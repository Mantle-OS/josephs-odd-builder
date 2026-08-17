#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <ipc/token_shm.h>

using job::token::TokenId;
using job::token::TokenPacketType;
using job::token::TokenShm;
using job::token::TokenShmRole;

//
// Block 1: usage / examples
//

TEST_CASE("TokenShm streams token IDs and packet frames across producer and consumer", "[token][ipc][shm][usage]")
{
    const std::string shmKey = "/job_token_example_shm";

    TokenShm producer;
    TokenShm consumer;

    REQUIRE(producer.openProducer(shmKey, 64 * 1024));
    REQUIRE(consumer.openConsumer(shmKey));
    REQUIRE(producer.isConnected());
    REQUIRE(consumer.isConnected());
    REQUIRE(producer.role() == TokenShmRole::Producer);
    REQUIRE(consumer.role() == TokenShmRole::Consumer);
    REQUIRE(producer.key() == shmKey);
    REQUIRE(consumer.key() == shmKey);

    SECTION("Stream token ID batches")
    {
        const std::vector<TokenId> sentTokens = {1, 15043, 29892, 590, 1024, 2};

        REQUIRE(producer.writeTokens(sentTokens));

        const std::vector<TokenId> receivedTokens = consumer.readAvailableTokens();

        REQUIRE(receivedTokens.size() == sentTokens.size());
        REQUIRE(receivedTokens == sentTokens);
    }

    SECTION("Stream text and EOS packets")
    {
        static constexpr std::string_view Text = "Hello from producer process";

        REQUIRE(producer.writeText(Text));
        REQUIRE(producer.writeEos());

        const auto textPacket = consumer.readNextPacket();

        REQUIRE(textPacket.has_value());
        REQUIRE(textPacket->type == TokenPacketType::Text);

        const std::string text{reinterpret_cast<const char *>(textPacket->payload.data()), textPacket->payload.size()};

        REQUIRE(text == Text);

        const auto eosPacket = consumer.readNextPacket();

        REQUIRE(eosPacket.has_value());
        REQUIRE(eosPacket->type == TokenPacketType::Eos);
        REQUIRE(eosPacket->payload.empty());
    }

    SECTION("Stream reset and heartbeat control packets")
    {
        REQUIRE(producer.writeReset());
        REQUIRE(producer.writeHeartbeat());

        const auto resetPacket = consumer.readNextPacket();

        REQUIRE(resetPacket.has_value());
        REQUIRE(resetPacket->type == TokenPacketType::Reset);
        REQUIRE(resetPacket->payload.empty());

        const auto heartbeatPacket = consumer.readNextPacket();

        REQUIRE(heartbeatPacket.has_value());
        REQUIRE(heartbeatPacket->type == TokenPacketType::Heartbeat);
        REQUIRE(heartbeatPacket->payload.empty());
    }

    producer.close();
    consumer.close();

    REQUIRE_FALSE(producer.isConnected());
    REQUIRE_FALSE(consumer.isConnected());
}

TEST_CASE("TokenShm reads token packets into caller supplied storage", "[token][ipc][shm][usage][tokens]")
{
    const std::string shmKey = "/job_token_read_tokens_shm";

    TokenShm producer;
    TokenShm consumer;

    REQUIRE(producer.openProducer(shmKey, 16 * 1024));
    REQUIRE(consumer.openConsumer(shmKey));

    const std::vector<TokenId> sent = {7, 42, 220, 9419, 20000};

    REQUIRE(producer.writeTokens(sent));

    std::vector<TokenId> received(sent.size());
    const ssize_t count = consumer.readTokens(received);

    REQUIRE(count == static_cast<ssize_t>(sent.size()));
    REQUIRE(received == sent);

    producer.close();
    consumer.close();
}

//
// Block 2: edge cases / failure behavior
//

TEST_CASE("TokenShm validates configuration and non-blocking polling", "[token][ipc][shm][edge]")
{
    TokenShm producer;
    TokenShm consumer;

    SECTION("Empty shared memory keys are rejected")
    {
        REQUIRE_FALSE(producer.openProducer("", 4096));
        REQUIRE_FALSE(consumer.openConsumer(""));
    }

    SECTION("Keys without the POSIX leading slash are rejected")
    {
        REQUIRE_FALSE(producer.openProducer("invalid_key_without_slash", 4096));
        REQUIRE_FALSE(consumer.openConsumer("invalid_key_without_slash"));
    }

    SECTION("Zero size producer ring buffer is rejected")
    {
        REQUIRE_FALSE(producer.openProducer("/job_token_zero_size", 0));
    }

    SECTION("Consumer cannot attach to missing shared memory")
    {
        REQUIRE_FALSE(consumer.openConsumer("/job_token_non_existent_shm_key"));
    }

    SECTION("Non-blocking read returns no packet when ring is empty")
    {
        const std::string shmKey = "/job_token_nonblock_shm";

        REQUIRE(producer.openProducer(shmKey, 16 * 1024));
        REQUIRE(consumer.openConsumer(shmKey));

        consumer.setNonBlocking(true);

        REQUIRE_FALSE(consumer.readNextPacket().has_value());
        REQUIRE(producer.writeToken(42));

        const auto packet = consumer.readNextPacket();

        REQUIRE(packet.has_value());
        REQUIRE(packet->type == TokenPacketType::Tokens);
        REQUIRE(packet->payload.size() == sizeof(TokenId));

        TokenId value = 0;
        std::memcpy(&value, packet->payload.data(), sizeof(TokenId));

        REQUIRE(value == 42);
        REQUIRE_FALSE(consumer.readNextPacket().has_value());

        producer.close();
        consumer.close();
    }

    SECTION("Writes fail while disconnected")
    {
        TokenShm disconnected;

        REQUIRE_FALSE(disconnected.writeToken(10));
        REQUIRE_FALSE(disconnected.writeText("test"));
        REQUIRE_FALSE(disconnected.writeEos());
        REQUIRE_FALSE(disconnected.writeReset());
        REQUIRE_FALSE(disconnected.writeHeartbeat());
    }

    SECTION("Empty token span succeeds without creating a packet")
    {
        const std::string shmKey = "/job_token_empty_span_shm";

        REQUIRE(producer.openProducer(shmKey, 16 * 1024));
        REQUIRE(consumer.openConsumer(shmKey));

        consumer.setNonBlocking(true);

        REQUIRE(producer.writeTokens(std::span<const TokenId>{}));
        REQUIRE_FALSE(consumer.readNextPacket().has_value());

        producer.close();
        consumer.close();
    }

    SECTION("Empty text succeeds without creating a packet")
    {
        const std::string shmKey = "/job_token_empty_text_shm";

        REQUIRE(producer.openProducer(shmKey, 16 * 1024));
        REQUIRE(consumer.openConsumer(shmKey));

        consumer.setNonBlocking(true);

        REQUIRE(producer.writeText(""));
        REQUIRE_FALSE(consumer.readNextPacket().has_value());

        producer.close();
        consumer.close();
    }
}

TEST_CASE("TokenShm enforces producer and consumer roles", "[token][ipc][shm][edge][role]")
{
    const std::string shmKey = "/job_token_role_shm";

    TokenShm producer;
    TokenShm consumer;

    REQUIRE(producer.openProducer(shmKey, 16 * 1024));
    REQUIRE(consumer.openConsumer(shmKey));

    consumer.setNonBlocking(true);

    REQUIRE_FALSE(consumer.writeToken(42));
    REQUIRE_FALSE(consumer.writeText("nope"));
    REQUIRE_FALSE(consumer.writeEos());
    REQUIRE_FALSE(producer.readNextPacket().has_value());

    std::vector<TokenId> output(16);

    REQUIRE(producer.readTokens(output) == -1);

    producer.close();
    consumer.close();
}

TEST_CASE("TokenShm rejects a second open while already connected", "[token][ipc][shm][edge][state]")
{
    const std::string shmKey = "/job_token_double_open_shm";

    TokenShm producer;

    REQUIRE(producer.openProducer(shmKey, 16 * 1024));
    REQUIRE_FALSE(producer.openProducer(shmKey, 16 * 1024));
    REQUIRE_FALSE(producer.openConsumer(shmKey));
    REQUIRE(producer.isConnected());

    producer.close();
}

TEST_CASE("TokenShm readTokens rejects insufficient caller storage", "[token][ipc][shm][edge][tokens]")
{
    const std::string shmKey = "/job_token_small_output_shm";

    TokenShm producer;
    TokenShm consumer;

    REQUIRE(producer.openProducer(shmKey, 16 * 1024));
    REQUIRE(consumer.openConsumer(shmKey));

    const std::vector<TokenId> sent = {1, 2, 3, 4};

    REQUIRE(producer.writeTokens(sent));

    std::vector<TokenId> output(2);

    REQUIRE(consumer.readTokens(output) == -1);

    producer.close();
    consumer.close();
}

//
// Block 3: benchmarks
//

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Benchmark TokenShm throughput and IPC latency", "[token][ipc][shm][benchmark]")
{
    const std::string shmKey = "/job_token_bench_shm";

    TokenShm producer;
    TokenShm consumer;

    REQUIRE(producer.openProducer(shmKey, 512 * 1024));
    REQUIRE(consumer.openConsumer(shmKey));

    consumer.setNonBlocking(true);

    BENCHMARK("Single token write and read IPC roundtrip")
    {
        REQUIRE(producer.writeToken(12345));
        return consumer.readNextPacket();
    };

    const std::vector<TokenId> batch(1000, 42);

    BENCHMARK("Batch streaming 1,000 tokens through ring buffer")
    {
        REQUIRE(producer.writeTokens(batch));
        return consumer.readAvailableTokens();
    };

    producer.close();
    consumer.close();
}

TEST_CASE("Benchmark TokenShm batch scaling", "[token][ipc][shm][benchmark][scaling]")
{
    const std::string shmKey = "/job_token_bench_scaling_shm";

    TokenShm producer;
    TokenShm consumer;

    REQUIRE(producer.openProducer(shmKey, 64 * 1024 * 1024));
    REQUIRE(consumer.openConsumer(shmKey));

    consumer.setNonBlocking(true);

    const std::vector<TokenId> batch1K(1'000, 42);
    const std::vector<TokenId> batch4K(4'096, 42);
    const std::vector<TokenId> batch16K(16'384, 42);
    const std::vector<TokenId> batch64K(65'536, 42);
    const std::vector<TokenId> batch256K(262'144, 42);
    const std::vector<TokenId> batch1M(1'048'576, 42);
    const std::vector<TokenId> batch4M(4'194'304, 42);

    std::vector<TokenId> rxBuffer(4'194'304);

    BENCHMARK("Batch 1K tokens")
    {
        (void)producer.writeTokens(batch1K);
        return consumer.readTokens(rxBuffer);
    };

    BENCHMARK("Batch 4K tokens")
    {
        (void)producer.writeTokens(batch4K);
        return consumer.readTokens(rxBuffer);
    };

    BENCHMARK("Batch 16K tokens") {
        (void)producer.writeTokens(batch16K);
        return consumer.readTokens(rxBuffer);
    };

    BENCHMARK("Batch 64K tokens") {
        (void)producer.writeTokens(batch64K);
        return consumer.readTokens(rxBuffer);
    };

    BENCHMARK("Batch 256K tokens") {
        (void)producer.writeTokens(batch256K);
        return consumer.readTokens(rxBuffer);
    };

    BENCHMARK("Batch 1M tokens"){
        (void)producer.writeTokens(batch1M);
        return consumer.readTokens(rxBuffer);
    };

    BENCHMARK("Batch 4M tokens"){
        (void)producer.writeTokens(batch4M);
        return consumer.readTokens(rxBuffer);
    };

    producer.close();
    consumer.close();
}

#endif