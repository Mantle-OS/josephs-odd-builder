#include <catch2/catch_test_macros.hpp>

#include "job_zstd_wire.h"
#include "transient_test_corruption.h"

#include <sstream>
#include <limits>
#include <string>

namespace job::zstd {
TEST_CASE("writeU8/readU8 round-trip a byte value", "[job_zstd][wire][usage]")
{
    std::ostringstream out;
    job::zstd::utils::writeU8(out, 0xAB);

    std::istringstream in(out.str());
    std::uint8_t value = 0;
    REQUIRE(job::zstd::utils::readU8(in, value));
    REQUIRE(value == 0xAB);
}

TEST_CASE("writeU64/readU64 round-trip a 64-bit value", "[job_zstd][wire][usage]")
{
    std::ostringstream out;
    job::zstd::utils::writeU64(out, 0x0102030405060708ULL);

    std::istringstream in(out.str());
    std::uint64_t value = 0;
    REQUIRE(job::zstd::utils::readU64(in, value));
    REQUIRE(value == 0x0102030405060708ULL);
}

TEST_CASE("writeU64 encodes little-endian regardless of host byte order", "[job_zstd][wire][usage]")
{
    // This isn't testing readU64 at all.
    // It's testing that the on-disk -> byte layout is a deliberate, fixed encoding rather than "whatever memcpy-ing the integer happens to produce on this machine."
    // An archive written on a big-endian box has to be readable on a little-endian one, and vice versa.
    std::ostringstream out;
    job::zstd::utils::writeU64(out, 0x0102030405060708ULL);

    std::string const bytes = out.str();
    REQUIRE(bytes.size() == 8);

    std::string const expected = {
        static_cast<char>(0x08), static_cast<char>(0x07), static_cast<char>(0x06), static_cast<char>(0x05),
        static_cast<char>(0x04), static_cast<char>(0x03), static_cast<char>(0x02), static_cast<char>(0x01)
    };

    REQUIRE(bytes == expected);
}

TEST_CASE("writeString/readString round-trip an ordinary string", "[job_zstd][wire][usage]")
{
    std::ostringstream out;
    job::zstd::utils::writeString(out, "hello archive");

    std::istringstream in(out.str());
    std::string value;
    REQUIRE(job::zstd::utils::readString(in, value));
    REQUIRE(value == "hello archive");
}

TEST_CASE("writeString/readString round-trip an empty string", "[job_zstd][wire][usage]")
{
    std::ostringstream out;
    job::zstd::utils::writeString(out, "");

    std::istringstream in(out.str());
    std::string value = "not empty yet";
    REQUIRE(job::zstd::utils::readString(in, value));
    REQUIRE(value.empty());
}

TEST_CASE("writeString/readString round-trip a string containing embedded nul bytes", "[job_zstd][wire][usage]")
{
    // Length-prefixed, not null-terminated,  a path or payload chunk with an embedded \0 has to survive intact, unlike C-string-style framing.
    std::string const original("before\0after", 12);

    std::ostringstream out;
    job::zstd::utils::writeString(out, original);

    std::istringstream in(out.str());
    std::string value;
    REQUIRE(job::zstd::utils::readString(in, value));
    REQUIRE(value.size() == 12);
    REQUIRE(value == original);
}

TEST_CASE("Multiple wire values can be written and read back in sequence", "[job_zstd][wire][usage]")
{
    // This is the actual shape every entry header in the archive format, Takes: tag, then path, then a size, so proving the three primitives
    // compose cleanly in sequence matters more than any one of them alone.
    std::ostringstream out;
    job::zstd::utils::writeString(out, "JOBZCRYPFILE");
    job::zstd::utils::writeString(out, "some/relative/path.txt");
    job::zstd::utils::writeU64(out, 4096);
    job::zstd::utils::writeU8(out, 1);

    std::istringstream in(out.str());

    std::string tag;
    std::string path;
    std::uint64_t size = 0;
    std::uint8_t flag = 0;

    REQUIRE(job::zstd::utils::readString(in, tag));
    REQUIRE(job::zstd::utils::readString(in, path));
    REQUIRE(job::zstd::utils::readU64(in, size));
    REQUIRE(job::zstd::utils::readU8(in, flag));

    REQUIRE(tag == "JOBZCRYPFILE");
    REQUIRE(path == "some/relative/path.txt");
    REQUIRE(size == 4096);
    REQUIRE(flag == 1);
}

// 2
TEST_CASE("readU8 fails cleanly on an empty stream", "[job_zstd][wire][edge]")
{
    std::istringstream in("");
    std::uint8_t value = 0;
    REQUIRE_FALSE(job::zstd::utils::readU8(in, value));
}

TEST_CASE("readU64 fails cleanly when fewer than 8 bytes are available", "[job_zstd][wire][edge]")
{
    std::istringstream in(std::string(5, '\x00')); // Only 5 of the 8 bytes readU64 needs.
    std::uint64_t value = 0;
    REQUIRE_FALSE(job::zstd::utils::readU64(in, value));
}

TEST_CASE("writeU64/readU64 round-trip boundary values", "[job_zstd][wire][edge]")
{
    for (std::uint64_t const original : std::array<std::uint64_t, 3>{
             0,
             1,
             std::numeric_limits<std::uint64_t>::max()
         })
    {        std::ostringstream out;
        job::zstd::utils::writeU64(out, original);

        std::istringstream in(out.str());
        std::uint64_t value = 0;

        REQUIRE(job::zstd::utils::readU64(in, value));
        REQUIRE(value == original);
    }
}

TEST_CASE("writeString encodes length as little-endian uint32", "[job_zstd][wire][edge]")
{
    std::ostringstream out;
    job::zstd::utils::writeString(out, "abcd");

    std::string const bytes = out.str();
    REQUIRE(bytes.size() == 8);

    REQUIRE(static_cast<unsigned char>(bytes[0]) == 0x04);
    REQUIRE(static_cast<unsigned char>(bytes[1]) == 0x00);
    REQUIRE(static_cast<unsigned char>(bytes[2]) == 0x00);
    REQUIRE(static_cast<unsigned char>(bytes[3]) == 0x00);
}

TEST_CASE("readString fails cleanly when the length prefix itself is truncated", "[job_zstd][wire][edge]")
{
    std::istringstream in(std::string(2, '\x00')); // Needs 4 bytes just for the length prefix.
    std::string value;
    REQUIRE_FALSE(job::zstd::utils::readString(in, value));
}

TEST_CASE("readString fails cleanly when the payload is shorter than its advertised length", "[job_zstd][wire][edge]")
{
    // A truthful length prefix (10), but the stream runs dry after 3 bytes ... the classic truncated-archive shape, not a hostile one.
    std::string const wire = job::zstd::test::wireStringWithLie(10, "abc");

    std::istringstream in(wire);
    std::string value;
    REQUIRE_FALSE(job::zstd::utils::readString(in, value));
}

TEST_CASE("readString refuses a length prefix beyond kMaxWireStringLength", "[job_zstd][wire][edge][security]")
{
    // The actual bug this guard exists for: a corrupt or hostile length prefix claiming billions of bytes, which would otherwise sail straight
    // into std::string::resize() before anyone gets a chance to say no.
    std::uint32_t const hostileLength = static_cast<std::uint32_t>(job::zstd::utils::kMaxWireStringLength) + 1;
    std::string const wire = job::zstd::test::wireStringWithLie(hostileLength, "irrelevant");

    std::istringstream in(wire);
    std::string value;
    REQUIRE_FALSE(job::zstd::utils::readString(in, value));
}

TEST_CASE("readString refuses an extreme length prefix without attempting to allocate it", "[job_zstd][wire][edge][security]")
{
    // Deliberately near uint32_t's ceiling. if the guard were somehow, bypassed or computed wrong, this is the input that would actually
    // crash the test process via a multi-gigabyte allocation attempt rather than failing an assertion. Its presence here is the point: surviving
    // this test at all (regardless of pass/fail) proves the guard runs before resize(), not after.
    std::uint32_t const enormousLength = std::numeric_limits<std::uint32_t>::max() - 1;
    std::string const wire = job::zstd::test::wireStringWithLie(enormousLength, "tiny");

    std::istringstream in(wire);
    std::string value;
    REQUIRE_FALSE(job::zstd::utils::readString(in, value));
}

TEST_CASE("readString accepts a length prefix exactly at kMaxWireStringLength", "[job_zstd][wire][edge]")
{
    // The boundary itself is legitimate -- this guard should refuse anything OVER the limit, not silently redefine the limit one lower.
    std::string const payload(job::zstd::utils::kMaxWireStringLength, 'x');

    std::ostringstream out;
    job::zstd::utils::writeString(out, payload);

    std::istringstream in(out.str());
    std::string value;
    REQUIRE(job::zstd::utils::readString(in, value));
    REQUIRE(value.size() == job::zstd::utils::kMaxWireStringLength);
}

TEST_CASE("readU8/readU64/readString leave the stream positioned immediately after their own data", "[job_zstd][wire][edge]")
{
    // If any of these over- or under-consume, the "multiple values in sequence" usage case above would desync silently instead of failing
    // loudly worth pinning down explicitly rather than trusting it works because the happy-path sequence test passed.
    std::ostringstream out;
    job::zstd::utils::writeU8(out, 0x01);
    job::zstd::utils::writeU8(out, 0x02);

    std::istringstream in(out.str());
    std::uint8_t first = 0;
    std::uint8_t second = 0;

    REQUIRE(job::zstd::utils::readU8(in, first));
    REQUIRE(job::zstd::utils::readU8(in, second));

    REQUIRE(first == 0x01);
    REQUIRE(second == 0x02);
}

} // namespace job::zstd