#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include <job_json_diagnostic.h>

TEST_CASE("JsonDiagnostic default constructs with no error", "[job_json][diagnostic]")
{
    constexpr job::json::JsonDiagnostic diagnostic;

    STATIC_REQUIRE(diagnostic.code() == job::json::JsonDiagnosticCode::None);
    STATIC_REQUIRE_FALSE(diagnostic.hasError());
    STATIC_REQUIRE(diagnostic.offset() == 0);
    STATIC_REQUIRE(diagnostic.range().begin() == 0);
    STATIC_REQUIRE(diagnostic.range().end() == 0);
    STATIC_REQUIRE(diagnostic.comment().empty());
}

TEST_CASE("JsonDiagnostic stores code range and comment", "[job_json][diagnostic]")
{
    constexpr job::json::JsonDiagnostic diagnostic{
        job::json::JsonDiagnosticCode::UnexpectedToken,
        job::json::JsonSourceRange{4, 7},
        "Unexpected token"
    };

    STATIC_REQUIRE(diagnostic.code() == job::json::JsonDiagnosticCode::UnexpectedToken);
    STATIC_REQUIRE(diagnostic.hasError());

    STATIC_REQUIRE(diagnostic.range().begin() == 4);
    STATIC_REQUIRE(diagnostic.range().end() == 7);
    STATIC_REQUIRE(diagnostic.range().size() == 3);

    STATIC_REQUIRE(diagnostic.offset() == 4);
    STATIC_REQUIRE(diagnostic.comment() == "Unexpected token");
}

TEST_CASE("JsonDiagnostic offset is range beginning", "[job_json][diagnostic]")
{
    constexpr job::json::JsonDiagnostic diagnostic{
        job::json::JsonDiagnosticCode::InvalidNumber,
        job::json::JsonSourceRange{17, 24}
    };

    STATIC_REQUIRE(diagnostic.offset() == 17);
}

TEST_CASE("JsonDiagnostic reports first source character as line one column one", "[job_json][diagnostic]")
{
    constexpr std::string_view source = "cake";

    constexpr job::json::JsonDiagnostic diagnostic{
        job::json::JsonDiagnosticCode::InvalidToken,
        job::json::JsonSourceRange{0, 1}
    };

    STATIC_REQUIRE(diagnostic.line(source) == 1);
    STATIC_REQUIRE(diagnostic.column(source) == 1);
}

TEST_CASE("JsonDiagnostic reports columns on first line", "[job_json][diagnostic]")
{
    constexpr std::string_view source = "abcdef";

    constexpr job::json::JsonDiagnostic first{
        job::json::JsonDiagnosticCode::InvalidToken,
        job::json::JsonSourceRange{0, 1}
    };

    constexpr job::json::JsonDiagnostic middle{
        job::json::JsonDiagnosticCode::InvalidToken,
        job::json::JsonSourceRange{2, 3}
    };

    constexpr job::json::JsonDiagnostic last{
        job::json::JsonDiagnosticCode::InvalidToken,
        job::json::JsonSourceRange{5, 6}
    };

    STATIC_REQUIRE(first.column(source) == 1);
    STATIC_REQUIRE(middle.column(source) == 3);
    STATIC_REQUIRE(last.column(source) == 6);

    STATIC_REQUIRE(first.line(source) == 1);
    STATIC_REQUIRE(middle.line(source) == 1);
    STATIC_REQUIRE(last.line(source) == 1);
}

TEST_CASE("JsonDiagnostic reports position after newline", "[job_json][diagnostic]")
{
    constexpr std::string_view source =
        "cake\n"
        "court";

    constexpr job::json::JsonDiagnostic diagnostic{
        job::json::JsonDiagnosticCode::UnexpectedToken,
        job::json::JsonSourceRange{5, 6}
    };

    STATIC_REQUIRE(diagnostic.line(source) == 2);
    STATIC_REQUIRE(diagnostic.column(source) == 1);
}

TEST_CASE("JsonDiagnostic reports position in second line", "[job_json][diagnostic]")
{
    constexpr std::string_view source =
        "cake\n"
        "court";

    constexpr job::json::JsonDiagnostic diagnostic{
        job::json::JsonDiagnosticCode::UnexpectedToken,
        job::json::JsonSourceRange{8, 9}
    };

    STATIC_REQUIRE(diagnostic.line(source) == 2);
    STATIC_REQUIRE(diagnostic.column(source) == 4);
}

TEST_CASE("JsonDiagnostic reports position across multiple lines", "[job_json][diagnostic]")
{
    constexpr std::string_view source =
        "{\n"
        "  \"cake\": true,\n"
        "  \"court\": false\n"
        "}";

    constexpr auto courtOffset = source.find("court");

    constexpr job::json::JsonDiagnostic diagnostic{
        job::json::JsonDiagnosticCode::UnexpectedToken,
        job::json::JsonSourceRange{courtOffset, courtOffset + 5}
    };

    STATIC_REQUIRE(diagnostic.line(source) == 3);
    STATIC_REQUIRE(diagnostic.column(source) == 4);
}

TEST_CASE("JsonDiagnostic handles empty lines", "[job_json][diagnostic]")
{
    constexpr std::string_view source =
        "one\n"
        "\n"
        "three";

    constexpr job::json::JsonDiagnostic diagnostic{
        job::json::JsonDiagnosticCode::InvalidToken,
        job::json::JsonSourceRange{5, 6}
    };

    STATIC_REQUIRE(diagnostic.line(source) == 3);
    STATIC_REQUIRE(diagnostic.column(source) == 1);
}

TEST_CASE("JsonDiagnostic handles range at end of source", "[job_json][diagnostic]")
{
    constexpr std::string_view source = "cake";

    constexpr job::json::JsonDiagnostic diagnostic{
        job::json::JsonDiagnosticCode::UnexpectedEnd,
        job::json::JsonSourceRange{source.size(), source.size()}
    };

    STATIC_REQUIRE(diagnostic.offset() == source.size());
    STATIC_REQUIRE(diagnostic.line(source) == 1);
    STATIC_REQUIRE(diagnostic.column(source) == 5);
}

TEST_CASE("JsonDiagnostic handles end of source after newline", "[job_json][diagnostic]")
{
    constexpr std::string_view source = "cake\n";

    constexpr job::json::JsonDiagnostic diagnostic{
        job::json::JsonDiagnosticCode::UnexpectedEnd,
        job::json::JsonSourceRange{source.size(), source.size()}
    };

    STATIC_REQUIRE(diagnostic.line(source) == 2);
    STATIC_REQUIRE(diagnostic.column(source) == 1);
}

TEST_CASE("JsonDiagnostic comment remains borrowed", "[job_json][diagnostic]")
{
    constexpr std::string_view comment = "Expected JSON value";

    constexpr job::json::JsonDiagnostic diagnostic{
        job::json::JsonDiagnosticCode::ExpectedValue,
        job::json::JsonSourceRange{3, 3},
        comment
    };

    STATIC_REQUIRE(diagnostic.comment() == comment);

    REQUIRE(diagnostic.comment().data() == comment.data());
    REQUIRE(diagnostic.comment().size() == comment.size());
}

TEST_CASE("JsonDiagnostic copy preserves diagnostic state", "[job_json][diagnostic]")
{
    constexpr job::json::JsonDiagnostic original{
        job::json::JsonDiagnosticCode::NumberOutOfRange,
        job::json::JsonSourceRange{10, 20},
        "Number is outside destination range"
    };

    constexpr job::json::JsonDiagnostic copied = original;

    STATIC_REQUIRE(copied.code() == original.code());
    STATIC_REQUIRE(copied.range().begin() == original.range().begin());
    STATIC_REQUIRE(copied.range().end() == original.range().end());
    STATIC_REQUIRE(copied.comment() == original.comment());
    STATIC_REQUIRE(copied.hasError());
}

TEST_CASE("JsonDiagnostic move preserves diagnostic state", "[job_json][diagnostic]")
{
    constexpr job::json::JsonDiagnostic moved = [] {
        job::json::JsonDiagnostic diagnostic{
            job::json::JsonDiagnosticCode::InvalidUnicode,
            job::json::JsonSourceRange{7, 13},
            "Invalid Unicode escape"
        };

        return job::json::JsonDiagnostic{std::move(diagnostic)};
    }();

    STATIC_REQUIRE(moved.code() == job::json::JsonDiagnosticCode::InvalidUnicode);
    STATIC_REQUIRE(moved.range().begin() == 7);
    STATIC_REQUIRE(moved.range().end() == 13);
    STATIC_REQUIRE(moved.comment() == "Invalid Unicode escape");
    STATIC_REQUIRE(moved.hasError());
}

TEST_CASE("JsonDiagnostic distinguishes informational none from error", "[job_json][diagnostic]")
{
    constexpr job::json::JsonDiagnostic none{
        job::json::JsonDiagnosticCode::None,
        job::json::JsonSourceRange{5, 5},
        "No error"
    };

    constexpr job::json::JsonDiagnostic error{
        job::json::JsonDiagnosticCode::ExpectedObjectKey,
        job::json::JsonSourceRange{5, 5},
        "Expected object key"
    };

    STATIC_REQUIRE_FALSE(none.hasError());
    STATIC_REQUIRE(error.hasError());

    STATIC_REQUIRE(none.offset() == error.offset());
}

