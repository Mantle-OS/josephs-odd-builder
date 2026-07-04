#include <catch2/catch_test_macros.hpp>

#include "job_zstd_options.h"

namespace job::zstd {

namespace {
class TestOptions : public JobZstdOptions
{
public:
    using JobZstdOptions::notifyFinished; // promoted to public for the test
};
} // namespace


TEST_CASE("JobZstdOptions starts empty and idle", "[job_zstd][options][usage]")
{
    TestOptions const opts;

    REQUIRE(opts.input().empty());
    REQUIRE(opts.output().empty());
    REQUIRE(opts.errorString().empty());
    REQUIRE(opts.current() == 0);
    REQUIRE(opts.total() == 0);
}

TEST_CASE("JobZstdOptions stores input/output paths", "[job_zstd][options][usage]")
{
    TestOptions opts;

    REQUIRE(opts.setInput("/tmp/source.txt"));
    REQUIRE(opts.setOutput("/tmp/source.txt.zst"));

    REQUIRE(opts.input() == "/tmp/source.txt");
    REQUIRE(opts.output() == "/tmp/source.txt.zst");
}

TEST_CASE("JobZstdOptions tracks progress via current/total", "[job_zstd][options][usage]")
{
    TestOptions opts;

    REQUIRE(opts.setTotal(1000));
    REQUIRE(opts.setCurrent(250));

    REQUIRE(opts.total() == 1000);
    REQUIRE(opts.current() == 250);
}

TEST_CASE("JobZstdOptions accepts a compression level within the valid zstd range", "[job_zstd][options][usage][compressionlevel]")
{
    TestOptions opts;

    REQUIRE(opts.setCompressionLevel(9));
    REQUIRE(opts.compressionLevel() == 9);
}

TEST_CASE("JobZstdOptions fires the finished callback when notifyFinished is invoked", "[job_zstd][options][usage][callback]")
{
    TestOptions opts;
    bool fired = false;

    opts.setOnFinished([&fired]() {
        fired = true;
    });

    opts.notifyFinished();
    REQUIRE(fired);
}

TEST_CASE("JobZstdOptions structural flags default to true", "[job_zstd][options][usage]")
{
    TestOptions const opts;

    REQUIRE(opts.preserveEmptyDirectories());
    REQUIRE(opts.preserveSymlinks());
    REQUIRE(opts.recursiveDirectories());
}

TEST_CASE("JobZstdOptions structural flags can be individually toggled off", "[job_zstd][options][usage]")
{
    TestOptions opts;

    REQUIRE(opts.setPreserveEmptyDirectories(false));
    REQUIRE(opts.setPreserveSymlinks(false));
    REQUIRE(opts.setRecursiveDirectories(false));

    REQUIRE_FALSE(opts.preserveEmptyDirectories());
    REQUIRE_FALSE(opts.preserveSymlinks());
    REQUIRE_FALSE(opts.recursiveDirectories());
}

TEST_CASE("JobZstdOptions reports an error string once set", "[job_zstd][options][usage]")
{
    TestOptions opts;

    REQUIRE(opts.setErrorString("Something regrettable happened."));
    REQUIRE(opts.errorString() == "Something regrettable happened.");
}

// 2
TEST_CASE("JobZstdOptions setters report false when the value doesn't actually change", "[job_zstd][options][edge]")
{
    TestOptions opts;

    REQUIRE(opts.setInput("same"));
    REQUIRE_FALSE(opts.setInput("same"));

    REQUIRE(opts.setOutput("same"));
    REQUIRE_FALSE(opts.setOutput("same"));

    REQUIRE(opts.setCurrent(5));
    REQUIRE_FALSE(opts.setCurrent(5));

    REQUIRE(opts.setTotal(10));
    REQUIRE_FALSE(opts.setTotal(10));

    REQUIRE(opts.setErrorString("oops"));
    REQUIRE_FALSE(opts.setErrorString("oops"));

    REQUIRE(opts.setPreserveSymlinks(false));
    REQUIRE_FALSE(opts.setPreserveSymlinks(false));

    REQUIRE(opts.setPreserveEmptyDirectories(false));
    REQUIRE_FALSE(opts.setPreserveEmptyDirectories(false));

    REQUIRE(opts.setRecursiveDirectories(false));
    REQUIRE_FALSE(opts.setRecursiveDirectories(false));
}


TEST_CASE("JobZstdOptions compressionLevel starts at the default", "[job_zstd][options][usage][compressionlevel]")
{
    TestOptions const opts;
    REQUIRE(opts.compressionLevel() == JobZstdOptions::kDefaultCompressionLevel);
}

TEST_CASE("JobZstdOptions out-of-range compression level falls back to the default", "[job_zstd][options][edge][compressionlevel]")
{
    TestOptions opts;

    int const belowMin = JobZstdOptions::minCompressionLevel() - 1;
    int const aboveMax = JobZstdOptions::maxCompressionLevel() + 1;

    int const validLevel = std::min(9, JobZstdOptions::maxCompressionLevel());
    REQUIRE(opts.setCompressionLevel(validLevel));
    REQUIRE(opts.compressionLevel() == validLevel);

    REQUIRE(opts.setCompressionLevel(aboveMax));
    REQUIRE(opts.compressionLevel() == JobZstdOptions::kDefaultCompressionLevel);

    REQUIRE(opts.setCompressionLevel(9));
    REQUIRE(opts.setCompressionLevel(belowMin));
    REQUIRE(opts.compressionLevel() == JobZstdOptions::kDefaultCompressionLevel);
}

TEST_CASE("JobZstdOptions setCompressionLevel reports no change if the resolved level is identical", "[job_zstd][options][edge][compressionlevel]")
{
    TestOptions opts;

    int const belowMin = JobZstdOptions::minCompressionLevel() - 1;
    int const aboveMax = JobZstdOptions::maxCompressionLevel() + 1;

    REQUIRE(opts.setCompressionLevel(9));
    REQUIRE(opts.setCompressionLevel(aboveMax));
    REQUIRE(opts.compressionLevel() == JobZstdOptions::kDefaultCompressionLevel);

    REQUIRE_FALSE(opts.setCompressionLevel(belowMin));
    REQUIRE(opts.compressionLevel() == JobZstdOptions::kDefaultCompressionLevel);
}

TEST_CASE("JobZstdOptions minCompressionLevel is strictly less than maxCompressionLevel", "[job_zstd][options][edge][compressionlevel]")
{
    REQUIRE(JobZstdOptions::minCompressionLevel() < JobZstdOptions::maxCompressionLevel());
}

TEST_CASE("JobZstdOptions notifyFinished with no callback set is a harmless no-op", "[job_zstd][options][edge][callback]")
{
    TestOptions opts;
    REQUIRE_NOTHROW(opts.notifyFinished()); // Nobody's listening, no harm done.
}

TEST_CASE("JobZstdOptions setOnFinished can replace a previously set callback", "[job_zstd][options][edge][callback]")
{
    TestOptions opts;
    int callCount = 0;

    opts.setOnFinished([&callCount]() {
        callCount = 1;
    });

    opts.setOnFinished([&callCount]() {
        callCount = 2;
    });

    opts.notifyFinished();
    REQUIRE(callCount == 2); // The old callback should be gone, not both firing.
}

TEST_CASE("JobZstdOptions magic strings are distinct and non-empty", "[job_zstd][options][edge][magic]")
{
    REQUIRE_FALSE(JobZstdOptions::magicDirString().empty());
    REQUIRE_FALSE(JobZstdOptions::magicEmptyDirString().empty());
    REQUIRE_FALSE(JobZstdOptions::magicFileString().empty());
    REQUIRE_FALSE(JobZstdOptions::magicLinkString().empty());

    REQUIRE(JobZstdOptions::magicDirString()      != JobZstdOptions::magicEmptyDirString());
    REQUIRE(JobZstdOptions::magicDirString()      != JobZstdOptions::magicFileString());
    REQUIRE(JobZstdOptions::magicDirString()      != JobZstdOptions::magicLinkString());
    REQUIRE(JobZstdOptions::magicEmptyDirString() != JobZstdOptions::magicFileString());
    REQUIRE(JobZstdOptions::magicEmptyDirString() != JobZstdOptions::magicLinkString());
    REQUIRE(JobZstdOptions::magicFileString()     != JobZstdOptions::magicLinkString());
}

TEST_CASE("JobZstdOptions magic strings are stable across repeated calls", "[job_zstd][options][edge][magic]")
{
    REQUIRE(&JobZstdOptions::magicDirString()      == &JobZstdOptions::magicDirString());
    REQUIRE(&JobZstdOptions::magicEmptyDirString() == &JobZstdOptions::magicEmptyDirString());
    REQUIRE(&JobZstdOptions::magicFileString()     == &JobZstdOptions::magicFileString());
    REQUIRE(&JobZstdOptions::magicLinkString()     == &JobZstdOptions::magicLinkString());
}

} // namespace job::zstd