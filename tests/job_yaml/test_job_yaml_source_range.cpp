#include <catch2/catch_test_macros.hpp>

#include <job_yaml_source_range.h>

#include <string_view>

using namespace std::string_view_literals;

namespace job::yaml::tests {

TEST_CASE("YamlSourceRange default constructs empty", "[job_yaml][source_range]")
{
    const YamlSourceRange range;

    REQUIRE(range.offset == 0);
    REQUIRE(range.size == 0);
    REQUIRE(range.endOffset() == 0);
    REQUIRE(range.empty());
}

TEST_CASE("YamlSourceRange reports end offset", "[job_yaml][source_range]")
{
    const YamlSourceRange range{
        .offset = 6,
        .size = 5
    };

    REQUIRE(range.endOffset() == 11);
    REQUIRE_FALSE(range.empty());
}

TEST_CASE("YamlSourceRange contains offsets inside range", "[job_yaml][source_range]")
{
    const YamlSourceRange range{
        .offset = 4,
        .size = 3
    };

    REQUIRE_FALSE(range.contains(3));
    REQUIRE(range.contains(4));
    REQUIRE(range.contains(5));
    REQUIRE(range.contains(6));
    REQUIRE_FALSE(range.contains(7));
}

TEST_CASE("YamlSourceRange empty range contains no offsets", "[job_yaml][source_range]")
{
    const YamlSourceRange range{
        .offset = 4,
        .size = 0
    };

    REQUIRE_FALSE(range.contains(3));
    REQUIRE_FALSE(range.contains(4));
    REQUIRE_FALSE(range.contains(5));
}

TEST_CASE("YamlSourceRange returns exact source view", "[job_yaml][source_range]")
{
    constexpr std::string_view source = "prefixalphasuffix";

    const YamlSourceRange range{
        .offset = 6,
        .size = 5
    };

    const std::string_view view = range.view(source);

    REQUIRE(view == "alpha");
    REQUIRE(view.data() == source.data() + 6);
    REQUIRE(view.size() == 5);
}

TEST_CASE("YamlSourceRange respects source view boundaries", "[job_yaml][source_range]")
{
    constexpr char source[] = "alpha-extra";
    const std::string_view boundedSource{source, 5};

    const YamlSourceRange range{
        .offset = 0,
        .size = 5
    };

    const std::string_view view = range.view(boundedSource);

    REQUIRE(view == "alpha");
    REQUIRE(view.data() == source);
    REQUIRE(view.size() == 5);
}

TEST_CASE("YamlSourceRange returns empty view for empty range", "[job_yaml][source_range]")
{
    constexpr std::string_view source = "alpha";

    const YamlSourceRange range{
        .offset = 2,
        .size = 0
    };

    const std::string_view view = range.view(source);

    REQUIRE(view.empty());
    REQUIRE(view.data() == source.data() + 2);
}

TEST_CASE("YamlSourceRange rejects offset beyond source", "[job_yaml][source_range]")
{
    constexpr std::string_view source = "alpha";

    const YamlSourceRange range{
        .offset = 6,
        .size = 1
    };

    REQUIRE(range.view(source).empty());
}

TEST_CASE("YamlSourceRange rejects range extending beyond source", "[job_yaml][source_range]")
{
    constexpr std::string_view source = "alpha";

    const YamlSourceRange range{
        .offset = 3,
        .size = 3
    };

    REQUIRE(range.view(source).empty());
}

TEST_CASE("YamlSourceRange accepts range ending exactly at source end", "[job_yaml][source_range]")
{
    constexpr std::string_view source = "alpha";

    const YamlSourceRange range{
        .offset = 2,
        .size = 3
    };

    REQUIRE(range.view(source) == "pha");
    REQUIRE(range.endOffset() == source.size());
}

TEST_CASE("YamlSourceRange preserves embedded null bytes", "[job_yaml][source_range]")
{
    constexpr char source[] = {
        'A', '\0', 'B', 'C'
    };

    const std::string_view sourceView{source, sizeof(source)};

    const YamlSourceRange range{
        .offset = 0,
        .size = 3
    };

    const std::string_view view = range.view(sourceView);

    REQUIRE(view.size() == 3);
    REQUIRE(view.data() == source);
    REQUIRE(view[0] == 'A');
    REQUIRE(view[1] == '\0');
    REQUIRE(view[2] == 'B');
}

} // namespace job::yaml::tests