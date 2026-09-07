#include <catch2/catch_test_macros.hpp>

#include <memory>

#include <job_schema.h>
#include <job_schema_concepts.h>
#include <job_schema_types.h>

using namespace job::schema;

namespace {

class TestSchema final : public job::schema::JobSchema
{
public:
    using Ptr  = std::shared_ptr<TestSchema>;
    using WPtr = std::weak_ptr<TestSchema>;
    using UPtr = std::unique_ptr<TestSchema>;

    TestSchema() = default;
    ~TestSchema() = default;

    TestSchema(const TestSchema &) = delete;
    TestSchema &operator=(const TestSchema &) = delete;
    TestSchema(TestSchema &&) noexcept = default;
    TestSchema &operator=(TestSchema &&) noexcept = default;

    static Ptr createShared()
    {
        return std::make_shared<TestSchema>();
    }

    static UPtr createUniq()
    {
        return std::make_unique<TestSchema>();
    }

    bin32 m_hash{};
};

} // namespace

static_assert(SchemaType<TestSchema>);

static_assert(RawPointerType<TestSchema *>);
static_assert(!RawPointerType<TestSchema>);

static_assert(SharedPointerType<std::shared_ptr<TestSchema>>);
static_assert(!SharedPointerType<std::unique_ptr<TestSchema>>);

static_assert(UniquePointerType<std::unique_ptr<TestSchema>>);
static_assert(!UniquePointerType<std::shared_ptr<TestSchema>>);

static_assert(SharedFactoryType<TestSchema>);
static_assert(UniqueFactoryType<TestSchema>);

TEST_CASE("JobSchema basic construction", "[job_schema][concepts]")
{
    auto shared = TestSchema::createShared();
    auto unique = TestSchema::createUniq();

    REQUIRE(shared != nullptr);
    REQUIRE(unique != nullptr);
}