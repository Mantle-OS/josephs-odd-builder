#include <catch2/catch_test_macros.hpp>
#ifdef JOB_TEST_BENCHMARKS
    #include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <sstream>
#include <istream>
#include <ostream>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>

#include <split_mix64.h>

#include <job_zstd_io.h>

#include "transient_test_corruption.h"

namespace job::zstd {

namespace {

[[nodiscard]] std::string compressToString(const std::string &plaintext, int level = 3)
{
    std::stringbuf transport(std::ios::out);
    JobZstdIO zstd(&transport);
    REQUIRE(zstd.setCompressionLevel(level));

    REQUIRE(zstd.open(JobZstdIO::Mode::WriteOnly));
    std::ostream out(&zstd);
    out.write(plaintext.data(), static_cast<std::streamsize>(plaintext.size()));
    REQUIRE(zstd.close());

    return transport.str();
}

[[nodiscard]] std::string decompressFromString(const std::string &compressed)
{
    std::stringbuf transport(compressed, std::ios::in);
    JobZstdIO zstd(&transport);

    REQUIRE(zstd.open(JobZstdIO::Mode::ReadOnly));
    std::istream in(&zstd);

    std::ostringstream result;
    result << in.rdbuf();
    static_cast<void>(zstd.close());

    return result.str();
}

// A transport that fails every sputn() call once a byte budget is
// exhausted  std::stringbuf never short-writes on its own, so proving
// hadEncodeError() actually catches a short write needs a transport that
// can be made to lie about how much it accepted.
class ShortWritingStreamBuf : public std::streambuf
{
public:
    explicit ShortWritingStreamBuf(std::size_t byteBudget) : m_remaining(byteBudget)
    {
    }

    [[nodiscard]] const std::string &written() const noexcept
    {
        return m_written;
    }

protected:
    std::streamsize xsputn(const char *s, std::streamsize count) override
    {
        std::streamsize const allowed = std::min<std::streamsize>(count, static_cast<std::streamsize>(m_remaining));

        m_written.append(s, static_cast<std::size_t>(allowed));
        m_remaining -= static_cast<std::size_t>(allowed);

        return allowed; // Silently accepts less than requested -- the short write itself.
    }

    int sync() override
    {
        return 0;
    }

private:
    std::size_t m_remaining;
    std::string m_written;
};

// Deterministic filler, one RNG for every test that needs "some bytes that
// won't compress well"  correctness tests and benchmarks alike. Not
// cryptographic (SplitMix64 says so itself), which is exactly right here:
// nothing in this file needs unpredictability, and reproducibility means a
// benchmark regression can be chased down with the same seed later instead
// of chasing a moving target.
void fillDeterministicBytes(std::string &buffer, std::uint64_t seed)
{
    job::core::SplitMix64 rng(seed);
    std::size_t offset = 0;

    while (offset + sizeof(std::uint64_t) <= buffer.size()) {
        std::uint64_t const value = rng.next();
        std::memcpy(buffer.data() + offset, &value, sizeof(value));
        offset += sizeof(value);
    }

    if (offset < buffer.size()) {
        std::uint64_t const value = rng.next();
        std::memcpy(buffer.data() + offset, &value, buffer.size() - offset);
    }
}

} // namespace


TEST_CASE("JobZstdIO round-trips plain text through compress and decompress", "[job_zstd][io][roundtrip]")
{
    std::string const original = "HuggingFace Token: hf_ABC123XYZ789SecureManifestTokenData";

    std::string const compressed = compressToString(original);
    std::string const restored   = decompressFromString(compressed);

    REQUIRE(restored == original);
    REQUIRE(compressed.size() > 0);
}

TEST_CASE("JobZstdIO handles bulk reads via istream::read, not just operator>>", "[job_zstd][io][roundtrip][xsgetn]")
{
    std::string original(1 << 20, '\0'); // 1 MiB
    fillDeterministicBytes(original, 0x1337);

    std::string const compressed = compressToString(original, 5);

    std::stringbuf transport(compressed, std::ios::in);
    JobZstdIO zstd(&transport);
    REQUIRE(zstd.open(JobZstdIO::Mode::ReadOnly));

    std::string restored(original.size(), '\0');
    std::istream in(&zstd);
    in.read(restored.data(), static_cast<std::streamsize>(restored.size()));

    REQUIRE(static_cast<std::size_t>(in.gcount()) == original.size());
    REQUIRE(restored == original);
    REQUIRE(zstd.atEnd());
    REQUIRE_FALSE(zstd.wasTruncated());
    REQUIRE_FALSE(zstd.hadDecodeError());
}

TEST_CASE("JobZstdIO compression level affects output size but not correctness", "[job_zstd][io][compressionlevel]")
{
    std::string const original(100000, 'A');

    std::string const low  = compressToString(original, 1);
    std::string const high = compressToString(original, 19);

    REQUIRE(decompressFromString(low)  == original);
    REQUIRE(decompressFromString(high) == original);
    REQUIRE(high.size() <= low.size());
}


// 2
TEST_CASE("JobZstdIO rejects a null transport", "[job_zstd][io][edge][nulltransport]")
{
    JobZstdIO zstd(nullptr);
    REQUIRE_FALSE(zstd.open(JobZstdIO::Mode::WriteOnly));
    REQUIRE_FALSE(zstd.errorString().empty());
}

TEST_CASE("JobZstdIO refuses to open twice", "[job_zstd][io][edge][doubleopen]")
{
    std::stringbuf transport(std::ios::out);
    JobZstdIO zstd(&transport);

    REQUIRE(zstd.open(JobZstdIO::Mode::WriteOnly));
    REQUIRE_FALSE(zstd.open(JobZstdIO::Mode::WriteOnly));
    REQUIRE(zstd.close());
}

TEST_CASE("JobZstdIO close is idempotent", "[job_zstd][io][edge][close]")
{
    std::stringbuf transport(std::ios::out);
    JobZstdIO zstd(&transport);

    REQUIRE(zstd.open(JobZstdIO::Mode::WriteOnly));
    REQUIRE(zstd.close());
    REQUIRE(zstd.close());
}

TEST_CASE("JobZstdIO round-trips a zero-length payload", "[job_zstd][io][edge][empty]")
{
    std::string const compressed = compressToString("");
    std::string const restored   = decompressFromString(compressed);

    REQUIRE(restored.empty());
}

TEST_CASE("JobZstdIO setCompressionLevel is ignored once open", "[job_zstd][io][edge][compressionlevel]")
{
    std::stringbuf transport(std::ios::out);
    JobZstdIO zstd(&transport);
    REQUIRE(zstd.setCompressionLevel(10));

    REQUIRE(zstd.open(JobZstdIO::Mode::WriteOnly));
    REQUIRE_FALSE(zstd.setCompressionLevel(1));
    REQUIRE(zstd.compressionLevel() == 10);
    static_cast<void>(zstd.close());
}

TEST_CASE("JobZstdIO out-of-range compression level falls back to the default", "[job_zstd][io][edge][compressionlevel]")
{
    std::stringbuf transport(std::ios::out);
    JobZstdIO zstd(&transport);

    int const belowMin = JobZstdOptions::minCompressionLevel() - 1;
    int const aboveMax = JobZstdOptions::maxCompressionLevel() + 1;

    REQUIRE(zstd.setCompressionLevel(aboveMax));
    REQUIRE(zstd.compressionLevel() == JobZstdOptions::kDefaultCompressionLevel);

    REQUIRE(zstd.setCompressionLevel(9));
    REQUIRE(zstd.setCompressionLevel(belowMin));
    REQUIRE(zstd.compressionLevel() == JobZstdOptions::kDefaultCompressionLevel);
}

TEST_CASE("JobZstdIO reports truncation, not a decode error, when the stream simply runs out early", "[job_zstd][io][edge][truncation]")
{
    std::string const original(50000, 'x');
    std::string const compressed = compressToString(original, 3);

    std::string const chopped = job::zstd::test::truncate(compressed, 0.5);

    std::stringbuf transport(chopped, std::ios::in);
    JobZstdIO zstd(&transport);
    REQUIRE(zstd.open(JobZstdIO::Mode::ReadOnly));

    std::string buffer(original.size(), '\0');
    std::istream in(&zstd);
    in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    REQUIRE(zstd.wasTruncated());
    REQUIRE_FALSE(zstd.hadDecodeError());
    REQUIRE_FALSE(zstd.errorString().empty());
    REQUIRE_FALSE(zstd.close());
}

TEST_CASE("JobZstdIO reports a decode error, not truncation, when the frame is corrupted mid-stream", "[job_zstd][io][edge][decodeerror]")
{
    std::string const original(50000, 'x');
    std::string const compressed = compressToString(original, 3);
    std::string const corrupted  = job::zstd::test::flipBit(compressed, compressed.size() / 2);

    std::stringbuf transport(corrupted, std::ios::in);
    JobZstdIO zstd(&transport);
    REQUIRE(zstd.open(JobZstdIO::Mode::ReadOnly));

    std::string buffer(original.size(), '\0');
    std::istream in(&zstd);
    in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    REQUIRE(zstd.hadDecodeError());
    REQUIRE_FALSE(zstd.wasTruncated());
    REQUIRE_FALSE(zstd.errorString().empty());
    REQUIRE_FALSE(zstd.close());
}

TEST_CASE("JobZstdIO atEnd is false mid-stream and true after a clean finish", "[job_zstd][io][edge][atend]")
{
    std::string const original(200, 'z');
    std::string const compressed = compressToString(original);

    std::stringbuf transport(compressed, std::ios::in);
    JobZstdIO zstd(&transport);
    REQUIRE(zstd.open(JobZstdIO::Mode::ReadOnly));

    REQUIRE_FALSE(zstd.atEnd());

    std::string buffer(original.size(), '\0');
    std::istream in(&zstd);
    in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    REQUIRE(zstd.atEnd());
    static_cast<void>(zstd.close());

    REQUIRE(zstd.atEnd());
}

TEST_CASE("JobZstdIO atEnd is false, not true, on a stream left in a decode-error state", "[job_zstd][io][edge][atend][decodeerror]")
{
    std::string const original(50000, 'x');
    std::string const compressed = compressToString(original, 3);
    std::string const corrupted  = job::zstd::test::flipBit(compressed, compressed.size() / 2);

    std::stringbuf transport(corrupted, std::ios::in);
    JobZstdIO zstd(&transport);
    REQUIRE(zstd.open(JobZstdIO::Mode::ReadOnly));

    std::string buffer(original.size(), '\0');
    std::istream in(&zstd);
    in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    REQUIRE(zstd.hadDecodeError());
    REQUIRE_FALSE(zstd.atEnd());
}

TEST_CASE("JobZstdIO sync on a null transport reports failure, not success", "[job_zstd][io][edge][sync]")
{
    JobZstdIO zstd(nullptr);
    std::ostream out(&zstd);
    out.flush();
    REQUIRE(out.fail());
}

TEST_CASE("JobZstdIO close reports failure when the transport silently accepted a short write", "[job_zstd][io][edge][encodeerror]")
{
    ShortWritingStreamBuf transport(16);
    JobZstdIO zstd(&transport);

    REQUIRE(zstd.open(JobZstdIO::Mode::WriteOnly));

    std::string const payload(50000, 'y');
    std::ostream out(&zstd);
    out.write(payload.data(), static_cast<std::streamsize>(payload.size()));

    REQUIRE_FALSE(zstd.close());
    REQUIRE(zstd.hadEncodeError());
    REQUIRE_FALSE(zstd.errorString().empty());
}


// 3
#ifdef JOB_TEST_BENCHMARKS


TEST_CASE("JobZstdIO throughput: compress and decompress 8 MiB", "[job_zstd][io][benchmark]")
{
    std::string payload(8 * 1024 * 1024, '\0');
    fillDeterministicBytes(payload, 0xC0FFEE);

    BENCHMARK("compress 8MiB (level 1, fast)")
    {
        return compressToString(payload, 1);
    };

    BENCHMARK("compress 8MiB (level 3, default)")
    {
        return compressToString(payload, 3);
    };

    // BENCHMARK("compress 8MiB (level 19, max-ish)")  // TO LONG  uncomment if you want. takes way to long
    // {
    //     return compressToString(payload, 19);
    // };

    std::string const compressedLevel3 = compressToString(payload, 3);

    BENCHMARK("decompress 8MiB (from level 3)")
    {
        return decompressFromString(compressedLevel3);
    };
}

TEST_CASE("JobZstdIO throughput: highly compressible vs incompressible payloads", "[job_zstd][io][benchmark]")
{
    std::string const repetitive(4 * 1024 * 1024, 'A');

    std::string incompressible(4 * 1024 * 1024, '\0');
    fillDeterministicBytes(incompressible, 0xDEADBEEF);

    BENCHMARK("compress 4MiB repetitive (level 3)")
    {
        return compressToString(repetitive, 3);
    };

    BENCHMARK("compress 4MiB random (level 3)")
    {
        return compressToString(incompressible, 3);
    };
}

TEST_CASE("JobZstdIO read path: bulk xsgetn versus single-character underflow", "[job_zstd][io][benchmark][xsgetn]")
{
    std::string payload(2 * 1024 * 1024, '\0');
    fillDeterministicBytes(payload, 0xFEEDFACE);

    std::string const compressed = compressToString(payload, 3);

    BENCHMARK("decompress 2MiB via bulk read (xsgetn)")
    {
        std::stringbuf transport(compressed, std::ios::in);
        JobZstdIO zstd(&transport);
        REQUIRE(zstd.open(JobZstdIO::Mode::ReadOnly));

        std::string out(payload.size(), '\0');
        std::istream in(&zstd);
        in.read(out.data(), static_cast<std::streamsize>(out.size()));

        static_cast<void>(zstd.close());
        return out;
    };

    BENCHMARK("decompress 2MiB one character at a time (underflow)")
    {
        std::stringbuf transport(compressed, std::ios::in);
        JobZstdIO zstd(&transport);
        REQUIRE(zstd.open(JobZstdIO::Mode::ReadOnly));

        std::string out;
        out.reserve(payload.size());

        std::istream in(&zstd);
        char ch = '\0';
        while (in.get(ch))
            out.push_back(ch);

        static_cast<void>(zstd.close());
        return out;
    };
}

#endif

} // namespace job::zstd