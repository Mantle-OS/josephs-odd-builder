#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <sys/mman.h>
#include <utility>

#include <job_mem_lock.h>
#include <job_mem_page.h>

namespace job::io::test {

class TestMemoryMapping final
{
public:
    explicit TestMemoryMapping(std::size_t size) :
        m_size(size)
    {
        m_addr = ::mmap(nullptr, m_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (m_addr == MAP_FAILED)
            m_addr = nullptr;
    }

    ~TestMemoryMapping()
    {
        if (m_addr != nullptr)
            ::munmap(m_addr, m_size);
    }

    TestMemoryMapping(const TestMemoryMapping &) = delete;
    TestMemoryMapping &operator=(const TestMemoryMapping &) = delete;
    TestMemoryMapping(TestMemoryMapping &&) = delete;
    TestMemoryMapping &operator=(TestMemoryMapping &&) = delete;

    [[nodiscard]] void *addr() noexcept
    {
        return m_addr;
    }

    [[nodiscard]] const void *addr() const noexcept
    {
        return m_addr;
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return m_size;
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return m_addr != nullptr;
    }

private:
    void *m_addr{nullptr};
    std::size_t m_size{0};
};

} // namespace job::io::test

//////////////////////////////////////////////////////////
// Block 1: Usage / examples
//////////////////////////////////////////////////////////

TEST_CASE("JobMemLock locks and unlocks a memory mapping", "[job_io][mem_lock][usage]")
{
    const std::size_t pageSize = job::io::systemPageSize();
    REQUIRE(pageSize > 0);

    job::io::test::TestMemoryMapping mapping(pageSize * 2);

    REQUIRE(mapping.valid());

    job::io::JobMemLock lock(mapping.addr(), mapping.size(), pageSize);

    REQUIRE(lock.valid());
    REQUIRE(lock.addr() == mapping.addr());
    REQUIRE(lock.size() == mapping.size());
    REQUIRE(lock.pageSize() == pageSize);
    REQUIRE_FALSE(lock.isLocked());

    REQUIRE(lock.lock());

    REQUIRE(lock.isLocked());
    REQUIRE(lock.lockedSize() == mapping.size());
    REQUIRE(lock.lockedRanges().size() == 1);
    REQUIRE(lock.lockedRanges().front() == job::io::JobMemRange{0, mapping.size()});

    REQUIRE(lock.unlock());

    REQUIRE_FALSE(lock.isLocked());
    REQUIRE(lock.lockedSize() == 0);
    REQUIRE(lock.lockedRanges().empty());
}

TEST_CASE("JobMemLock locks a selected memory range", "[job_io][mem_lock][usage][range]")
{
    const std::size_t pageSize = job::io::systemPageSize();
    REQUIRE(pageSize > 0);

    job::io::test::TestMemoryMapping mapping(pageSize * 4);

    REQUIRE(mapping.valid());

    job::io::JobMemLock lock(mapping.addr(), mapping.size(), pageSize);

    const job::io::JobMemRange range{pageSize, pageSize * 3};

    REQUIRE(lock.lock(range));

    REQUIRE_FALSE(lock.isLocked(0));
    REQUIRE(lock.isLocked(pageSize));
    REQUIRE(lock.isLocked(pageSize * 2));
    REQUIRE_FALSE(lock.isLocked(pageSize * 3));

    REQUIRE(lock.isLocked(range));
    REQUIRE(lock.lockedSize() == pageSize * 2);

    REQUIRE(lock.unlock(range));

    REQUIRE_FALSE(lock.isLocked());
}

TEST_CASE("JobMemLock expands unaligned ranges to native pages", "[job_io][mem_lock][usage][alignment]")
{
    const std::size_t pageSize = job::io::systemPageSize();
    REQUIRE(pageSize > 0);

    job::io::test::TestMemoryMapping mapping(pageSize * 3);

    REQUIRE(mapping.valid());

    job::io::JobMemLock lock(mapping.addr(), mapping.size(), pageSize);

    // This touches bytes in the first and second pages, so both pages
    // must be locked by the native mlock operation.
    const job::io::JobMemRange requested{1, pageSize + 1};

    REQUIRE(lock.lock(requested));

    REQUIRE(lock.lockedRanges().size() == 1);
    REQUIRE(lock.lockedRanges().front() == job::io::JobMemRange{0, pageSize * 2});
    REQUIRE(lock.lockedSize() == pageSize * 2);

    REQUIRE(lock.isLocked(0));
    REQUIRE(lock.isLocked(pageSize));
    REQUIRE(lock.isLocked(pageSize * 2 - 1));
    REQUIRE_FALSE(lock.isLocked(pageSize * 2));

    REQUIRE(lock.unlock(requested));
    REQUIRE_FALSE(lock.isLocked());
}

TEST_CASE("JobMemLock merges adjacent locked ranges", "[job_io][mem_lock][usage][ranges]")
{
    const std::size_t pageSize = job::io::systemPageSize();
    REQUIRE(pageSize > 0);

    job::io::test::TestMemoryMapping mapping(pageSize * 4);

    REQUIRE(mapping.valid());

    job::io::JobMemLock lock(mapping.addr(), mapping.size(), pageSize);

    REQUIRE(lock.lock(job::io::JobMemRange{0, pageSize}));
    REQUIRE(lock.lock(job::io::JobMemRange{pageSize, pageSize * 2}));

    REQUIRE(lock.lockedRanges().size() == 1);
    REQUIRE(lock.lockedRanges().front() == job::io::JobMemRange{0, pageSize * 2});
    REQUIRE(lock.lockedSize() == pageSize * 2);

    REQUIRE(lock.unlock());
}

TEST_CASE("JobMemLock merges overlapping locked ranges", "[job_io][mem_lock][usage][ranges]")
{
    const std::size_t pageSize = job::io::systemPageSize();
    REQUIRE(pageSize > 0);

    job::io::test::TestMemoryMapping mapping(pageSize * 4);

    REQUIRE(mapping.valid());

    job::io::JobMemLock lock(mapping.addr(), mapping.size(), pageSize);

    REQUIRE(lock.lock(job::io::JobMemRange{0, pageSize * 2}));
    REQUIRE(lock.lock(job::io::JobMemRange{pageSize, pageSize * 3}));

    REQUIRE(lock.lockedRanges().size() == 1);
    REQUIRE(lock.lockedRanges().front() == job::io::JobMemRange{0, pageSize * 3});
    REQUIRE(lock.lockedSize() == pageSize * 3);

    REQUIRE(lock.unlock());
}

TEST_CASE("JobMemLock partial unlock preserves the remaining locked ranges", "[job_io][mem_lock][usage][partial_unlock]")
{
    const std::size_t pageSize = job::io::systemPageSize();
    REQUIRE(pageSize > 0);

    job::io::test::TestMemoryMapping mapping(pageSize * 4);

    REQUIRE(mapping.valid());

    job::io::JobMemLock lock(mapping.addr(), mapping.size(), pageSize);

    REQUIRE(lock.lock());

    const job::io::JobMemRange middle{pageSize, pageSize * 3};

    REQUIRE(lock.unlock(middle));

    REQUIRE(lock.isLocked());
    REQUIRE(lock.lockedSize() == pageSize * 2);
    REQUIRE(lock.lockedRanges().size() == 2);

    REQUIRE(lock.lockedRanges()[0] == job::io::JobMemRange{0, pageSize});
    REQUIRE(lock.lockedRanges()[1] == job::io::JobMemRange{pageSize * 3, pageSize * 4});

    REQUIRE(lock.isLocked(0));
    REQUIRE_FALSE(lock.isLocked(pageSize));
    REQUIRE_FALSE(lock.isLocked(pageSize * 2));
    REQUIRE(lock.isLocked(pageSize * 3));

    REQUIRE(lock.unlock());
}

TEST_CASE("JobMemLock can be rebound to a new memory mapping", "[job_io][mem_lock][usage][reset]")
{
    const std::size_t pageSize = job::io::systemPageSize();
    REQUIRE(pageSize > 0);

    job::io::test::TestMemoryMapping first(pageSize * 2);
    job::io::test::TestMemoryMapping second(pageSize * 3);

    REQUIRE(first.valid());
    REQUIRE(second.valid());

    job::io::JobMemLock lock(first.addr(), first.size(), pageSize);

    REQUIRE(lock.lock());
    REQUIRE(lock.lockedSize() == first.size());

    REQUIRE(lock.reset(second.addr(), second.size(), pageSize));

    REQUIRE(lock.addr() == second.addr());
    REQUIRE(lock.size() == second.size());
    REQUIRE(lock.pageSize() == pageSize);
    REQUIRE_FALSE(lock.isLocked());

    REQUIRE(lock.lock(job::io::JobMemRange{0, pageSize}));
    REQUIRE(lock.lockedSize() == pageSize);

    REQUIRE(lock.unlock());
}

TEST_CASE("JobMemLock move construction transfers locked memory ownership", "[job_io][mem_lock][usage][move]")
{
    const std::size_t pageSize = job::io::systemPageSize();
    REQUIRE(pageSize > 0);

    job::io::test::TestMemoryMapping mapping(pageSize * 2);

    REQUIRE(mapping.valid());

    job::io::JobMemLock source(mapping.addr(), mapping.size(), pageSize);

    REQUIRE(source.lock());
    REQUIRE(source.isLocked());

    job::io::JobMemLock destination(std::move(source));

    REQUIRE(destination.valid());
    REQUIRE(destination.addr() == mapping.addr());
    REQUIRE(destination.size() == mapping.size());
    REQUIRE(destination.isLocked());
    REQUIRE(destination.lockedSize() == mapping.size());

    REQUIRE_FALSE(source.valid());
    REQUIRE(source.addr() == nullptr);
    REQUIRE(source.size() == 0);
    REQUIRE_FALSE(source.isLocked());

    REQUIRE(destination.unlock());
}

TEST_CASE("JobMemLock factories create memory lock resources", "[job_io][mem_lock][usage][factory]")
{
    const std::size_t pageSize = job::io::systemPageSize();
    REQUIRE(pageSize > 0);

    job::io::test::TestMemoryMapping sharedMapping(pageSize);
    job::io::test::TestMemoryMapping uniqueMapping(pageSize);

    REQUIRE(sharedMapping.valid());
    REQUIRE(uniqueMapping.valid());

    const auto shared = job::io::JobMemLock::createShared(sharedMapping.addr(), sharedMapping.size(), pageSize);
    const auto unique = job::io::JobMemLock::createUniq(uniqueMapping.addr(), uniqueMapping.size(), pageSize);

    REQUIRE(shared);
    REQUIRE(unique);

    REQUIRE(shared->valid());
    REQUIRE(unique->valid());

    REQUIRE(shared->lock());
    REQUIRE(unique->lock());

    REQUIRE(shared->isLocked());
    REQUIRE(unique->isLocked());

    REQUIRE(shared->unlock());
    REQUIRE(unique->unlock());
}

//////////////////////////////////////////////////////////
// Block 2: Edge cases / failure behavior
//////////////////////////////////////////////////////////

TEST_CASE("JobMemLock default object has no memory domain", "[job_io][mem_lock][edge]")
{
    job::io::JobMemLock lock;

    REQUIRE_FALSE(lock.valid());
    REQUIRE(lock.addr() == nullptr);
    REQUIRE(lock.size() == 0);
    REQUIRE(lock.pageSize() == 0);
    REQUIRE_FALSE(lock.isLocked());
    REQUIRE(lock.lockedSize() == 0);
    REQUIRE(lock.lockedRanges().empty());

    REQUIRE_FALSE(lock.lock());
    REQUIRE_FALSE(lock.unlock());
}

TEST_CASE("JobMemLock rejects empty ranges", "[job_io][mem_lock][edge][empty]")
{
    const std::size_t pageSize = job::io::systemPageSize();
    REQUIRE(pageSize > 0);

    job::io::test::TestMemoryMapping mapping(pageSize * 2);

    REQUIRE(mapping.valid());

    job::io::JobMemLock lock(mapping.addr(), mapping.size(), pageSize);

    const job::io::JobMemRange empty{pageSize, pageSize};

    REQUIRE_FALSE(lock.lock(empty));
    REQUIRE_FALSE(lock.unlock(empty));
    REQUIRE_FALSE(lock.isLocked(empty));

    REQUIRE_FALSE(lock.isLocked());
}

TEST_CASE("JobMemLock rejects ranges outside its memory domain", "[job_io][mem_lock][edge][range]")
{
    const std::size_t pageSize = job::io::systemPageSize();
    REQUIRE(pageSize > 0);

    job::io::test::TestMemoryMapping mapping(pageSize * 2);

    REQUIRE(mapping.valid());

    job::io::JobMemLock lock(mapping.addr(), mapping.size(), pageSize);

    const job::io::JobMemRange outside{pageSize, pageSize * 3};

    REQUIRE_FALSE(lock.lock(outside));
    REQUIRE_FALSE(lock.unlock(outside));
    REQUIRE_FALSE(lock.isLocked(outside));

    REQUIRE_FALSE(lock.isLocked(mapping.size()));
}

TEST_CASE("JobMemLock locking an already locked range is idempotent", "[job_io][mem_lock][edge][idempotent]")
{
    const std::size_t pageSize = job::io::systemPageSize();
    REQUIRE(pageSize > 0);

    job::io::test::TestMemoryMapping mapping(pageSize * 2);

    REQUIRE(mapping.valid());

    job::io::JobMemLock lock(mapping.addr(), mapping.size(), pageSize);

    const job::io::JobMemRange range{0, pageSize};

    REQUIRE(lock.lock(range));
    REQUIRE(lock.lock(range));
    REQUIRE(lock.lock(range));

    REQUIRE(lock.lockedRanges().size() == 1);
    REQUIRE(lock.lockedRanges().front() == range);
    REQUIRE(lock.lockedSize() == pageSize);

    REQUIRE(lock.unlock());
}

TEST_CASE("JobMemLock unlocking an unlocked range is idempotent", "[job_io][mem_lock][edge][idempotent]")
{
    const std::size_t pageSize = job::io::systemPageSize();
    REQUIRE(pageSize > 0);

    job::io::test::TestMemoryMapping mapping(pageSize * 2);

    REQUIRE(mapping.valid());

    job::io::JobMemLock lock(mapping.addr(), mapping.size(), pageSize);

    REQUIRE(lock.unlock(job::io::JobMemRange{0, pageSize}));
    REQUIRE_FALSE(lock.isLocked());

    REQUIRE(lock.lock(job::io::JobMemRange{pageSize, pageSize * 2}));

    // First page was never locked by JobMemLock.
    REQUIRE(lock.unlock(job::io::JobMemRange{0, pageSize}));

    REQUIRE(lock.isLocked());
    REQUIRE(lock.lockedRanges().size() == 1);
    REQUIRE(lock.lockedRanges().front() == job::io::JobMemRange{pageSize, pageSize * 2});

    REQUIRE(lock.unlock());
}

TEST_CASE("JobMemLock unlock can trim the beginning of a locked range", "[job_io][mem_lock][edge][partial_unlock]")
{
    const std::size_t pageSize = job::io::systemPageSize();
    REQUIRE(pageSize > 0);

    job::io::test::TestMemoryMapping mapping(pageSize * 3);

    REQUIRE(mapping.valid());

    job::io::JobMemLock lock(mapping.addr(), mapping.size(), pageSize);

    REQUIRE(lock.lock());

    REQUIRE(lock.unlock(job::io::JobMemRange{0, pageSize}));

    REQUIRE(lock.lockedRanges().size() == 1);
    REQUIRE(lock.lockedRanges().front() == job::io::JobMemRange{pageSize, pageSize * 3});
    REQUIRE(lock.lockedSize() == pageSize * 2);

    REQUIRE(lock.unlock());
}

TEST_CASE("JobMemLock unlock can trim the end of a locked range", "[job_io][mem_lock][edge][partial_unlock]")
{
    const std::size_t pageSize = job::io::systemPageSize();
    REQUIRE(pageSize > 0);

    job::io::test::TestMemoryMapping mapping(pageSize * 3);

    REQUIRE(mapping.valid());

    job::io::JobMemLock lock(mapping.addr(), mapping.size(), pageSize);

    REQUIRE(lock.lock());

    REQUIRE(lock.unlock(job::io::JobMemRange{pageSize * 2, pageSize * 3}));

    REQUIRE(lock.lockedRanges().size() == 1);
    REQUIRE(lock.lockedRanges().front() == job::io::JobMemRange{0, pageSize * 2});
    REQUIRE(lock.lockedSize() == pageSize * 2);

    REQUIRE(lock.unlock());
}

TEST_CASE("JobMemLock clear releases locks and removes its memory domain", "[job_io][mem_lock][edge][clear]")
{
    const std::size_t pageSize = job::io::systemPageSize();
    REQUIRE(pageSize > 0);

    job::io::test::TestMemoryMapping mapping(pageSize * 2);

    REQUIRE(mapping.valid());

    job::io::JobMemLock lock(mapping.addr(), mapping.size(), pageSize);

    REQUIRE(lock.lock());
    REQUIRE(lock.isLocked());

    REQUIRE(lock.clear());

    REQUIRE_FALSE(lock.valid());
    REQUIRE(lock.addr() == nullptr);
    REQUIRE(lock.size() == 0);
    REQUIRE(lock.pageSize() == 0);
    REQUIRE_FALSE(lock.isLocked());
    REQUIRE(lock.lockedRanges().empty());
}

TEST_CASE("JobMemLock move assignment releases the destination and transfers the source", "[job_io][mem_lock][edge][move]")
{
    const std::size_t pageSize = job::io::systemPageSize();
    REQUIRE(pageSize > 0);

    job::io::test::TestMemoryMapping sourceMapping(pageSize);
    job::io::test::TestMemoryMapping destinationMapping(pageSize);

    REQUIRE(sourceMapping.valid());
    REQUIRE(destinationMapping.valid());

    job::io::JobMemLock source(sourceMapping.addr(), sourceMapping.size(), pageSize);
    job::io::JobMemLock destination(destinationMapping.addr(), destinationMapping.size(), pageSize);

    REQUIRE(source.lock());
    REQUIRE(destination.lock());

    destination = std::move(source);

    REQUIRE(destination.valid());
    REQUIRE(destination.addr() == sourceMapping.addr());
    REQUIRE(destination.size() == sourceMapping.size());
    REQUIRE(destination.isLocked());
    REQUIRE(destination.lockedSize() == pageSize);

    REQUIRE_FALSE(source.valid());
    REQUIRE(source.addr() == nullptr);
    REQUIRE(source.size() == 0);
    REQUIRE_FALSE(source.isLocked());

    REQUIRE(destination.unlock());
}