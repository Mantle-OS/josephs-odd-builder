#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>

#include <job_json_emitter.h>


TEST_CASE("JsonEmitter emits empty string", "[job_json][emitter][string]")
{
    std::string output;
    const std::string value;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"("")");
}

TEST_CASE("JsonEmitter emits ordinary string", "[job_json][emitter][string]")
{
    std::string output;
    const std::string value = "Cake Court";

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"("Cake Court")");
}

TEST_CASE("JsonEmitter emits string view", "[job_json][emitter][string]")
{
    std::string output;
    constexpr std::string_view value = "JOB";

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"("JOB")");
}

TEST_CASE("JsonEmitter escapes quotation mark", "[job_json][emitter][string]")
{
    std::string output;
    const std::string value = "Cake \"Court\"";

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"("Cake \"Court\"")");
}

TEST_CASE("JsonEmitter escapes reverse solidus", "[job_json][emitter][string]")
{
    std::string output;
    const std::string value = R"(Cake\Court)";

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"("Cake\\Court")");
}

TEST_CASE("JsonEmitter escapes backspace", "[job_json][emitter][string]")
{
    std::string output;
    const std::string value{"Cake\bCourt", 10};

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"("Cake\bCourt")");
}

TEST_CASE("JsonEmitter escapes form feed", "[job_json][emitter][string]")
{
    std::string output;
    const std::string value{"Cake\fCourt", 10};

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"("Cake\fCourt")");
}

TEST_CASE("JsonEmitter escapes line feed", "[job_json][emitter][string]")
{
    std::string output;
    const std::string value = "Cake\nCourt";

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"("Cake\nCourt")");
}

TEST_CASE("JsonEmitter escapes carriage return", "[job_json][emitter][string]")
{
    std::string output;
    const std::string value = "Cake\rCourt";

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"("Cake\rCourt")");
}

TEST_CASE("JsonEmitter escapes horizontal tab", "[job_json][emitter][string]")
{
    std::string output;
    const std::string value = "Cake\tCourt";

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"("Cake\tCourt")");
}

TEST_CASE("JsonEmitter escapes all short escape characters", "[job_json][emitter][string]")
{
    std::string output;

    const std::string value{
        '"',
        '\\',
        '\b',
        '\f',
        '\n',
        '\r',
        '\t'
    };

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"("\"\\\b\f\n\r\t")");
}

TEST_CASE("JsonEmitter escapes null character", "[job_json][emitter][string]")
{
    std::string output;

    const char bytes[] = {
        'A',
        '\0',
        'B'
    };

    const std::string value{bytes, sizeof(bytes)};

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"("A\u0000B")");
}

TEST_CASE("JsonEmitter escapes generic control character", "[job_json][emitter][string]")
{
    std::string output;

    const char bytes[] = {
        'A',
        static_cast<char>(0x01),
        'B'
    };

    const std::string value{bytes, sizeof(bytes)};

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"("A\u0001B")");
}

TEST_CASE("JsonEmitter escapes highest control character", "[job_json][emitter][string]")
{
    std::string output;

    const char bytes[] = {
        static_cast<char>(0x1F)
};

const std::string value{bytes, sizeof(bytes)};

REQUIRE(job::json::JsonEmitter::emit(value, output));
REQUIRE(output == R"("\u001F")");
}

TEST_CASE("JsonEmitter does not escape space", "[job_json][emitter][string]")
{
    std::string output;
    const std::string value = " ";

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"(" ")");
}

TEST_CASE("JsonEmitter preserves raw UTF8", "[job_json][emitter][string]")
{
    std::string output;
    const std::string value = "Grüße 世界 😀";

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "\"Grüße 世界 😀\"");
}

TEST_CASE("JsonEmitter preserves UTF8 bytes exactly", "[job_json][emitter][string]")
{
    const std::string value = "😀";

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));

    REQUIRE(output.size() == value.size() + 2);
    REQUIRE(output.front() == '"');
    REQUIRE(output.back() == '"');
    REQUIRE(std::string_view{output}.substr(1, value.size()) == value);
}

TEST_CASE("JsonEmitter preserves embedded UTF8 around escaped characters", "[job_json][emitter][string]")
{
    std::string output;
    const std::string value = "世界\n😀";

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "\"世界\\n😀\"");
}

TEST_CASE("JsonEmitter emits embedded null from string view", "[job_json][emitter][string]")
{
    static constexpr char bytes[] = {
        'J',
        'O',
        '\0',
        'B'
    };

    constexpr std::string_view value{bytes, sizeof(bytes)};

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"("JO\u0000B")");
}

TEST_CASE("JsonEmitter escapes multiple control characters independently", "[job_json][emitter][string]")
{
    std::string output;

    const char bytes[] = {
        static_cast<char>(0x01),
        static_cast<char>(0x02),
        static_cast<char>(0x03)
    };

    const std::string value{bytes, sizeof(bytes)};

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"("\u0001\u0002\u0003")");
}

TEST_CASE("JsonEmitter appends emitted string to existing sink", "[job_json][emitter][string]")
{
    std::string output = "prefix:";

    const std::string value = "JOB";

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"(prefix:"JOB")");
}

