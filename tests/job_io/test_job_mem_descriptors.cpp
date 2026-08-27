#include <catch2/catch_test_macros.hpp>

#include <compare>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <filesystem>
#include <string>
#include <unistd.h>


#include <job_mem_extent.h>
#include <job_mem_page.h>
#include <job_mem_range.h>
#include <job_mem_region_concept.h>
#include <job_mem_size.h>
#include <job_mem_span.h>
#include <job_mmap.h>
#include <job_tmp_file.h>

namespace job::io::test {

//////////////////////////////////////////////////////////
// JobMemRange compile-time checks
//////////////////////////////////////////////////////////

constexpr JobMemRange kRangeA(100, 200);
constexpr JobMemRange kRangeB(150, 250);
constexpr JobMemRange kRangeAdjacent(200, 300);
constexpr JobMemRange kRangeSeparated(300, 400);
constexpr JobMemRange kEmptyRange(128, 128);

static_assert(kRangeA.first() == 100);
static_assert(kRangeA.last() == 200);
static_assert(kRangeA.size() == 100);
static_assert(!kRangeA.empty());

static_assert(kEmptyRange.first() == 128);
static_assert(kEmptyRange.last() == 128);
static_assert(kEmptyRange.size() == 0);
static_assert(kEmptyRange.empty());

static_assert(kRangeA.contains(100));
static_assert(kRangeA.contains(150));
static_assert(kRangeA.contains(199));
static_assert(!kRangeA.contains(99));
static_assert(!kRangeA.contains(200));

static_assert(kRangeA.contains(JobMemRange(100, 200)));
static_assert(kRangeA.contains(JobMemRange(120, 180)));
static_assert(kRangeA.contains(JobMemRange(100, 100)));
static_assert(kRangeA.contains(JobMemRange(200, 200)));

static_assert(!kRangeA.contains(JobMemRange(99, 150)));
static_assert(!kRangeA.contains(JobMemRange(150, 201)));

static_assert(kRangeA.overlaps(kRangeB));
static_assert(!kRangeA.overlaps(kRangeAdjacent));
static_assert(!kRangeA.overlaps(kRangeSeparated));

static_assert(!kRangeA.adjacent(kRangeB));
static_assert(kRangeA.adjacent(kRangeAdjacent));
static_assert(!kRangeA.adjacent(kRangeSeparated));

static_assert(kRangeA.mergeable(kRangeB));
static_assert(kRangeA.mergeable(kRangeAdjacent));
static_assert(!kRangeA.mergeable(kRangeSeparated));

static_assert(kRangeA.merged(kRangeB) == JobMemRange(100, 250));
static_assert(kRangeA.merged(kRangeAdjacent) == JobMemRange(100, 300));
static_assert(kRangeA.intersection(kRangeB) == JobMemRange(150, 200));

static_assert(JobMemRange::fromSize(100, 50) == JobMemRange(100, 150));
static_assert(JobMemRange::fromSize(100, 50).first() == 100);
static_assert(JobMemRange::fromSize(100, 50).last() == 150);
static_assert(JobMemRange::fromSize(100, 50).size() == 50);

static_assert(JobMemRange(100, 900).alignedInward(256) == JobMemRange(256, 768));
static_assert(JobMemRange(100, 900).alignedOutward(256) == JobMemRange(0, 1024));

static_assert(JobMemRange(101, 120).alignedInward(64).empty());

static_assert(JobMemRange::validAlignment(1));
static_assert(JobMemRange::validAlignment(2));
static_assert(JobMemRange::validAlignment(4));
static_assert(JobMemRange::validAlignment(16));
static_assert(JobMemRange::validAlignment(4096));

static_assert(!JobMemRange::validAlignment(0));
static_assert(!JobMemRange::validAlignment(3));
static_assert(!JobMemRange::validAlignment(6));
static_assert(!JobMemRange::validAlignment(12));

static_assert(JobMemRange(100, 200) == JobMemRange(100, 200));
static_assert(JobMemRange(100, 200) != JobMemRange(100, 201));
static_assert(JobMemRange(100, 200) < JobMemRange(101, 200));

//////////////////////////////////////////////////////////
// JobMemRegion concept compile-time checks
//////////////////////////////////////////////////////////

static_assert(!JobMemRegion<JobMemRange>);

static_assert(JobMemRegion<JobMemExtent>);
static_assert(JobMemRegion<JobMemPage>);
static_assert(JobMemRegion<JobMemSpan>);
static_assert(!JobMemRegion<JobMemSize>);






//////////////////////////////////////////////////////////
// JobMemSize compile-time checks
//////////////////////////////////////////////////////////

constexpr JobMemSize kSizeClass(
    1,
    64,
    16,
    4096,
    4);

static_assert(kSizeClass.id() == 1);
static_assert(kSizeClass.objectSize() == 64);
static_assert(kSizeClass.alignment() == 16);
static_assert(kSizeClass.pageSize() == 4096);
static_assert(kSizeClass.pagesPerSpan() == 4);

static_assert(kSizeClass.spanSize() == 16384);

static_assert(kSizeClass.objectsPerPage() == 64);
static_assert(kSizeClass.objectsPerSpan() == 256);

static_assert(kSizeClass.wastePerPage() == 0);
static_assert(kSizeClass.wastePerSpan() == 0);

static_assert(kSizeClass.fitsInPage());
static_assert(kSizeClass.fitsInSpan());

static_assert(JobMemSize::validAlignment(1));
static_assert(JobMemSize::validAlignment(16));
static_assert(JobMemSize::validAlignment(4096));

static_assert(!JobMemSize::validAlignment(0));
static_assert(!JobMemSize::validAlignment(3));
static_assert(!JobMemSize::validAlignment(24));

//////////////////////////////////////////////////////////
// Descriptor properties
//////////////////////////////////////////////////////////

static_assert(std::is_copy_constructible_v<JobMemRange>);
static_assert(std::is_move_constructible_v<JobMemRange>);
static_assert(std::is_copy_assignable_v<JobMemRange>);
static_assert(std::is_move_assignable_v<JobMemRange>);

static_assert(std::is_copy_constructible_v<JobMemExtent>);
static_assert(std::is_move_constructible_v<JobMemExtent>);
static_assert(std::is_copy_assignable_v<JobMemExtent>);
static_assert(std::is_move_assignable_v<JobMemExtent>);

static_assert(std::is_copy_constructible_v<JobMemPage>);
static_assert(std::is_move_constructible_v<JobMemPage>);
static_assert(std::is_copy_assignable_v<JobMemPage>);
static_assert(std::is_move_assignable_v<JobMemPage>);

static_assert(std::is_copy_constructible_v<JobMemSpan>);
static_assert(std::is_move_constructible_v<JobMemSpan>);
static_assert(std::is_copy_assignable_v<JobMemSpan>);
static_assert(std::is_move_assignable_v<JobMemSpan>);

static_assert(std::is_copy_constructible_v<JobMemSize>);
static_assert(std::is_move_constructible_v<JobMemSize>);
static_assert(std::is_copy_assignable_v<JobMemSize>);
static_assert(std::is_move_assignable_v<JobMemSize>);

} // namespace job::io::test

//////////////////////////////////////////////////////////
// JobMemRange
//////////////////////////////////////////////////////////

TEST_CASE("JobMemRange describes half-open byte geometry", "[job_io][mem][range]")
{
    const job::io::JobMemRange range(100, 200);

    REQUIRE(range.first() == 100);
    REQUIRE(range.last() == 200);
    REQUIRE(range.size() == 100);
    REQUIRE_FALSE(range.empty());

    REQUIRE(range.contains(100));
    REQUIRE(range.contains(150));
    REQUIRE(range.contains(199));

    REQUIRE_FALSE(range.contains(99));
    REQUIRE_FALSE(range.contains(200));
}

TEST_CASE("JobMemRange supports empty ranges", "[job_io][mem][range]")
{
    const job::io::JobMemRange range(128, 128);

    REQUIRE(range.empty());
    REQUIRE(range.size() == 0);
    REQUIRE(range.first() == 128);
    REQUIRE(range.last() == 128);

    REQUIRE_FALSE(range.contains(127));
    REQUIRE_FALSE(range.contains(128));
    REQUIRE_FALSE(range.contains(129));
}

TEST_CASE("JobMemRange contains other ranges", "[job_io][mem][range]")
{
    const job::io::JobMemRange range(100, 200);

    REQUIRE(range.contains(job::io::JobMemRange(100, 200)));
    REQUIRE(range.contains(job::io::JobMemRange(120, 180)));

    REQUIRE(range.contains(job::io::JobMemRange(100, 100)));
    REQUIRE(range.contains(job::io::JobMemRange(200, 200)));

    REQUIRE_FALSE(range.contains(job::io::JobMemRange(99, 150)));
    REQUIRE_FALSE(range.contains(job::io::JobMemRange(150, 201)));
}

TEST_CASE("JobMemRange detects overlap adjacency and mergeability", "[job_io][mem][range]")
{
    const job::io::JobMemRange range(100, 200);
    const job::io::JobMemRange overlap(150, 250);
    const job::io::JobMemRange adjacent(200, 300);
    const job::io::JobMemRange separated(300, 400);

    REQUIRE(range.overlaps(overlap));
    REQUIRE_FALSE(range.overlaps(adjacent));
    REQUIRE_FALSE(range.overlaps(separated));

    REQUIRE_FALSE(range.adjacent(overlap));
    REQUIRE(range.adjacent(adjacent));
    REQUIRE_FALSE(range.adjacent(separated));

    REQUIRE(range.mergeable(overlap));
    REQUIRE(range.mergeable(adjacent));
    REQUIRE_FALSE(range.mergeable(separated));
}

TEST_CASE("JobMemRange merges and intersects ranges", "[job_io][mem][range]")
{
    const job::io::JobMemRange range(100, 200);
    const job::io::JobMemRange overlap(150, 250);
    const job::io::JobMemRange adjacent(200, 300);

    REQUIRE(range.merged(overlap) == job::io::JobMemRange(100, 250));
    REQUIRE(range.merged(adjacent) == job::io::JobMemRange(100, 300));
    REQUIRE(range.intersection(overlap) == job::io::JobMemRange(150, 200));
}

TEST_CASE("JobMemRange creates ranges from size", "[job_io][mem][range]")
{
    const auto range = job::io::JobMemRange::fromSize(4096, 1024);

    REQUIRE(range.first() == 4096);
    REQUIRE(range.last() == 5120);
    REQUIRE(range.size() == 1024);
}

TEST_CASE("JobMemRange aligns ranges inward and outward", "[job_io][mem][range]")
{
    const job::io::JobMemRange range(100, 900);

    REQUIRE(range.alignedInward(256) == job::io::JobMemRange(256, 768));
    REQUIRE(range.alignedOutward(256) == job::io::JobMemRange(0, 1024));

    const job::io::JobMemRange tooSmall(101, 120);
    const auto inward = tooSmall.alignedInward(64);

    REQUIRE(inward.empty());
    REQUIRE(tooSmall.contains(inward));
}

TEST_CASE("JobMemRange validates power of two alignment", "[job_io][mem][range]")
{
    REQUIRE(job::io::JobMemRange::validAlignment(1));
    REQUIRE(job::io::JobMemRange::validAlignment(2));
    REQUIRE(job::io::JobMemRange::validAlignment(16));
    REQUIRE(job::io::JobMemRange::validAlignment(4096));

    REQUIRE_FALSE(job::io::JobMemRange::validAlignment(0));
    REQUIRE_FALSE(job::io::JobMemRange::validAlignment(3));
    REQUIRE_FALSE(job::io::JobMemRange::validAlignment(6));
    REQUIRE_FALSE(job::io::JobMemRange::validAlignment(12));
}

TEST_CASE("JobMemRange pointer factories create descriptors", "[job_io][mem][range]")
{
    const auto shared = job::io::JobMemRange::createShared(100, 200);
    const auto unique = job::io::JobMemRange::createUniq(300, 400);

    REQUIRE(shared);
    REQUIRE(unique);

    REQUIRE(*shared == job::io::JobMemRange(100, 200));
    REQUIRE(*unique == job::io::JobMemRange(300, 400));
}


//////////////////////////////////////////////////////////
// JobMemExtent
//////////////////////////////////////////////////////////

TEST_CASE("JobMemExtent describes a range inside a mapped file", "[job_io][mem][extent][usage]")
{
    constexpr std::size_t fileSize = 16384;

    const auto path = std::filesystem::temp_directory_path() /
                      ("job_mem_extent_" + std::to_string(::getpid()) + "_mapped.bin");

    job::io::JobTmpFile tmp(path, fileSize, std::byte{0x5A});

    const auto mmap = job::io::JobMmap::createShared(path);

    REQUIRE(mmap);
    REQUIRE(mmap->isValid());
    REQUIRE(mmap->mapLength() == fileSize);
    REQUIRE(mmap->mappedSize() == fileSize);

    const job::io::JobMemRange range(4096, 12288);
    const job::io::JobMemExtent extent(mmap, range);

    REQUIRE(extent.id() != job::io::JobMemExtent::kInvalidId);
    REQUIRE(extent.range() == range);
    REQUIRE(extent.size() == 8192);

    REQUIRE(extent.mmap() == mmap);
    REQUIRE(extent.addr() == static_cast<std::byte *>(mmap->addr()) + 4096);
}

TEST_CASE("JobMemExtent uses mmap relative byte geometry", "[job_io][mem][extent][usage][range]")
{
    constexpr std::size_t fileSize = 16384;

    const auto path = std::filesystem::temp_directory_path() /
                      ("job_mem_extent_" + std::to_string(::getpid()) + "_geometry.bin");

    job::io::JobTmpFile tmp(path, fileSize, std::byte{0x00});

    const auto mmap = job::io::JobMmap::createShared(path);

    REQUIRE(mmap);
    REQUIRE(mmap->isValid());

    const job::io::JobMemExtent extent(
        mmap,
        job::io::JobMemRange(4096, 12288));

    const auto *mappingBase = static_cast<const std::byte *>(mmap->addr());
    const auto *extentBase = static_cast<const std::byte *>(extent.addr());

    REQUIRE(extentBase == mappingBase + 4096);

    REQUIRE(extent.contains(extentBase));
    REQUIRE(extent.contains(extentBase + 4095));
    REQUIRE(extent.contains(extentBase + 8191));

    REQUIRE_FALSE(extent.contains(mappingBase));
    REQUIRE_FALSE(extent.contains(extentBase + 8192));
}

TEST_CASE("JobMemExtent translates between extent and mmap offsets", "[job_io][mem][extent][usage][offset]")
{
    constexpr std::size_t fileSize = 16384;

    const auto path = std::filesystem::temp_directory_path() /
                      ("job_mem_extent_" + std::to_string(::getpid()) + "_offset.bin");

    job::io::JobTmpFile tmp(path, fileSize, std::byte{0x00});

    const auto mmap = job::io::JobMmap::createShared(path);

    REQUIRE(mmap);
    REQUIRE(mmap->isValid());

    const job::io::JobMemExtent extent(
        mmap,
        job::io::JobMemRange(4096, 12288));

    const auto *base = static_cast<const std::byte *>(extent.addr());

    REQUIRE(extent.offsetOf(base) == 0);
    REQUIRE(extent.offsetOf(base + 1024) == 1024);
    REQUIRE(extent.offsetOf(base + 8191) == 8191);

    REQUIRE(extent.mmapOffsetOf(base) == 4096);
    REQUIRE(extent.mmapOffsetOf(base + 1024) == 5120);
    REQUIRE(extent.mmapOffsetOf(base + 8191) == 12287);

    REQUIRE(extent.ptrAt(0) == base);
    REQUIRE(extent.ptrAt(1024) == base + 1024);
    REQUIRE(extent.ptrAt(8191) == base + 8191);
}

TEST_CASE("JobMemExtent contains mmap relative subranges", "[job_io][mem][extent][usage][range]")
{
    constexpr std::size_t fileSize = 16384;

    const auto path = std::filesystem::temp_directory_path() /
                      ("job_mem_extent_" + std::to_string(::getpid()) + "_contains.bin");

    job::io::JobTmpFile tmp(path, fileSize, std::byte{0x00});

    const auto mmap = job::io::JobMmap::createShared(path);

    REQUIRE(mmap);
    REQUIRE(mmap->isValid());

    const job::io::JobMemExtent extent(
        mmap,
        job::io::JobMemRange(4096, 12288));

    REQUIRE(extent.contains(job::io::JobMemRange(4096, 12288)));
    REQUIRE(extent.contains(job::io::JobMemRange(4096, 8192)));
    REQUIRE(extent.contains(job::io::JobMemRange(8192, 12288)));
    REQUIRE(extent.contains(job::io::JobMemRange(6000, 10000)));

    REQUIRE_FALSE(extent.contains(job::io::JobMemRange(0, 4096)));
    REQUIRE_FALSE(extent.contains(job::io::JobMemRange(0, 8192)));
    REQUIRE_FALSE(extent.contains(job::io::JobMemRange(8192, 16384)));
}

TEST_CASE("JobMemExtent identifies shared backing mappings", "[job_io][mem][extent][usage][backing]")
{
    constexpr std::size_t fileSize = 16384;

    const auto firstPath = std::filesystem::temp_directory_path() /
                           ("job_mem_extent_" + std::to_string(::getpid()) + "_backing_first.bin");

    const auto secondPath = std::filesystem::temp_directory_path() /
                            ("job_mem_extent_" + std::to_string(::getpid()) + "_backing_second.bin");

    job::io::JobTmpFile firstTmp(firstPath, fileSize, std::byte{0x00});
    job::io::JobTmpFile secondTmp(secondPath, fileSize, std::byte{0x00});

    const auto firstMap = job::io::JobMmap::createShared(firstPath);
    const auto secondMap = job::io::JobMmap::createShared(secondPath);

    REQUIRE(firstMap);
    REQUIRE(secondMap);

    REQUIRE(firstMap->isValid());
    REQUIRE(secondMap->isValid());

    const job::io::JobMemExtent first(
        firstMap,
        job::io::JobMemRange(0, 4096));

    const job::io::JobMemExtent second(
        firstMap,
        job::io::JobMemRange(4096, 8192));

    const job::io::JobMemExtent other(
        secondMap,
        job::io::JobMemRange(0, 4096));

    REQUIRE(first.sameBacking(second));
    REQUIRE_FALSE(first.sameBacking(other));

    REQUIRE(first.id() != second.id());
    REQUIRE(first.id() != other.id());
    REQUIRE(second.id() != other.id());
}

TEST_CASE("JobMemExtent copies preserve logical extent identity", "[job_io][mem][extent][usage][identity]")
{
    constexpr std::size_t fileSize = 8192;

    const auto path = std::filesystem::temp_directory_path() /
                      ("job_mem_extent_" + std::to_string(::getpid()) + "_identity_copy.bin");

    job::io::JobTmpFile tmp(path, fileSize, std::byte{0x00});

    const auto mmap = job::io::JobMmap::createShared(path);

    REQUIRE(mmap);
    REQUIRE(mmap->isValid());

    const job::io::JobMemExtent original(
        mmap,
        job::io::JobMemRange(0, 4096));

    const job::io::JobMemExtent copy = original;

    REQUIRE(original.id() != job::io::JobMemExtent::kInvalidId);
    REQUIRE(copy.id() == original.id());

    REQUIRE(copy.mmap() == original.mmap());
    REQUIRE(copy.range() == original.range());

    REQUIRE(copy == original);
}

TEST_CASE("JobMemExtent independently created descriptors receive distinct identities", "[job_io][mem][extent][usage][identity]")
{
    constexpr std::size_t fileSize = 8192;

    const auto path = std::filesystem::temp_directory_path() /
                      ("job_mem_extent_" + std::to_string(::getpid()) + "_identity_distinct.bin");

    job::io::JobTmpFile tmp(path, fileSize, std::byte{0x00});

    const auto mmap = job::io::JobMmap::createShared(path);

    REQUIRE(mmap);
    REQUIRE(mmap->isValid());

    const job::io::JobMemRange range(0, 4096);

    const job::io::JobMemExtent first(mmap, range);
    const job::io::JobMemExtent second(mmap, range);

    REQUIRE(first.id() != job::io::JobMemExtent::kInvalidId);
    REQUIRE(second.id() != job::io::JobMemExtent::kInvalidId);

    REQUIRE(first.id() != second.id());

    REQUIRE(first.sameBacking(second));
    REQUIRE(first.range() == second.range());

    REQUIRE_FALSE(first == second);
}

TEST_CASE("JobMemExtent pointer factories create mapped descriptors", "[job_io][mem][extent][usage][factory]")
{
    constexpr std::size_t fileSize = 8192;

    const auto path = std::filesystem::temp_directory_path() /
                      ("job_mem_extent_" + std::to_string(::getpid()) + "_factory.bin");

    job::io::JobTmpFile tmp(path, fileSize, std::byte{0x00});

    const auto mmap = job::io::JobMmap::createShared(path);

    REQUIRE(mmap);
    REQUIRE(mmap->isValid());

    const auto shared = job::io::JobMemExtent::createShared(
        mmap,
        job::io::JobMemRange(0, 4096));

    const auto unique = job::io::JobMemExtent::createUniq(
        mmap,
        job::io::JobMemRange(4096, 8192));

    REQUIRE(shared);
    REQUIRE(unique);

    REQUIRE(shared->id() != job::io::JobMemExtent::kInvalidId);
    REQUIRE(unique->id() != job::io::JobMemExtent::kInvalidId);
    REQUIRE(shared->id() != unique->id());

    REQUIRE(shared->mmap() == mmap);
    REQUIRE(unique->mmap() == mmap);

    REQUIRE(shared->range() == job::io::JobMemRange(0, 4096));
    REQUIRE(unique->range() == job::io::JobMemRange(4096, 8192));
}

//////////////////////////////////////////////////////////
// JobMemExtent edge cases
//////////////////////////////////////////////////////////

TEST_CASE("JobMemExtent can describe the complete mapping", "[job_io][mem][extent][edge][whole]")
{
    constexpr std::size_t fileSize = 8192;

    const auto path = std::filesystem::temp_directory_path() /
                      ("job_mem_extent_" + std::to_string(::getpid()) + "_whole.bin");

    job::io::JobTmpFile tmp(path, fileSize, std::byte{0x00});

    const auto mmap = job::io::JobMmap::createShared(path);

    REQUIRE(mmap);
    REQUIRE(mmap->isValid());

    const job::io::JobMemExtent extent(
        mmap,
        job::io::JobMemRange(0, mmap->mapLength()));

    REQUIRE(extent.id() != job::io::JobMemExtent::kInvalidId);
    REQUIRE(extent.range() == job::io::JobMemRange(0, fileSize));
    REQUIRE(extent.size() == fileSize);
    REQUIRE(extent.addr() == mmap->addr());
}

TEST_CASE("JobMemExtent remains constrained to currently mapped ranges", "[job_io][mem][extent][edge][unmap]")
{
    constexpr std::size_t fileSize = 16384;

    const auto path = std::filesystem::temp_directory_path() /
                      ("job_mem_extent_" + std::to_string(::getpid()) + "_partial_unmap.bin");

    job::io::JobTmpFile tmp(path, fileSize, std::byte{0x00});

    const auto mmap = job::io::JobMmap::createShared(path);

    REQUIRE(mmap);
    REQUIRE(mmap->isValid());

    const std::size_t pageSize = mmap->pageSize();

    REQUIRE(pageSize > 0);
    REQUIRE(fileSize >= pageSize * 3);

    // Remove one complete native page from the middle of the mapping.
    REQUIRE(mmap->unmap(job::io::JobMemRange(pageSize, pageSize * 2)));

    REQUIRE(mmap->mappedRanges().size() == 2);
    REQUIRE(mmap->mappedRanges()[0] == job::io::JobMemRange(0, pageSize));
    REQUIRE(mmap->mappedRanges()[1] == job::io::JobMemRange(pageSize * 2, fileSize));

    // These ranges are still valid JobMemExtent backing candidates.
    const job::io::JobMemExtent first(
        mmap,
        job::io::JobMemRange(0, pageSize));

    const job::io::JobMemExtent second(
        mmap,
        job::io::JobMemRange(pageSize * 2, fileSize));

    REQUIRE(first.id() != job::io::JobMemExtent::kInvalidId);
    REQUIRE(second.id() != job::io::JobMemExtent::kInvalidId);
    REQUIRE(first.id() != second.id());

    REQUIRE(first.size() == pageSize);
    REQUIRE(second.size() == fileSize - pageSize * 2);

    REQUIRE(first.sameBacking(second));
}

//////////////////////////////////////////////////////////
// JobMemPage
//////////////////////////////////////////////////////////

TEST_CASE("JobMemPage describes one allocator page", "[job_io][mem][page]")
{
    const job::io::JobMemPage page(
        10,
        2,
        3,
        4096,
        job::io::JobMemRange(12288, 16384));

    REQUIRE(page.id() == 10);
    REQUIRE(page.extentId() == 2);
    REQUIRE(page.index() == 3);
    REQUIRE(page.pageSize() == 4096);

    REQUIRE(page.range() == job::io::JobMemRange(12288, 16384));
    REQUIRE(page.size() == 4096);

    REQUIRE(page.first() == 12288);
    REQUIRE(page.last() == 16384);

    REQUIRE(page.contains(12288));
    REQUIRE(page.contains(15000));
    REQUIRE_FALSE(page.contains(16384));
}

TEST_CASE("JobMemPage identifies extent membership and adjacency", "[job_io][mem][page]")
{
    const job::io::JobMemPage first(
        10,
        2,
        0,
        4096,
        job::io::JobMemRange(8192, 12288));

    const job::io::JobMemPage second(
        11,
        2,
        1,
        4096,
        job::io::JobMemRange(12288, 16384));

    const job::io::JobMemPage otherExtent(
        12,
        3,
        2,
        4096,
        job::io::JobMemRange(16384, 20480));

    REQUIRE(first.sameExtent(second));
    REQUIRE_FALSE(first.sameExtent(otherExtent));

    REQUIRE(first.adjacent(second));
    REQUIRE_FALSE(first.adjacent(otherExtent));
}

TEST_CASE("JobMemPage pointer factories create descriptors", "[job_io][mem][page]")
{
    const auto shared = job::io::JobMemPage::createShared(
        10,
        2,
        0,
        4096,
        job::io::JobMemRange(8192, 12288));

    const auto unique = job::io::JobMemPage::createUniq(
        11,
        2,
        1,
        4096,
        job::io::JobMemRange(12288, 16384));

    REQUIRE(shared);
    REQUIRE(unique);

    REQUIRE(shared->id() == 10);
    REQUIRE(unique->id() == 11);
}

//////////////////////////////////////////////////////////
// JobMemSpan
//////////////////////////////////////////////////////////

TEST_CASE("JobMemSpan describes contiguous allocator pages", "[job_io][mem][span]")
{
    const job::io::JobMemSpan span(
        20,
        2,
        3,
        4,
        4096,
        job::io::JobMemRange(12288, 28672));

    REQUIRE(span.id() == 20);
    REQUIRE(span.extentId() == 2);

    REQUIRE(span.firstPageIndex() == 3);
    REQUIRE(span.endPageIndex() == 7);
    REQUIRE(span.pageCount() == 4);
    REQUIRE(span.pageSize() == 4096);

    REQUIRE(span.range() == job::io::JobMemRange(12288, 28672));
    REQUIRE(span.size() == 16384);

    REQUIRE(span.first() == 12288);
    REQUIRE(span.last() == 28672);
}

TEST_CASE("JobMemSpan contains page indices", "[job_io][mem][span]")
{
    const job::io::JobMemSpan span(
        20,
        2,
        3,
        4,
        4096,
        job::io::JobMemRange(12288, 28672));

    REQUIRE(span.containsPageIndex(3));
    REQUIRE(span.containsPageIndex(4));
    REQUIRE(span.containsPageIndex(5));
    REQUIRE(span.containsPageIndex(6));

    REQUIRE_FALSE(span.containsPageIndex(2));
    REQUIRE_FALSE(span.containsPageIndex(7));
}

TEST_CASE("JobMemSpan contains matching JobMemPage descriptors", "[job_io][mem][span]")
{
    const job::io::JobMemSpan span(
        20,
        2,
        3,
        4,
        4096,
        job::io::JobMemRange(12288, 28672));

    const job::io::JobMemPage inside(
        10,
        2,
        4,
        4096,
        job::io::JobMemRange(16384, 20480));

    const job::io::JobMemPage wrongExtent(
        11,
        3,
        4,
        4096,
        job::io::JobMemRange(16384, 20480));

    const job::io::JobMemPage outside(
        12,
        2,
        7,
        4096,
        job::io::JobMemRange(28672, 32768));

    REQUIRE(span.contains(inside));
    REQUIRE_FALSE(span.contains(wrongExtent));
    REQUIRE_FALSE(span.contains(outside));
}

TEST_CASE("JobMemSpan detects span adjacency", "[job_io][mem][span]")
{
    const job::io::JobMemSpan first(
        20,
        2,
        0,
        2,
        4096,
        job::io::JobMemRange(0, 8192));

    const job::io::JobMemSpan second(
        21,
        2,
        2,
        2,
        4096,
        job::io::JobMemRange(8192, 16384));

    const job::io::JobMemSpan otherExtent(
        22,
        3,
        2,
        2,
        4096,
        job::io::JobMemRange(8192, 16384));

    REQUIRE(first.sameExtent(second));
    REQUIRE(first.adjacent(second));

    REQUIRE_FALSE(first.sameExtent(otherExtent));
    REQUIRE_FALSE(first.adjacent(otherExtent));
}

TEST_CASE("JobMemSpan contains another span", "[job_io][mem][span]")
{
    const job::io::JobMemSpan outer(
        20,
        2,
        0,
        8,
        4096,
        job::io::JobMemRange(0, 32768));

    const job::io::JobMemSpan inner(
        21,
        2,
        2,
        3,
        4096,
        job::io::JobMemRange(8192, 20480));

    REQUIRE(outer.contains(inner));
    REQUIRE_FALSE(inner.contains(outer));
}

TEST_CASE("JobMemSpan pointer factories create descriptors", "[job_io][mem][span]")
{
    const auto shared = job::io::JobMemSpan::createShared(
        20,
        2,
        0,
        2,
        4096,
        job::io::JobMemRange(0, 8192));

    const auto unique = job::io::JobMemSpan::createUniq(
        21,
        2,
        2,
        2,
        4096,
        job::io::JobMemRange(8192, 16384));

    REQUIRE(shared);
    REQUIRE(unique);

    REQUIRE(shared->id() == 20);
    REQUIRE(unique->id() == 21);
}

//////////////////////////////////////////////////////////
// JobMemSize
//////////////////////////////////////////////////////////

TEST_CASE("JobMemSize describes a fixed size class", "[job_io][mem][size]")
{
    const job::io::JobMemSize sizeClass(
        1,
        64,
        16,
        4096,
        4);

    REQUIRE(sizeClass.id() == 1);
    REQUIRE(sizeClass.objectSize() == 64);
    REQUIRE(sizeClass.alignment() == 16);
    REQUIRE(sizeClass.pageSize() == 4096);
    REQUIRE(sizeClass.pagesPerSpan() == 4);

    REQUIRE(sizeClass.spanSize() == 16384);
    REQUIRE(sizeClass.objectsPerPage() == 64);
    REQUIRE(sizeClass.objectsPerSpan() == 256);

    REQUIRE(sizeClass.wastePerPage() == 0);
    REQUIRE(sizeClass.wastePerSpan() == 0);

    REQUIRE(sizeClass.fitsInPage());
    REQUIRE(sizeClass.fitsInSpan());
}

TEST_CASE("JobMemSize reports size class waste", "[job_io][mem][size]")
{
    const job::io::JobMemSize sizeClass(
        2,
        96,
        32,
        4096,
        3);

    REQUIRE(sizeClass.spanSize() == 12288);

    REQUIRE(sizeClass.objectsPerPage() == 42);
    REQUIRE(sizeClass.objectsPerSpan() == 128);

    REQUIRE(sizeClass.wastePerPage() == 64);
    REQUIRE(sizeClass.wastePerSpan() == 0);

    REQUIRE(sizeClass.fitsInPage());
    REQUIRE(sizeClass.fitsInSpan());
}

TEST_CASE("JobMemSize supports objects larger than one allocator page", "[job_io][mem][size]")
{
    const job::io::JobMemSize sizeClass(
        3,
        8192,
        4096,
        4096,
        4);

    REQUIRE(sizeClass.spanSize() == 16384);

    REQUIRE(sizeClass.objectsPerPage() == 0);
    REQUIRE(sizeClass.objectsPerSpan() == 2);

    REQUIRE_FALSE(sizeClass.fitsInPage());
    REQUIRE(sizeClass.fitsInSpan());
}

TEST_CASE("JobMemSize validates power of two alignment", "[job_io][mem][size]")
{
    REQUIRE(job::io::JobMemSize::validAlignment(1));
    REQUIRE(job::io::JobMemSize::validAlignment(16));
    REQUIRE(job::io::JobMemSize::validAlignment(4096));

    REQUIRE_FALSE(job::io::JobMemSize::validAlignment(0));
    REQUIRE_FALSE(job::io::JobMemSize::validAlignment(3));
    REQUIRE_FALSE(job::io::JobMemSize::validAlignment(24));
}

TEST_CASE("JobMemSize pointer factories create descriptors", "[job_io][mem][size]")
{
    const auto shared = job::io::JobMemSize::createShared(
        1,
        64,
        16,
        4096,
        4);

    const auto unique = job::io::JobMemSize::createUniq(
        2,
        128,
        32,
        4096,
        2);

    REQUIRE(shared);
    REQUIRE(unique);

    REQUIRE(shared->id() == 1);
    REQUIRE(unique->id() == 2);
}