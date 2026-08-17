#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>
#include <vector>

#include "regex_splitter.h"

using job::token::RegexSplitter;
using job::token::SplitPattern;

//
// Block 1: usage / examples
//

TEST_CASE("RegexSplitter defaults to no configured pattern", "[token][regex][usage]")
{
    RegexSplitter splitter;

    REQUIRE(splitter.patternType() == SplitPattern::None);
    REQUIRE(splitter.patternString().empty());
    REQUIRE_FALSE(splitter.isValid());
}

TEST_CASE("RegexSplitter selects canonical pattern strings", "[token][regex][usage]")
{
    RegexSplitter splitter;

    splitter.setPattern(SplitPattern::GPT2);
    REQUIRE(splitter.patternType() == SplitPattern::GPT2);
    REQUIRE(splitter.patternString() == RegexSplitter::kPatternGPT2);

    splitter.setPattern(SplitPattern::R50K);
    REQUIRE(splitter.patternType() == SplitPattern::R50K);
    REQUIRE(splitter.patternString() == RegexSplitter::kPatternR50K);

    splitter.setPattern(SplitPattern::P50K);
    REQUIRE(splitter.patternType() == SplitPattern::P50K);
    REQUIRE(splitter.patternString() == RegexSplitter::kPatternP50K);

    splitter.setPattern(SplitPattern::P50KEdit);
    REQUIRE(splitter.patternType() == SplitPattern::P50KEdit);
    REQUIRE(splitter.patternString() == RegexSplitter::kPatternP50KEdit);

    splitter.setPattern(SplitPattern::CL100K);
    REQUIRE(splitter.patternType() == SplitPattern::CL100K);
    REQUIRE(splitter.patternString() == RegexSplitter::kPatternCL100K);

    splitter.setPattern(SplitPattern::O200K);
    REQUIRE(splitter.patternType() == SplitPattern::O200K);
    REQUIRE(splitter.patternString() == RegexSplitter::kPatternO200K);

    splitter.setPattern(SplitPattern::O200KHarmony);
    REQUIRE(splitter.patternType() == SplitPattern::O200KHarmony);
    REQUIRE(splitter.patternString() == RegexSplitter::kPatternO200KHarmony);

    splitter.setPattern(SplitPattern::GPT4);
    REQUIRE(splitter.patternType() == SplitPattern::GPT4);
    REQUIRE(splitter.patternString() == RegexSplitter::kPatternGPT4);

    splitter.setPattern(SplitPattern::LLaMA3);
    REQUIRE(splitter.patternType() == SplitPattern::LLaMA3);
    REQUIRE(splitter.patternString() == RegexSplitter::kPatternLLaMA3);

    splitter.setPattern(SplitPattern::Qwen2);
    REQUIRE(splitter.patternType() == SplitPattern::Qwen2);
    REQUIRE(splitter.patternString() == RegexSplitter::kPatternQwen2);
}

TEST_CASE("RegexSplitter preserves canonical alias relationships", "[token][regex][usage]")
{
    REQUIRE(RegexSplitter::kPatternR50K == RegexSplitter::kPatternGPT2);
    REQUIRE(RegexSplitter::kPatternP50K == RegexSplitter::kPatternGPT2);
    REQUIRE(RegexSplitter::kPatternP50KEdit == RegexSplitter::kPatternGPT2);
    REQUIRE(RegexSplitter::kPatternGPT4 == RegexSplitter::kPatternCL100K);
    REQUIRE(RegexSplitter::kPatternO200KHarmony == RegexSplitter::kPatternO200K);
}

TEST_CASE("RegexSplitter custom pattern splits matching text", "[token][regex][custom][usage]")
{
    RegexSplitter splitter{std::string{R"([A-Za-z]+|\d+|[^A-Za-z\d]+)"}};

    REQUIRE(splitter.patternType() == SplitPattern::Custom);
    REQUIRE(splitter.isValid());

    const std::string text = "hello 123 world";
    const auto chunks = splitter.split(text);

    REQUIRE(chunks.size() == 5);
    REQUIRE(chunks[0] == "hello");
    REQUIRE(chunks[1] == " ");
    REQUIRE(chunks[2] == "123");
    REQUIRE(chunks[3] == " ");
    REQUIRE(chunks[4] == "world");
}

TEST_CASE("RegexSplitter preserves unmatched text around regex matches", "[token][regex][custom][usage]")
{
    RegexSplitter splitter{std::string{R"(\d+)"}};

    REQUIRE(splitter.isValid());

    const std::string text = "abc123def456ghi";
    const auto chunks = splitter.split(text);

    REQUIRE(chunks.size() == 5);
    REQUIRE(chunks[0] == "abc");
    REQUIRE(chunks[1] == "123");
    REQUIRE(chunks[2] == "def");
    REQUIRE(chunks[3] == "456");
    REQUIRE(chunks[4] == "ghi");
}

TEST_CASE("RegexSplitter returned chunks reference original input storage", "[token][regex][usage]")
{
    RegexSplitter splitter{std::string{R"([A-Za-z]+|\s+)"}};

    const std::string text = "hello world";
    const auto chunks = splitter.split(text);

    REQUIRE(chunks.size() == 3);
    REQUIRE(chunks[0] == "hello");
    REQUIRE(chunks[1] == " ");
    REQUIRE(chunks[2] == "world");
    REQUIRE(chunks[0].data() == text.data());
    REQUIRE(chunks[1].data() == text.data() + 5);
    REQUIRE(chunks[2].data() == text.data() + 6);
}

TEST_CASE("RegexSplitter output reconstructs original input exactly", "[token][regex][usage]")
{
    RegexSplitter splitter{std::string{R"([A-Za-z]+|\d+|\s+)"}};

    const std::string text = "abc 123 !!! xyz";
    const auto chunks = splitter.split(text);

    std::string reconstructed;
    for (const std::string_view chunk : chunks)
        reconstructed.append(chunk);

    REQUIRE(reconstructed == text);
}

TEST_CASE("RegexSplitter can be reconfigured from custom to built-in pattern", "[token][regex][usage]")
{
    RegexSplitter splitter{std::string{R"(\d+)"}};

    REQUIRE(splitter.patternType() == SplitPattern::Custom);

    splitter.setPattern(SplitPattern::LLaMA3);

    REQUIRE(splitter.patternType() == SplitPattern::LLaMA3);
    REQUIRE(splitter.patternString() == RegexSplitter::kPatternLLaMA3);
}

TEST_CASE("RegexSplitter can be reconfigured between custom patterns", "[token][regex][custom][usage]")
{
    RegexSplitter splitter{std::string{R"(\d+)"}};

    REQUIRE(splitter.isValid());

    splitter.setCustomPattern(R"([A-Za-z]+)");

    REQUIRE(splitter.patternType() == SplitPattern::Custom);
    REQUIRE(splitter.patternString() == R"([A-Za-z]+)");
    REQUIRE(splitter.isValid());

    const auto chunks = splitter.split("abc123");

    REQUIRE(chunks.size() == 2);
    REQUIRE(chunks[0] == "abc");
    REQUIRE(chunks[1] == "123");
}

//
// Block 2: edge cases / invalid regex behavior
//

TEST_CASE("RegexSplitter None pattern returns input as one chunk", "[token][regex][edge]")
{
    RegexSplitter splitter;

    const std::string text = "leave me alone";
    const auto chunks = splitter.split(text);

    REQUIRE(chunks.size() == 1);
    REQUIRE(chunks[0] == text);
}

TEST_CASE("RegexSplitter empty input produces no chunks", "[token][regex][edge]")
{
    RegexSplitter splitter{std::string{R"(\w+)"}};
    const auto chunks = splitter.split("");

    REQUIRE(chunks.empty());
}

TEST_CASE("RegexSplitter appends into caller-owned output buffer", "[token][regex][edge]")
{
    RegexSplitter splitter{std::string{R"(\d+)"}};

    std::vector<std::string_view> chunks{"existing"};
    const std::string text = "abc123";

    splitter.split(text, chunks);

    REQUIRE(chunks.size() == 3);
    REQUIRE(chunks[0] == "existing");
    REQUIRE(chunks[1] == "abc");
    REQUIRE(chunks[2] == "123");
}

TEST_CASE("RegexSplitter invalid custom regex degrades to one unchanged chunk", "[token][regex][edge][invalid]")
{
    RegexSplitter splitter{std::string{"("}};

    REQUIRE(splitter.patternType() == SplitPattern::Custom);
    REQUIRE_FALSE(splitter.isValid());

    const std::string text = "this survives regex nonsense";
    const auto chunks = splitter.split(text);

    REQUIRE(chunks.size() == 1);
    REQUIRE(chunks[0] == text);
}

TEST_CASE("RegexSplitter empty custom pattern is invalid and preserves input", "[token][regex][edge][invalid]")
{
    RegexSplitter splitter{std::string{}};

    REQUIRE(splitter.patternType() == SplitPattern::Custom);
    REQUIRE(splitter.patternString().empty());
    REQUIRE_FALSE(splitter.isValid());

    const std::string text = "still here";
    const auto chunks = splitter.split(text);

    REQUIRE(chunks.size() == 1);
    REQUIRE(chunks[0] == text);
}

TEST_CASE("RegexSplitter setting None clears previous regex state", "[token][regex][edge]")
{
    RegexSplitter splitter{std::string{R"(\d+)"}};

    REQUIRE(splitter.isValid());

    splitter.setPattern(SplitPattern::None);

    REQUIRE(splitter.patternType() == SplitPattern::None);
    REQUIRE(splitter.patternString().empty());
    REQUIRE_FALSE(splitter.isValid());

    const auto chunks = splitter.split("abc123");

    REQUIRE(chunks.size() == 1);
    REQUIRE(chunks[0] == "abc123");
}

TEST_CASE("RegexSplitter zero-length custom matches still make forward progress", "[token][regex][edge]")
{
    RegexSplitter splitter{std::string{R"(^|$)"}};

    REQUIRE(splitter.isValid());

    const std::string text = "abc";
    const auto chunks = splitter.split(text);

    std::string reconstructed;
    for (const std::string_view chunk : chunks)
        reconstructed.append(chunk);

    REQUIRE(reconstructed == text);
}

TEST_CASE("RegexSplitter keeps unsupported canonical patterns as definitions even when std regex cannot compile them", "[token][regex][edge][canonical]")
{
    RegexSplitter splitter{SplitPattern::Qwen2};

    REQUIRE(splitter.patternType() == SplitPattern::Qwen2);
    REQUIRE(splitter.patternString() == RegexSplitter::kPatternQwen2);

    const std::string text = "Qwen3 tokenizer regex time";
    const auto chunks = splitter.split(text);

    // Current std::regex backend may reject Unicode-property, inline-flag,
    // lookaround, or related canonical syntax. The class contract is to
    // degrade safely to the original input rather than discard text.
    if (!splitter.isValid()) {
        REQUIRE(chunks.size() == 1);
        REQUIRE(chunks[0] == text);
    } else {
        std::string reconstructed;
        for (const std::string_view chunk : chunks)
            reconstructed.append(chunk);

        REQUIRE(reconstructed == text);
    }
}

TEST_CASE("RegexSplitter every canonical pattern preserves all input text", "[token][regex][canonical][edge]")
{
    constexpr SplitPattern patterns[] = {
        SplitPattern::GPT2, SplitPattern::R50K, SplitPattern::P50K, SplitPattern::P50KEdit,
        SplitPattern::CL100K, SplitPattern::O200K, SplitPattern::O200KHarmony,
        SplitPattern::GPT4, SplitPattern::LLaMA3, SplitPattern::Qwen2
    };

    const std::string text = "Hello, world! 123\n" "Qwen3 can't eat my bytes. \xC3\xA9";

    for (const SplitPattern pattern : patterns) {
        RegexSplitter splitter{pattern};
        const auto chunks = splitter.split(text);

        std::string reconstructed;
        for (const std::string_view chunk : chunks)
            reconstructed.append(chunk);

        REQUIRE(reconstructed == text);
    }
}

//
// Block 3: stress / benchmarks
//

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
TEST_CASE("Benchmark RegexSplitter custom ASCII splitting", "[token][regex][benchmark]")
{
    RegexSplitter splitter{std::string{R"([A-Za-z]+|\d+|\s+|[^A-Za-z\d\s]+)"}};

    REQUIRE(splitter.isValid());

    std::string text;
    for (std::size_t i = 0; i < 100; ++i)
        text += "the quick brown fox 123 jumps! ";

    BENCHMARK("Split repeated ASCII text"){
        return splitter.split(text);
    };
}
#endif