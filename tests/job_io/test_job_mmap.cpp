#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <string_view>

#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include <job_mmap.h>
#include <job_tmp_file.h>

namespace job::io::test {

[[nodiscard]] std::filesystem::path mmapTestPath(std::string_view name)
{
    return std::filesystem::temp_directory_path() /
           ("job_mmap_" + std::to_string(::getpid()) + "_" + std::string(name) + ".tmp");
}

[[nodiscard]] std::size_t systemPageSize()
{
    const long pageSize = ::sysconf(_SC_PAGESIZE);
    REQUIRE(pageSize > 0);
    return static_cast<std::size_t>(pageSize);
}

} // namespace job::io::test

//////////////////////////////////////////////////////////
// Block 1: Usage / examples
//////////////////////////////////////////////////////////

TEST_CASE("JobMmap maps an existing file", "[job_io][mmap][usage]")
{
    const auto path = job::io::test::mmapTestPath("open");
    job::io::JobTmpFile tmp(path, std::string_view("JosephsOddBuilder"));

    job::io::JobMmap mmap(path);

    REQUIRE(mmap.isOpen());
    REQUIRE(mmap.isValid());
    REQUIRE(mmap.fileBacked());
    REQUIRE_FALSE(mmap.anonymous());

    REQUIRE(mmap.fd() >= 0);
    REQUIRE(mmap.addr() != nullptr);

    REQUIRE(mmap.fileSize() == 17);
    REQUIRE(mmap.mapLength() == 17);
    REQUIRE(mmap.mappedSize() == 17);
    REQUIRE(mmap.pageSize() > 0);

    REQUIRE(mmap.mappedRanges().size() == 1);
    REQUIRE(mmap.mappedRanges().front() == job::io::JobMemRange(0, 17));

    const auto *bytes = static_cast<const char *>(mmap.addr());

    REQUIRE(std::memcmp(bytes, "JosephsOddBuilder", 17) == 0);
}

TEST_CASE("JobMmap exposes its JobFile backing", "[job_io][mmap][usage][file]")
{
    const auto path = job::io::test::mmapTestPath("job_file");
    job::io::JobTmpFile tmp(path, std::string_view("Hello mapped world"));

    job::io::JobMmap mmap(path);

    REQUIRE(mmap.isValid());

    REQUIRE(mmap.file().isOpen());
    REQUIRE(mmap.file().fd() == mmap.fd());
    REQUIRE(mmap.file().size() == mmap.fileSize());
    REQUIRE(mmap.file().path() == path);
}

TEST_CASE("JobMmap delegates file IO through IODevice", "[job_io][mmap][usage][io]")
{
    const auto path = job::io::test::mmapTestPath("io");
    job::io::JobTmpFile tmp(path, std::string_view("Hello mapped world"));

    job::io::JobMmap mmap(path);

    REQUIRE(mmap.isValid());

    char hello[6]{};

    REQUIRE(mmap.read(hello, 5) == 5);
    REQUIRE(std::string_view(hello, 5) == "Hello");
    REQUIRE(mmap.tell() == 5);

    REQUIRE(mmap.seek(6, job::io::JobFile::Seek::Begin));

    char mapped[7]{};

    REQUIRE(mmap.read(mapped, 6) == 6);
    REQUIRE(std::string_view(mapped, 6) == "mapped");
    REQUIRE(mmap.tell() == 12);
}

TEST_CASE("JobMmap writes through its JobFile backing", "[job_io][mmap][usage][write]")
{
    const auto path = job::io::test::mmapTestPath("write");
    job::io::JobTmpFile tmp(path, 8, '0');

    job::io::JobMmap mmap(path);

    REQUIRE(mmap.isValid());

    REQUIRE(mmap.seek(0, job::io::JobFile::Seek::Begin));
    REQUIRE(mmap.write("JOB!", 4) == 4);
    REQUIRE(mmap.flush());

    REQUIRE(mmap.seek(0, job::io::JobFile::Seek::Begin));

    char buffer[5]{};

    REQUIRE(mmap.read(buffer, 4) == 4);
    REQUIRE(std::string_view(buffer, 4) == "JOB!");
}

TEST_CASE("JobMmap exposes mapped memory directly", "[job_io][mmap][usage][direct]")
{
    const std::size_t pageSize = job::io::test::systemPageSize();
    const auto path = job::io::test::mmapTestPath("direct");

    job::io::JobTmpFile tmp(path, pageSize, std::byte{0x00});

    {
        job::io::JobMmap mmap(path);

        REQUIRE(mmap.isValid());

        auto *bytes = static_cast<std::byte *>(mmap.addr());

        REQUIRE(bytes != nullptr);

        bytes[0] = std::byte{0x12};
        bytes[1] = std::byte{0x34};
        bytes[2] = std::byte{0x56};
        bytes[3] = std::byte{0x78};

        REQUIRE(::msync(mmap.addr(), mmap.mapLength(), MS_SYNC) == 0);
    }

    REQUIRE(tmp.file().seek(0, job::io::JobFile::Seek::Begin));

    std::uint8_t bytes[4]{};

    REQUIRE(tmp.file().read(reinterpret_cast<char *>(bytes), sizeof(bytes)) == 4);

    REQUIRE(bytes[0] == 0x12);
    REQUIRE(bytes[1] == 0x34);
    REQUIRE(bytes[2] == 0x56);
    REQUIRE(bytes[3] == 0x78);
}

TEST_CASE("JobMmap maps anonymous memory", "[job_io][mmap][usage][anonymous]")
{
    const std::size_t pageSize = job::io::test::systemPageSize();

    job::io::JobMmap mmap(pageSize * 2);

    REQUIRE(mmap.anonymous());
    REQUIRE_FALSE(mmap.fileBacked());

    REQUIRE(mmap.isOpen());
    REQUIRE(mmap.isValid());

    REQUIRE(mmap.fd() == -1);
    REQUIRE(mmap.addr() != nullptr);

    REQUIRE(mmap.mapLength() == pageSize * 2);
    REQUIRE(mmap.mappedSize() == pageSize * 2);

    REQUIRE(mmap.mappedRanges().size() == 1);
    REQUIRE(mmap.mappedRanges().front() == job::io::JobMemRange(0, pageSize * 2));

    auto *bytes = static_cast<std::byte *>(mmap.addr());

    bytes[0] = std::byte{0xAA};
    bytes[pageSize] = std::byte{0xBB};

    REQUIRE(bytes[0] == std::byte{0xAA});
    REQUIRE(bytes[pageSize] == std::byte{0xBB});
}

TEST_CASE("JobMmap borrows an existing file descriptor", "[job_io][mmap][usage][fd]")
{
    const auto path = job::io::test::mmapTestPath("borrow_fd");
    job::io::JobTmpFile tmp(path, std::string_view("borrowed"));

    const int fd = ::open(path.c_str(), O_RDWR);
    REQUIRE(fd >= 0);

    {
        job::io::JobMmap mmap(fd, 0, false, false);

        REQUIRE(mmap.isValid());
        REQUIRE(mmap.fd() == fd);
        REQUIRE(mmap.file().fd() == fd);
        REQUIRE_FALSE(mmap.file().owned());

        REQUIRE(mmap.fileSize() == 8);

        const auto *bytes = static_cast<const char *>(mmap.addr());
        REQUIRE(std::string_view(bytes, 8) == "borrowed");
    }

    // JobMmap -> JobFile borrowed the descriptor, so it remains alive.
    REQUIRE(::lseek(fd, 0, SEEK_SET) == 0);

    char buffer[9]{};

    REQUIRE(::read(fd, buffer, 8) == 8);
    REQUIRE(std::string_view(buffer, 8) == "borrowed");

    REQUIRE(::close(fd) == 0);
}

TEST_CASE("JobMmap borrows a FILE pointer", "[job_io][mmap][usage][file_pointer]")
{
    const auto path = job::io::test::mmapTestPath("borrow_fp");
    job::io::JobTmpFile tmp(path, std::string_view("stdio"));

    FILE *fp = ::fopen(path.c_str(), "r+b");
    REQUIRE(fp != nullptr);

    const int fd = ::fileno(fp);
    REQUIRE(fd >= 0);

    {
        job::io::JobMmap mmap(fp, 0, false, false);

        REQUIRE(mmap.isValid());
        REQUIRE(mmap.file().fp() == fp);
        REQUIRE(mmap.fd() == fd);
        REQUIRE_FALSE(mmap.file().owned());

        const auto *bytes = static_cast<const char *>(mmap.addr());
        REQUIRE(std::string_view(bytes, 5) == "stdio");
    }

    REQUIRE(::fileno(fp) == fd);
    REQUIRE(::fclose(fp) == 0);
}

TEST_CASE("JobMmap move construction transfers mapping ownership", "[job_io][mmap][usage][move]")
{
    const auto path = job::io::test::mmapTestPath("move");
    job::io::JobTmpFile tmp(path, std::string_view("move mmap"));

    job::io::JobMmap source(path);

    REQUIRE(source.isValid());

    void *addr = source.addr();
    const int fd = source.fd();

    job::io::JobMmap destination(std::move(source));

    REQUIRE(destination.isValid());
    REQUIRE(destination.addr() == addr);
    REQUIRE(destination.fd() == fd);

    REQUIRE(destination.mapLength() == 9);
    REQUIRE(destination.mappedSize() == 9);

    REQUIRE(source.addr() == nullptr);
    REQUIRE_FALSE(source.isValid());
    REQUIRE(source.fd() == -1);

    const auto *bytes = static_cast<const char *>(destination.addr());
    REQUIRE(std::string_view(bytes, 9) == "move mmap");
}

TEST_CASE("JobMmap factories create file and anonymous mappings", "[job_io][mmap][usage][factory]")
{
    const std::size_t pageSize = job::io::test::systemPageSize();
    const auto path = job::io::test::mmapTestPath("factory");

    job::io::JobTmpFile tmp(path, pageSize, std::byte{0x00});

    const auto shared = job::io::JobMmap::createShared(path);
    const auto unique = job::io::JobMmap::createUniq(pageSize);

    REQUIRE(shared);
    REQUIRE(unique);

    REQUIRE(shared->fileBacked());
    REQUIRE(shared->isValid());

    REQUIRE(unique->anonymous());
    REQUIRE(unique->isValid());
}

//////////////////////////////////////////////////////////
// Block 2: Edge cases / lifecycle / fragmentation
//////////////////////////////////////////////////////////

TEST_CASE("JobMmap rejects a missing file mapping", "[job_io][mmap][edge][open]")
{
    const auto path = job::io::test::mmapTestPath("missing");

    std::filesystem::remove(path);

    job::io::JobMmap mmap(path);

    REQUIRE_FALSE(mmap.openDevice());
    REQUIRE_FALSE(mmap.isOpen());
    REQUIRE_FALSE(mmap.isValid());

    REQUIRE(mmap.fd() == -1);
    REQUIRE(mmap.addr() == nullptr);

    REQUIRE(mmap.mappedSize() == 0);
    REQUIRE(mmap.mappedRanges().empty());
}

TEST_CASE("JobMmap cannot map an empty file", "[job_io][mmap][edge][empty]")
{
    const auto path = job::io::test::mmapTestPath("empty");
    job::io::JobTmpFile tmp(path);

    job::io::JobMmap mmap(path);

    // The backing JobFile can be open while there is no valid mmap.
    REQUIRE(mmap.isOpen());
    REQUIRE_FALSE(mmap.isValid());

    REQUIRE(mmap.fileSize() == 0);
    REQUIRE(mmap.mapLength() == 0);
    REQUIRE(mmap.mappedSize() == 0);

    REQUIRE(mmap.addr() == nullptr);
    REQUIRE(mmap.mappedRanges().empty());

    REQUIRE_FALSE(mmap.openDevice());
}

TEST_CASE("JobMmap zero length file IO is harmless", "[job_io][mmap][edge][zero]")
{
    const auto path = job::io::test::mmapTestPath("zero_io");
    job::io::JobTmpFile tmp(path, 4096, std::byte{0x00});

    job::io::JobMmap mmap(path);

    REQUIRE(mmap.isValid());

    char value{};

    REQUIRE(mmap.read(&value, 0) == 0);
    REQUIRE(mmap.write(&value, 0) == 0);
    REQUIRE(mmap.tell() == 0);
}

TEST_CASE("JobMmap closeDevice releases mapping and file resources", "[job_io][mmap][edge][lifecycle]")
{
    const std::size_t pageSize = job::io::test::systemPageSize();
    const auto path = job::io::test::mmapTestPath("close");

    job::io::JobTmpFile tmp(path, pageSize, std::byte{0x00});

    job::io::JobMmap mmap(path);

    REQUIRE(mmap.isValid());
    REQUIRE(mmap.isOpen());

    mmap.closeDevice();

    REQUIRE_FALSE(mmap.isOpen());
    REQUIRE_FALSE(mmap.isValid());

    REQUIRE(mmap.fd() == -1);
    REQUIRE(mmap.addr() == nullptr);

    REQUIRE(mmap.mappedSize() == 0);
    REQUIRE(mmap.mappedRanges().empty());

    // mapLength describes the mapping domain and allows the object to
    // establish a new mapping if it is opened again.
    REQUIRE(mmap.mapLength() == pageSize);

    mmap.closeDevice();

    REQUIRE_FALSE(mmap.isOpen());
    REQUIRE_FALSE(mmap.isValid());

    REQUIRE(mmap.openDevice());
    REQUIRE(mmap.isValid());
    REQUIRE(mmap.mappedSize() == pageSize);
}

TEST_CASE("JobMmap partial unmap removes only complete pages", "[job_io][mmap][edge][fragmentation]")
{
    const std::size_t pageSize = job::io::test::systemPageSize();
    const std::size_t fileSize = pageSize * 3;

    const auto path = job::io::test::mmapTestPath("partial_unmap");
    job::io::JobTmpFile tmp(path, fileSize, std::byte{0x00});

    job::io::JobMmap mmap(path);

    REQUIRE(mmap.isValid());
    REQUIRE(mmap.pageSize() == pageSize);
    REQUIRE(mmap.mappedSize() == fileSize);

    // Requested:
    //
    //   half page 0 -> half page 2
    //
    // Only page 1 is completely enclosed, so alignedInward() produces:
    //
    //   [pageSize, pageSize * 2)
    REQUIRE(mmap.unmap(
        job::io::JobMemRange(
            pageSize / 2,
            (pageSize * 2) + (pageSize / 2))));

    REQUIRE(mmap.mapLength() == fileSize);
    REQUIRE(mmap.mappedSize() == pageSize * 2);

    const auto &ranges = mmap.mappedRanges();

    REQUIRE(ranges.size() == 2);
    REQUIRE(ranges[0] == job::io::JobMemRange(0, pageSize));
    REQUIRE(ranges[1] == job::io::JobMemRange(pageSize * 2, pageSize * 3));
}

TEST_CASE("JobMmap partial unmap ignores ranges containing no complete page", "[job_io][mmap][edge][fragmentation]")
{
    const std::size_t pageSize = job::io::test::systemPageSize();

    const auto path = job::io::test::mmapTestPath("small_unmap");
    job::io::JobTmpFile tmp(path, pageSize * 2, std::byte{0x00});

    job::io::JobMmap mmap(path);

    REQUIRE(mmap.isValid());

    const std::size_t before = mmap.mappedSize();

    REQUIRE(mmap.unmap(job::io::JobMemRange(1, pageSize - 1)));

    REQUIRE(mmap.mappedSize() == before);
    REQUIRE(mmap.mappedRanges().size() == 1);
    REQUIRE(mmap.mappedRanges().front() == job::io::JobMemRange(0, pageSize * 2));
}

TEST_CASE("JobMmap complete unmap preserves its mapping domain", "[job_io][mmap][edge][unmap]")
{
    const std::size_t pageSize = job::io::test::systemPageSize();

    const auto path = job::io::test::mmapTestPath("full_unmap");
    job::io::JobTmpFile tmp(path, pageSize * 2, std::byte{0x00});

    job::io::JobMmap mmap(path);

    REQUIRE(mmap.isValid());

    const std::size_t mapLength = mmap.mapLength();

    mmap.unmap();

    REQUIRE_FALSE(mmap.isValid());

    REQUIRE(mmap.addr() == nullptr);
    REQUIRE(mmap.mappedSize() == 0);
    REQUIRE(mmap.mappedRanges().empty());

    REQUIRE(mmap.mapLength() == mapLength);

    // unmap() destroys the mapping, not the JobFile backing.
    REQUIRE(mmap.isOpen());
    REQUIRE(mmap.fd() >= 0);

    REQUIRE(mmap.openDevice());
    REQUIRE(mmap.isValid());
    REQUIRE(mmap.mappedSize() == mapLength);
}

TEST_CASE("JobMmap rejects ranges outside the mapping domain", "[job_io][mmap][edge][range]")
{
    const std::size_t pageSize = job::io::test::systemPageSize();

    const auto path = job::io::test::mmapTestPath("bad_range");
    job::io::JobTmpFile tmp(path, pageSize, std::byte{0x00});

    job::io::JobMmap mmap(path);

    REQUIRE(mmap.isValid());

    REQUIRE_FALSE(mmap.unmap(job::io::JobMemRange(100, 100)));

    REQUIRE_FALSE(mmap.unmap(
        job::io::JobMemRange(
            mmap.mapLength(),
            mmap.mapLength() + 1)));

    REQUIRE_FALSE(mmap.prefetch(job::io::JobMemRange(100, 100)));

    REQUIRE_FALSE(mmap.prefetch(
        job::io::JobMemRange(
            mmap.mapLength(),
            mmap.mapLength() + 1)));
}

TEST_CASE("JobMmap refuses operations across unmapped holes", "[job_io][mmap][edge][fragmentation]")
{
    const std::size_t pageSize = job::io::test::systemPageSize();

    const auto path = job::io::test::mmapTestPath("hole");
    job::io::JobTmpFile tmp(path, pageSize * 3, std::byte{0x00});

    job::io::JobMmap mmap(path);

    REQUIRE(mmap.isValid());

    REQUIRE(mmap.unmap(job::io::JobMemRange(pageSize, pageSize * 2)));

    REQUIRE(mmap.mappedRanges().size() == 2);

    REQUIRE_FALSE(mmap.prefetch(
        job::io::JobMemRange(
            pageSize / 2,
            pageSize * 2 + pageSize / 2)));

    REQUIRE_FALSE(mmap.lock(
        job::io::JobMemRange(
            pageSize / 2,
            pageSize * 2 + pageSize / 2)));
}

TEST_CASE("JobMmap grows an unfragmented file mapping", "[job_io][mmap][edge][grow]")
{
    const std::size_t pageSize = job::io::test::systemPageSize();

    const auto path = job::io::test::mmapTestPath("grow");
    job::io::JobTmpFile tmp(path, pageSize, std::byte{0x00});

    job::io::JobMmap mmap(path);

    REQUIRE(mmap.isValid());

    REQUIRE(mmap.fileSize() == pageSize);
    REQUIRE(mmap.mapLength() == pageSize);
    REQUIRE(mmap.mappedSize() == pageSize);

    REQUIRE(mmap.grow(pageSize * 4));

    REQUIRE(mmap.isValid());

    REQUIRE(mmap.fileSize() == pageSize * 4);
    REQUIRE(mmap.mapLength() == pageSize * 4);
    REQUIRE(mmap.mappedSize() == pageSize * 4);

    REQUIRE(mmap.mappedRanges().size() == 1);
    REQUIRE(mmap.mappedRanges().front() == job::io::JobMemRange(0, pageSize * 4));

    REQUIRE(std::filesystem::file_size(path) == pageSize * 4);
}

TEST_CASE("JobMmap grow with the current size is idempotent", "[job_io][mmap][edge][grow]")
{
    const std::size_t pageSize = job::io::test::systemPageSize();

    const auto path = job::io::test::mmapTestPath("grow_same");
    job::io::JobTmpFile tmp(path, pageSize, std::byte{0x00});

    job::io::JobMmap mmap(path);

    REQUIRE(mmap.isValid());

    void *addr = mmap.addr();

    REQUIRE(mmap.grow(pageSize));

    REQUIRE(mmap.addr() == addr);
    REQUIRE(mmap.mapLength() == pageSize);
    REQUIRE(mmap.mappedSize() == pageSize);
}

TEST_CASE("JobMmap does not shrink through grow", "[job_io][mmap][edge][grow]")
{
    const std::size_t pageSize = job::io::test::systemPageSize();

    const auto path = job::io::test::mmapTestPath("grow_shrink");
    job::io::JobTmpFile tmp(path, pageSize * 2, std::byte{0x00});

    job::io::JobMmap mmap(path);

    REQUIRE(mmap.isValid());

    REQUIRE_FALSE(mmap.grow(pageSize));

    REQUIRE(mmap.fileSize() == pageSize * 2);
    REQUIRE(mmap.mapLength() == pageSize * 2);
    REQUIRE(mmap.mappedSize() == pageSize * 2);
}

TEST_CASE("JobMmap refuses to grow a fragmented mapping", "[job_io][mmap][edge][grow][fragmentation]")
{
    const std::size_t pageSize = job::io::test::systemPageSize();

    const auto path = job::io::test::mmapTestPath("grow_fragmented");
    job::io::JobTmpFile tmp(path, pageSize * 3, std::byte{0x00});

    job::io::JobMmap mmap(path);

    REQUIRE(mmap.isValid());

    REQUIRE(mmap.unmap(job::io::JobMemRange(pageSize, pageSize * 2)));

    REQUIRE(mmap.mappedRanges().size() == 2);

    REQUIRE_FALSE(mmap.grow(pageSize * 4));

    REQUIRE(mmap.fileSize() == pageSize * 3);
    REQUIRE(mmap.mapLength() == pageSize * 3);
    REQUIRE(mmap.mappedSize() == pageSize * 2);
}

TEST_CASE("JobMmap grows anonymous mappings", "[job_io][mmap][edge][grow][anonymous]")
{
    const std::size_t pageSize = job::io::test::systemPageSize();

    job::io::JobMmap mmap(pageSize);

    REQUIRE(mmap.isValid());
    REQUIRE(mmap.anonymous());

    REQUIRE(mmap.grow(pageSize * 4));

    REQUIRE(mmap.isValid());
    REQUIRE(mmap.anonymous());

    REQUIRE(mmap.mapLength() == pageSize * 4);
    REQUIRE(mmap.mappedSize() == pageSize * 4);

    REQUIRE(mmap.mappedRanges().size() == 1);
    REQUIRE(mmap.mappedRanges().front() == job::io::JobMemRange(0, pageSize * 4));
}

TEST_CASE("JobMmap prefetches complete and selected mapped ranges", "[job_io][mmap][edge][prefetch]")
{
    const std::size_t pageSize = job::io::test::systemPageSize();

    const auto path = job::io::test::mmapTestPath("prefetch");
    job::io::JobTmpFile tmp(path, pageSize * 4, std::byte{0x00});

    job::io::JobMmap mmap(path);

    REQUIRE(mmap.isValid());

    REQUIRE(mmap.prefetch());
    REQUIRE(mmap.prefetch(job::io::JobMemRange(pageSize, pageSize * 2)));
}

TEST_CASE("JobMmap exposes NUMA preference state", "[job_io][mmap][edge][numa]")
{
    const std::size_t pageSize = job::io::test::systemPageSize();

    job::io::JobMmap mmap(pageSize, 0, false);

    REQUIRE_FALSE(mmap.numa());

    mmap.setNuma(true);
    REQUIRE(mmap.numa());

    mmap.setNuma(false);
    REQUIRE_FALSE(mmap.numa());
}

TEST_CASE("JobMmap delegates range locking to JobMemLock", "[job_io][mmap][edge][lock]")
{
    const std::size_t pageSize = job::io::test::systemPageSize();

    job::io::JobMmap mmap(pageSize * 2);

    REQUIRE(mmap.isValid());

    const job::io::JobMemRange range(0, pageSize);

    // mlock can legitimately fail because of RLIMIT_MEMLOCK or a restricted
    // runtime environment. If the kernel accepts it, verify JobMmap's
    // delegation and bookkeeping.
    if (mmap.lock(range)) {
        REQUIRE(mmap.lockedSize() == pageSize);
        REQUIRE(mmap.lockedRanges().size() == 1);
        REQUIRE(mmap.lockedRanges().front() == range);

        REQUIRE(mmap.memLock().isLocked(range));

        REQUIRE(mmap.unlock(range));

        REQUIRE(mmap.lockedSize() == 0);
        REQUIRE(mmap.lockedRanges().empty());
    } else {
        SUCCEED("mlock unavailable in this test environment");
    }
}

TEST_CASE("JobMmap whole lock requires an unfragmented mapping", "[job_io][mmap][edge][lock][fragmentation]")
{
    const std::size_t pageSize = job::io::test::systemPageSize();

    job::io::JobMmap mmap(pageSize * 3);

    REQUIRE(mmap.isValid());

    REQUIRE(mmap.unmap(job::io::JobMemRange(pageSize, pageSize * 2)));

    REQUIRE(mmap.mappedRanges().size() == 2);
    REQUIRE_FALSE(mmap.lock());
}

TEST_CASE("JobMmap move assignment releases the previous mapping", "[job_io][mmap][edge][move]")
{
    const std::size_t pageSize = job::io::test::systemPageSize();

    const auto firstPath = job::io::test::mmapTestPath("move_assign_first");
    const auto secondPath = job::io::test::mmapTestPath("move_assign_second");

    job::io::JobTmpFile firstTmp(firstPath, pageSize, std::byte{0x11});
    job::io::JobTmpFile secondTmp(secondPath, pageSize, std::byte{0x22});

    job::io::JobMmap source(firstPath);
    job::io::JobMmap destination(secondPath);

    REQUIRE(source.isValid());
    REQUIRE(destination.isValid());

    void *sourceAddr = source.addr();
    const int sourceFd = source.fd();

    destination = std::move(source);

    REQUIRE(destination.isValid());
    REQUIRE(destination.addr() == sourceAddr);
    REQUIRE(destination.fd() == sourceFd);

    REQUIRE_FALSE(source.isValid());
    REQUIRE(source.addr() == nullptr);
    REQUIRE(source.fd() == -1);

    const auto *bytes = static_cast<const std::byte *>(destination.addr());
    REQUIRE(bytes[0] == std::byte{0x11});
}

//////////////////////////////////////////////////////////
// Block 3: Benchmarks / stress
//////////////////////////////////////////////////////////

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Benchmark JobMmap direct mapped writes", "[job_io][mmap][benchmark][write]")
{
    const std::size_t pageSize = job::io::test::systemPageSize();

    job::io::JobMmap mmap(pageSize * 16);

    REQUIRE(mmap.isValid());

    auto *data = static_cast<std::uint64_t *>(mmap.addr());
    REQUIRE(data != nullptr);

    constexpr std::size_t iterations = 1'000'000;
    const std::size_t count = (pageSize * 16) / sizeof(std::uint64_t);

    BENCHMARK("1,000,000 mapped uint64 writes") {
        for (std::size_t i = 0; i < iterations; ++i)
            data[i % count] = static_cast<std::uint64_t>(i);

        return data[0];
    };
}

TEST_CASE("Benchmark JobMmap direct mapped reads", "[job_io][mmap][benchmark][read]")
{
    const std::size_t pageSize = job::io::test::systemPageSize();

    job::io::JobMmap mmap(pageSize * 16);

    REQUIRE(mmap.isValid());

    auto *data = static_cast<std::uint64_t *>(mmap.addr());
    REQUIRE(data != nullptr);

    const std::size_t count = (pageSize * 16) / sizeof(std::uint64_t);

    for (std::size_t i = 0; i < count; ++i)
        data[i] = static_cast<std::uint64_t>(i + 1);

    constexpr std::size_t iterations = 1'000'000;

    BENCHMARK("1,000,000 mapped uint64 reads") {
        std::uint64_t checksum = 0;

        for (std::size_t i = 0; i < iterations; ++i)
            checksum += data[i % count];

        return checksum;
    };
}

TEST_CASE("Stress JobMmap repeated partial unmap bookkeeping", "[job_io][mmap][benchmark][fragmentation]")
{
    const std::size_t pageSize = job::io::test::systemPageSize();
    constexpr std::size_t pageCount = 256;

    BENCHMARK("Unmap alternating pages from a 256 page anonymous mapping") {
        job::io::JobMmap mmap(pageSize * pageCount);

        if (!mmap.isValid())
            return std::size_t{0};

        for (std::size_t page = 1; page < pageCount; page += 2)
            (void)mmap.unmap(job::io::JobMemRange(page * pageSize, (page + 1) * pageSize));

        return mmap.mappedRanges().size();
    };
}

#endif