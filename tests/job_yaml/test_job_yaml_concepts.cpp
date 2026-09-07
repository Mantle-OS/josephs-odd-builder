#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <job_yaml_concepts.h>

namespace job::yaml::tests {

struct ParseDestination
{
    bool null()
    {
        return true;
    }

    bool scalar(std::string_view)
    {
        return true;
    }

    bool scalarOwned(std::string)
    {
        return true;
    }

    bool key(std::string_view)
    {
        return true;
    }

    bool keyOwned(std::string)
    {
        return true;
    }

    bool beginMapping()
    {
        return true;
    }

    bool endMapping()
    {
        return true;
    }

    bool beginSequence()
    {
        return true;
    }

    bool endSequence()
    {
        return true;
    }
};

struct VoidParseDestination
{
    void null() {}

    void scalar(std::string_view) {}
    void scalarOwned(std::string) {}

    void key(std::string_view) {}
    void keyOwned(std::string) {}

    void beginMapping() {}
    void endMapping() {}

    void beginSequence() {}
    void endSequence() {}
};

struct MissingNullDestination
{
    bool scalar(std::string_view)
    {
        return true;
    }

    bool scalarOwned(std::string)
    {
        return true;
    }

    bool key(std::string_view)
    {
        return true;
    }

    bool keyOwned(std::string)
    {
        return true;
    }

    bool beginMapping()
    {
        return true;
    }

    bool endMapping()
    {
        return true;
    }

    bool beginSequence()
    {
        return true;
    }

    bool endSequence()
    {
        return true;
    }
};

struct MissingScalarDestination
{
    bool null()
    {
        return true;
    }

    bool scalarOwned(std::string)
    {
        return true;
    }

    bool key(std::string_view)
    {
        return true;
    }

    bool keyOwned(std::string)
    {
        return true;
    }

    bool beginMapping()
    {
        return true;
    }

    bool endMapping()
    {
        return true;
    }

    bool beginSequence()
    {
        return true;
    }

    bool endSequence()
    {
        return true;
    }
};

struct MissingOwnedScalarDestination
{
    bool null()
    {
        return true;
    }

    bool scalar(std::string_view)
    {
        return true;
    }

    bool key(std::string_view)
    {
        return true;
    }

    bool keyOwned(std::string)
    {
        return true;
    }

    bool beginMapping()
    {
        return true;
    }

    bool endMapping()
    {
        return true;
    }

    bool beginSequence()
    {
        return true;
    }

    bool endSequence()
    {
        return true;
    }
};

struct MissingKeyDestination
{
    bool null()
    {
        return true;
    }

    bool scalar(std::string_view)
    {
        return true;
    }

    bool scalarOwned(std::string)
    {
        return true;
    }

    bool keyOwned(std::string)
    {
        return true;
    }

    bool beginMapping()
    {
        return true;
    }

    bool endMapping()
    {
        return true;
    }

    bool beginSequence()
    {
        return true;
    }

    bool endSequence()
    {
        return true;
    }
};

struct MissingOwnedKeyDestination
{
    bool null()
    {
        return true;
    }

    bool scalar(std::string_view)
    {
        return true;
    }

    bool scalarOwned(std::string)
    {
        return true;
    }

    bool key(std::string_view)
    {
        return true;
    }

    bool beginMapping()
    {
        return true;
    }

    bool endMapping()
    {
        return true;
    }

    bool beginSequence()
    {
        return true;
    }

    bool endSequence()
    {
        return true;
    }
};

struct MissingMappingDestination
{
    bool null()
    {
        return true;
    }

    bool scalar(std::string_view)
    {
        return true;
    }

    bool scalarOwned(std::string)
    {
        return true;
    }

    bool key(std::string_view)
    {
        return true;
    }

    bool keyOwned(std::string)
    {
        return true;
    }

    bool beginSequence()
    {
        return true;
    }

    bool endSequence()
    {
        return true;
    }
};

struct MissingSequenceDestination
{
    bool null()
    {
        return true;
    }

    bool scalar(std::string_view)
    {
        return true;
    }

    bool scalarOwned(std::string)
    {
        return true;
    }

    bool key(std::string_view)
    {
        return true;
    }

    bool keyOwned(std::string)
    {
        return true;
    }

    bool beginMapping()
    {
        return true;
    }

    bool endMapping()
    {
        return true;
    }
};

struct ExistingEventDestination
{
    void scalar(std::string_view) {}
    void member(std::string_view, std::string_view) {}
    void append(std::string_view) {}
};

struct ExistingNodeDestination
{
    void setNull() {}
    void setScalar(std::string_view) {}
    void setMapping() {}
    void setSequence() {}

    void setMember(std::string_view, ExistingNodeDestination) {}
    void setMemberScalar(std::string_view, std::string_view) {}

    void append(ExistingNodeDestination) {}
    void appendScalar(std::string_view) {}
};

struct CustomDeleter
{
    void operator()(int *ptr) const
    {
        delete ptr;
    }
};
// ========================================
// YamlOptional
// ========================================

static_assert(YamlOptional<std::optional<int>>);
static_assert(YamlOptional<const std::optional<int> &>);
static_assert(!YamlOptional<int>);
static_assert(!YamlOptional<std::shared_ptr<int>>);

TEST_CASE("YamlOptional recognizes std::optional",
          "[job_yaml][concepts][optional]")
{
    REQUIRE(YamlOptional<std::optional<int>>);
    REQUIRE(YamlOptional<const std::optional<int> &>);
    REQUIRE_FALSE(YamlOptional<int>);
    REQUIRE_FALSE(YamlOptional<std::shared_ptr<int>>);
}

// ========================================
// YamlSharedPointer
// ========================================

static_assert(YamlSharedPointer<std::shared_ptr<int>>);
static_assert(YamlSharedPointer<const std::shared_ptr<int> &>);
static_assert(!YamlSharedPointer<int *>);
static_assert(!YamlSharedPointer<std::unique_ptr<int>>);

TEST_CASE("YamlSharedPointer recognizes std::shared_ptr",
          "[job_yaml][concepts][pointer][shared_pointer]")
{
    REQUIRE(YamlSharedPointer<std::shared_ptr<int>>);
    REQUIRE(YamlSharedPointer<const std::shared_ptr<int> &>);
    REQUIRE_FALSE(YamlSharedPointer<int *>);
    REQUIRE_FALSE(YamlSharedPointer<std::unique_ptr<int>>);
}

// ========================================
// YamlUniquePointer
// ========================================

static_assert(YamlUniquePointer<std::unique_ptr<int>>);
static_assert(YamlUniquePointer<const std::unique_ptr<int> &>);
static_assert(YamlUniquePointer<std::unique_ptr<int, CustomDeleter>>);
static_assert(!YamlUniquePointer<int *>);
static_assert(!YamlUniquePointer<std::shared_ptr<int>>);

TEST_CASE("YamlUniquePointer recognizes std::unique_ptr including custom deleters",
          "[job_yaml][concepts][pointer][unique_pointer]")
{
    REQUIRE(YamlUniquePointer<std::unique_ptr<int>>);
    REQUIRE(YamlUniquePointer<const std::unique_ptr<int> &>);
    REQUIRE(YamlUniquePointer<std::unique_ptr<int, CustomDeleter>>);
    REQUIRE_FALSE(YamlUniquePointer<int *>);
    REQUIRE_FALSE(YamlUniquePointer<std::shared_ptr<int>>);
}

// ========================================
// YamlPointer
// ========================================

static_assert(YamlPointer<std::shared_ptr<int>>);
static_assert(YamlPointer<std::unique_ptr<int>>);
static_assert(YamlPointer<std::unique_ptr<int, CustomDeleter>>);

static_assert(!YamlPointer<int *>);
static_assert(!YamlPointer<std::weak_ptr<int>>);
static_assert(!YamlPointer<int>);

TEST_CASE("YamlPointer recognizes supported owning pointer types only",
          "[job_yaml][concepts][pointer]")
{
    REQUIRE(YamlPointer<std::shared_ptr<int>>);
    REQUIRE(YamlPointer<std::unique_ptr<int>>);
    REQUIRE(YamlPointer<std::unique_ptr<int, CustomDeleter>>);

    REQUIRE_FALSE(YamlPointer<int *>);
    REQUIRE_FALSE(YamlPointer<std::weak_ptr<int>>);
    REQUIRE_FALSE(YamlPointer<int>);
}

// ========================================
// YamlBasicValue
// ========================================

static_assert(YamlBasicValue<int>);
static_assert(YamlBasicValue<std::string>);
static_assert(YamlBasicValue<std::optional<int>>);
static_assert(YamlBasicValue<std::shared_ptr<int>>);
static_assert(YamlBasicValue<std::unique_ptr<int>>);

TEST_CASE("YamlBasicValue includes optional and supported pointer categories",
          "[job_yaml][concepts][basic_value]")
{
    REQUIRE(YamlBasicValue<int>);
    REQUIRE(YamlBasicValue<std::string>);
    REQUIRE(YamlBasicValue<std::optional<int>>);
    REQUIRE(YamlBasicValue<std::shared_ptr<int>>);
    REQUIRE(YamlBasicValue<std::unique_ptr<int>>);
}

// ========================================
// YamlParseDestination
// ========================================

static_assert(YamlParseDestination<ParseDestination>);

static_assert(!YamlParseDestination<VoidParseDestination>);
static_assert(!YamlParseDestination<MissingNullDestination>);
static_assert(!YamlParseDestination<MissingScalarDestination>);
static_assert(!YamlParseDestination<MissingOwnedScalarDestination>);
static_assert(!YamlParseDestination<MissingKeyDestination>);
static_assert(!YamlParseDestination<MissingOwnedKeyDestination>);
static_assert(!YamlParseDestination<MissingMappingDestination>);
static_assert(!YamlParseDestination<MissingSequenceDestination>);

TEST_CASE("YamlParseDestination accepts the complete structural destination shape",
          "[job_yaml][concepts][parse_destination]")
{
    REQUIRE(YamlParseDestination<ParseDestination>);
}

TEST_CASE("YamlParseDestination requires boolean-compatible destination results",
          "[job_yaml][concepts][parse_destination]")
{
    REQUIRE_FALSE(YamlParseDestination<VoidParseDestination>);
}

TEST_CASE("YamlParseDestination requires borrowed scalar routing",
          "[job_yaml][concepts][parse_destination]")
{
    REQUIRE_FALSE(YamlParseDestination<MissingScalarDestination>);
}

TEST_CASE("YamlParseDestination requires owned scalar routing",
          "[job_yaml][concepts][parse_destination]")
{
    REQUIRE_FALSE(YamlParseDestination<MissingOwnedScalarDestination>);
}

TEST_CASE("YamlParseDestination requires borrowed key routing",
          "[job_yaml][concepts][parse_destination]")
{
    REQUIRE_FALSE(YamlParseDestination<MissingKeyDestination>);
}

TEST_CASE("YamlParseDestination requires owned key routing",
          "[job_yaml][concepts][parse_destination]")
{
    REQUIRE_FALSE(YamlParseDestination<MissingOwnedKeyDestination>);
}

TEST_CASE("YamlParseDestination requires mapping structural boundaries",
          "[job_yaml][concepts][parse_destination]")
{
    REQUIRE_FALSE(YamlParseDestination<MissingMappingDestination>);
}

TEST_CASE("YamlParseDestination requires sequence structural boundaries",
          "[job_yaml][concepts][parse_destination]")
{
    REQUIRE_FALSE(YamlParseDestination<MissingSequenceDestination>);
}

// ========================================
// Existing destination concepts remain distinct
// ========================================

static_assert(YamlEventDestination<ExistingEventDestination>);
static_assert(!YamlParseDestination<ExistingEventDestination>);

static_assert(YamlNodeDestination<ExistingNodeDestination>);
static_assert(!YamlParseDestination<ExistingNodeDestination>);

TEST_CASE("YamlEventDestination does not imply YamlParseDestination",
          "[job_yaml][concepts][parse_destination][event_destination]")
{
    REQUIRE(YamlEventDestination<ExistingEventDestination>);
    REQUIRE_FALSE(YamlParseDestination<ExistingEventDestination>);
}

TEST_CASE("YamlNodeDestination does not imply YamlParseDestination",
          "[job_yaml][concepts][parse_destination][node_destination]")
{
    REQUIRE(YamlNodeDestination<ExistingNodeDestination>);
    REQUIRE_FALSE(YamlParseDestination<ExistingNodeDestination>);
}

} // namespace job::yaml::tests