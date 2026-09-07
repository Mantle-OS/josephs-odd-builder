#include <catch2/catch_test_macros.hpp>
#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif
#include <array>
#include <cstddef>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <job_http_parser.h>

using namespace job::net;

TEST_CASE("JobHttpParser parses a complete Content-Length response", "[job_net][http_parser][usage][content_length]")
{
    JobHttpParser parser;

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 5\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Hello";

    const auto result = parser.parse(input);

    REQUIRE(result == JobHttpParser::ParseResult::Complete);
    REQUIRE(parser.isComplete());
    REQUIRE_FALSE(parser.hasError());
    REQUIRE(parser.state() == JobHttpParser::State::Complete);

    REQUIRE(parser.bodyFraming() == JobHttpParser::BodyFraming::ContentLength);
    REQUIRE(parser.hasContentLength());
    REQUIRE(parser.contentLength() == 5);

    REQUIRE(parser.response().code() == 200);
    REQUIRE(parser.response().statusCode() == JobHttpStatusCode::Ok);
    REQUIRE(parser.response().reasonPhrase() == "OK");

    REQUIRE(parser.response().headers().value("content-length") == "5");
    REQUIRE(parser.response().headers().value("content-type") == "text/plain");

    REQUIRE(parser.bodyBytesReceived() == 5);
    REQUIRE(parser.response().bodySize() == 5);

    const auto body = parser.response().bodyView();

    REQUIRE(body.size() == 5);
    REQUIRE(static_cast<char>(body[0]) == 'H');
    REQUIRE(static_cast<char>(body[1]) == 'e');
    REQUIRE(static_cast<char>(body[2]) == 'l');
    REQUIRE(static_cast<char>(body[3]) == 'l');
    REQUIRE(static_cast<char>(body[4]) == 'o');

    REQUIRE(input.empty());
}

TEST_CASE("JobHttpParser preserves unconsumed bytes after Content-Length response", "[job_net][http_parser][usage][tail]")
{
    JobHttpParser parser;

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "HelloNEXT";

    const auto result = parser.parse(input);

    REQUIRE(result == JobHttpParser::ParseResult::Complete);
    REQUIRE(parser.isComplete());

    REQUIRE(parser.response().bodySize() == 5);
    REQUIRE(parser.bodyBytesReceived() == 5);

    REQUIRE(input == "NEXT");

    const std::size_t expectedConsumed =
        std::string_view{
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 5\r\n"
            "\r\n"
            "Hello"
        }.size();

    REQUIRE(parser.bytesReceived() == expectedConsumed);
}

TEST_CASE("JobHttpParser parses zero Content-Length response without body", "[job_net][http_parser][usage][content_length][empty]")
{
    JobHttpParser parser;

    std::string_view input =
        "HTTP/1.1 204 No Content\r\n"
        "Content-Length: 0\r\n"
        "\r\n";

    const auto result = parser.parse(input);

    REQUIRE(result == JobHttpParser::ParseResult::Complete);
    REQUIRE(parser.isComplete());

    REQUIRE(parser.response().code() == 204);
    REQUIRE(parser.response().reasonPhrase() == "No Content");

    REQUIRE(parser.hasContentLength());
    REQUIRE(parser.contentLength() == 0);
    REQUIRE(parser.bodyFraming() == JobHttpParser::BodyFraming::None);

    REQUIRE_FALSE(parser.response().hasBody());
    REQUIRE(parser.bodyBytesReceived() == 0);
    REQUIRE(input.empty());
}

TEST_CASE("JobHttpParser parses response without a reason phrase", "[job_net][http_parser][usage][status_line]")
{
    JobHttpParser parser;

    std::string_view input =
        "HTTP/1.1 200\r\n"
        "Content-Length: 0\r\n"
        "\r\n";

    const auto result = parser.parse(input);

    REQUIRE(result == JobHttpParser::ParseResult::Complete);
    REQUIRE(parser.response().code() == 200);
    REQUIRE(parser.response().reasonPhrase().empty());
}

TEST_CASE("JobHttpParser preserves repeated header field lines", "[job_net][http_parser][usage][headers][repeated]")
{
    JobHttpParser parser;

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 0\r\n"
        "X-Test: first\r\n"
        "X-Test: second\r\n"
        "\r\n";

    const auto result = parser.parse(input);

    REQUIRE(result == JobHttpParser::ParseResult::Complete);

    REQUIRE(parser.response().headers().count("x-test") == 2);

    const auto values = parser.response().headers().values("x-test");

    REQUIRE(values.size() == 2);
    REQUIRE(values[0] == "first");
    REQUIRE(values[1] == "second");
}

TEST_CASE("JobHttpParser parses identical repeated Content-Length values", "[job_net][http_parser][content_length][repeated]")
{
    JobHttpParser parser;

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 5\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "Hello";

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Complete);

    REQUIRE(parser.hasContentLength());
    REQUIRE(parser.contentLength() == 5);
    REQUIRE(parser.response().bodySize() == 5);
}

TEST_CASE("JobHttpParser parses identical comma separated Content-Length values", "[job_net][http_parser][content_length][list]")
{
    JobHttpParser parser;

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 5, 5\r\n"
        "\r\n"
        "Hello";

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Complete);

    REQUIRE(parser.hasContentLength());
    REQUIRE(parser.contentLength() == 5);
    REQUIRE(parser.response().bodySize() == 5);
}

TEST_CASE("JobHttpParser rejects conflicting Content-Length values", "[job_net][http_parser][content_length][error]")
{
    SECTION("Repeated field lines")
    {
        JobHttpParser parser;

        std::string_view input =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 5\r\n"
            "Content-Length: 6\r\n"
            "\r\n";

        REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Error);
        REQUIRE(parser.hasError());
        REQUIRE(parser.state() == JobHttpParser::State::Error);
        REQUIRE_FALSE(parser.lastError().empty());
    }

    SECTION("Comma separated values")
    {
        JobHttpParser parser;

        std::string_view input =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 5, 6\r\n"
            "\r\n";

        REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Error);
        REQUIRE(parser.hasError());
        REQUIRE_FALSE(parser.lastError().empty());
    }
}

TEST_CASE("JobHttpParser rejects invalid Content-Length values", "[job_net][http_parser][content_length][error]")
{
    static constexpr std::string_view invalidValues[] = {
        "",
        "abc",
        "-1",
        "+5",
        "5x",
        "1 2"
    };

    for (const std::string_view value : invalidValues) {
        INFO("Content-Length value: '" << value << "'");

        JobHttpParser parser;

        std::string input =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: ";

        input += value;
        input += "\r\n\r\n";

        std::string_view view = input;

        REQUIRE(parser.parse(view) == JobHttpParser::ParseResult::Error);
        REQUIRE(parser.hasError());
    }
}

TEST_CASE("JobHttpParser does not eagerly allocate a huge Content-Length body", "[job_net][http_parser][content_length][large]")
{
    JobHttpParser parser;

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 18446744073709551615\r\n"
        "\r\n";

    const auto result = parser.parse(input);

    REQUIRE(result == JobHttpParser::ParseResult::NeedMoreData);
    REQUIRE_FALSE(parser.hasError());

    REQUIRE(parser.hasContentLength());
    REQUIRE(parser.contentLength() == std::numeric_limits<std::size_t>::max());

    REQUIRE(parser.bodyFraming() == JobHttpParser::BodyFraming::ContentLength);
    REQUIRE(parser.bodyBytesReceived() == 0);
    REQUIRE(parser.response().bodySize() == 0);
}

TEST_CASE("JobHttpParser parses a chunked response", "[job_net][http_parser][chunked][usage]")
{
    JobHttpParser parser;

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5\r\n"
        "Hello\r\n"
        "6\r\n"
        " World\r\n"
        "0\r\n"
        "\r\n";

    const auto result = parser.parse(input);

    REQUIRE(result == JobHttpParser::ParseResult::Complete);
    REQUIRE(parser.isComplete());

    REQUIRE(parser.bodyFraming() == JobHttpParser::BodyFraming::Chunked);
    REQUIRE_FALSE(parser.hasContentLength());

    REQUIRE(parser.bodyBytesReceived() == 11);
    REQUIRE(parser.response().bodySize() == 11);

    const auto body = parser.response().bodyView();
    const std::string text(reinterpret_cast<const char *>(body.data()), body.size());

    REQUIRE(text == "Hello World");
    REQUIRE(input.empty());
}

TEST_CASE("JobHttpParser accepts hexadecimal chunk sizes case insensitively", "[job_net][http_parser][chunked][hex]")
{
    JobHttpParser parser;

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "A\r\n"
        "0123456789\r\n"
        "0\r\n"
        "\r\n";

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Complete);
    REQUIRE(parser.response().bodySize() == 10);
}

TEST_CASE("JobHttpParser accepts chunk extensions", "[job_net][http_parser][chunked][extension]")
{
    JobHttpParser parser;

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5;foo=bar\r\n"
        "Hello\r\n"
        "0;done=yes\r\n"
        "\r\n";

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Complete);
    REQUIRE(parser.response().bodySize() == 5);
}

TEST_CASE("JobHttpParser rejects malformed chunk sizes", "[job_net][http_parser][chunked][error]")
{
    static constexpr std::string_view badChunkSizes[] = {
        "",
        "xyz",
        "-1",
        "+5",
        "5x",
        " 5"
    };

    for (const std::string_view chunkSize : badChunkSizes) {
        INFO("Chunk size: '" << chunkSize << "'");

        JobHttpParser parser;

        std::string input =
            "HTTP/1.1 200 OK\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n";

        input += chunkSize;
        input += "\r\n";

        std::string_view view = input;

        REQUIRE(parser.parse(view) == JobHttpParser::ParseResult::Error);
        REQUIRE(parser.hasError());
    }
}

TEST_CASE("JobHttpParser stores trailers separately from initial headers", "[job_net][http_parser][chunked][trailers]")
{
    JobHttpParser parser;

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked\r\n"
        "X-Initial: initial\r\n"
        "\r\n"
        "5\r\n"
        "Hello\r\n"
        "0\r\n"
        "Digest: sha-256=test\r\n"
        "X-Trailer: final\r\n"
        "\r\n";

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Complete);

    REQUIRE(parser.response().headers().value("x-initial") == "initial");
    REQUIRE_FALSE(parser.response().headers().contains("digest"));
    REQUIRE_FALSE(parser.response().headers().contains("x-trailer"));

    REQUIRE(parser.response().hasTrailers());
    REQUIRE(parser.response().trailers().value("digest") == "sha-256=test");
    REQUIRE(parser.response().trailers().value("x-trailer") == "final");
}

TEST_CASE("JobHttpParser rejects forbidden trailer fields", "[job_net][http_parser][chunked][trailers][error]")
{
    static constexpr std::string_view forbidden[] = {
        "Content-Length",
        "Transfer-Encoding",
        "Host",
        "Authorization",
        "Set-Cookie",
        "Location"
    };

    for (const std::string_view field : forbidden) {
        INFO("Trailer field: " << field);

        JobHttpParser parser;

        std::string input =
            "HTTP/1.1 200 OK\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n"
            "0\r\n";

        input += field;
        input += ": value\r\n\r\n";

        std::string_view view = input;

        REQUIRE(parser.parse(view) == JobHttpParser::ParseResult::Error);
        REQUIRE(parser.hasError());
    }
}

TEST_CASE("JobHttpParser rejects Transfer-Encoding and Content-Length together", "[job_net][http_parser][framing][error]")
{
    JobHttpParser parser;

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "5\r\n"
        "Hello\r\n"
        "0\r\n"
        "\r\n";

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Error);
    REQUIRE(parser.hasError());
    REQUIRE(parser.lastError() == "Both Transfer-Encoding and Content-Length present");
}

TEST_CASE("JobHttpParser requires chunked Transfer-Encoding to be final", "[job_net][http_parser][transfer_encoding][error]")
{
    JobHttpParser parser;

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked, gzip\r\n"
        "\r\n";

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Error);
    REQUIRE(parser.hasError());
}

TEST_CASE("JobHttpParser rejects duplicate chunked Transfer-Encoding", "[job_net][http_parser][transfer_encoding][error]")
{
    JobHttpParser parser;

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked, chunked\r\n"
        "\r\n";

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Error);
    REQUIRE(parser.hasError());
}

TEST_CASE("JobHttpParser rejects parameters on chunked Transfer-Encoding", "[job_net][http_parser][transfer_encoding][error]")
{
    JobHttpParser parser;

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked;foo=bar\r\n"
        "\r\n";

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Error);
    REQUIRE(parser.hasError());
}

TEST_CASE("JobHttpParser uses close-delimited framing for non-chunked Transfer-Encoding", "[job_net][http_parser][transfer_encoding][close_delimited]")
{
    JobHttpParser parser;

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: gzip\r\n"
        "\r\n"
        "Hello";

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::NeedMoreData);

    REQUIRE(parser.bodyFraming() == JobHttpParser::BodyFraming::CloseDelimited);
    REQUIRE(parser.bodyBytesReceived() == 5);
    REQUIRE(parser.response().bodySize() == 5);

    REQUIRE(parser.notifyEof() == JobHttpParser::ParseResult::Complete);
    REQUIRE(parser.isComplete());
}

TEST_CASE("JobHttpParser uses close-delimited framing when no length is supplied", "[job_net][http_parser][close_delimited][usage]")
{
    JobHttpParser parser;

    std::string_view first =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Hello ";

    REQUIRE(parser.parse(first) == JobHttpParser::ParseResult::NeedMoreData);
    REQUIRE(parser.bodyFraming() == JobHttpParser::BodyFraming::CloseDelimited);

    std::string_view second = "World";

    REQUIRE(parser.parse(second) == JobHttpParser::ParseResult::NeedMoreData);

    REQUIRE(parser.bodyBytesReceived() == 11);

    const auto body = parser.response().bodyView();
    const std::string text(reinterpret_cast<const char *>(body.data()), body.size());

    REQUIRE(text == "Hello World");

    REQUIRE(parser.notifyEof() == JobHttpParser::ParseResult::Complete);
    REQUIRE(parser.isComplete());
}

TEST_CASE("JobHttpParser reports premature EOF for Content-Length body", "[job_net][http_parser][eof][content_length]")
{
    JobHttpParser parser;

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 10\r\n"
        "\r\n"
        "Hello";

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::NeedMoreData);

    REQUIRE(parser.bodyBytesReceived() == 5);
    REQUIRE(parser.notifyEof() == JobHttpParser::ParseResult::Error);
    REQUIRE(parser.hasError());
}

TEST_CASE("JobHttpParser reports premature EOF for chunked body", "[job_net][http_parser][eof][chunked]")
{
    JobHttpParser parser;

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5\r\n"
        "Hel";

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::NeedMoreData);

    REQUIRE(parser.notifyEof() == JobHttpParser::ParseResult::Error);
    REQUIRE(parser.hasError());
}

TEST_CASE("JobHttpParser handles HEAD responses as bodyless", "[job_net][http_parser][head][framing]")
{
    JobHttpParser parser;
    parser.setRequestMethod(HttpMethod::Head);

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 12345\r\n"
        "\r\n"
        "NEXT";

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Complete);

    REQUIRE(parser.bodyFraming() == JobHttpParser::BodyFraming::None);
    REQUIRE(parser.hasContentLength());
    REQUIRE(parser.contentLength() == 12345);

    REQUIRE(parser.response().bodySize() == 0);
    REQUIRE(parser.bodyBytesReceived() == 0);

    REQUIRE(input == "NEXT");
}

TEST_CASE("JobHttpParser rejects malformed Content-Length on HEAD response", "[job_net][http_parser][head][content_length][error]")
{
    JobHttpParser parser;
    parser.setRequestMethod(HttpMethod::Head);

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: nope\r\n"
        "\r\n";

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Error);
    REQUIRE(parser.hasError());
}

TEST_CASE("JobHttpParser does not semantically validate Transfer-Encoding for HEAD response", "[job_net][http_parser][head][transfer_encoding]")
{
    JobHttpParser parser;
    parser.setRequestMethod(HttpMethod::Head);

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: definitely-not-real\r\n"
        "\r\n"
        "NEXT";

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Complete);
    REQUIRE(parser.bodyFraming() == JobHttpParser::BodyFraming::None);
    REQUIRE(input == "NEXT");
}

TEST_CASE("JobHttpParser handles 204 and 304 responses as bodyless", "[job_net][http_parser][bodyless]")
{
    SECTION("204")
    {
        JobHttpParser parser;

        std::string_view input =
            "HTTP/1.1 204 No Content\r\n"
            "\r\n"
            "NEXT";

        REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Complete);
        REQUIRE(parser.bodyFraming() == JobHttpParser::BodyFraming::None);
        REQUIRE(input == "NEXT");
    }

    SECTION("304")
    {
        JobHttpParser parser;

        std::string_view input =
            "HTTP/1.1 304 Not Modified\r\n"
            "Content-Length: 123\r\n"
            "\r\n"
            "NEXT";

        REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Complete);
        REQUIRE(parser.bodyFraming() == JobHttpParser::BodyFraming::None);
        REQUIRE(parser.hasContentLength());
        REQUIRE(parser.contentLength() == 123);
        REQUIRE(input == "NEXT");
    }
}

TEST_CASE("JobHttpParser discards interim informational response before final response", "[job_net][http_parser][informational][interim]")
{
    JobHttpParser parser;

    std::string_view input =
        "HTTP/1.1 103 Early Hints\r\n"
        "X-Early: yes\r\n"
        "\r\n"
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 2\r\n"
        "X-Final: yes\r\n"
        "\r\n"
        "OK";

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Complete);

    REQUIRE(parser.response().code() == 200);
    REQUIRE(parser.response().reasonPhrase() == "OK");

    REQUIRE_FALSE(parser.response().headers().contains("x-early"));
    REQUIRE(parser.response().headers().value("x-final") == "yes");

    REQUIRE(parser.response().bodySize() == 2);
}

TEST_CASE("JobHttpParser handles multiple informational responses", "[job_net][http_parser][informational][multiple]")
{
    JobHttpParser parser;

    std::string_view input =
        "HTTP/1.1 100 Continue\r\n"
        "\r\n"
        "HTTP/1.1 103 Early Hints\r\n"
        "X-Early: yes\r\n"
        "\r\n"
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 0\r\n"
        "\r\n";

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Complete);

    REQUIRE(parser.response().code() == 200);
    REQUIRE_FALSE(parser.response().headers().contains("x-early"));
}

TEST_CASE("JobHttpParser treats 101 response as tunnel", "[job_net][http_parser][tunnel][switching_protocols]")
{
    JobHttpParser parser;

    std::string_view input =
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "\r\n"
        "RAW";

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Complete);

    REQUIRE(parser.bodyFraming() == JobHttpParser::BodyFraming::Tunnel);
    REQUIRE(parser.response().code() == 101);
    REQUIRE(input == "RAW");
}

TEST_CASE("JobHttpParser treats successful CONNECT response as tunnel", "[job_net][http_parser][tunnel][connect]")
{
    JobHttpParser parser;
    parser.setRequestMethod(HttpMethod::Connect);

    std::string_view input =
        "HTTP/1.1 200 Connection Established\r\n"
        "\r\n"
        "TLS";

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Complete);

    REQUIRE(parser.bodyFraming() == JobHttpParser::BodyFraming::Tunnel);
    REQUIRE(parser.response().code() == 200);
    REQUIRE(input == "TLS");
}

TEST_CASE("JobHttpParser body Store mode stores bytes", "[job_net][http_parser][body_mode][store]")
{
    JobHttpParser parser;

    parser.setBodyMode(JobHttpParser::BodyMode::Store);

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "Hello";

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Complete);

    REQUIRE(parser.bodyMode() == JobHttpParser::BodyMode::Store);
    REQUIRE(parser.response().bodySize() == 5);
    REQUIRE(parser.bodyBytesReceived() == 5);
}

TEST_CASE("JobHttpParser body Stream mode sends bytes to callback without storing them", "[job_net][http_parser][body_mode][stream]")
{
    JobHttpParser parser;

    std::string streamed;

    parser.setBodyMode(JobHttpParser::BodyMode::Stream);
    parser.setBodyCallback([&](std::span<const std::byte> data) {
        streamed.append(reinterpret_cast<const char *>(data.data()), data.size());
    });

    std::string_view first =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "Hello ";

    REQUIRE(parser.parse(first) == JobHttpParser::ParseResult::NeedMoreData);

    std::string_view second = "World";

    REQUIRE(parser.parse(second) == JobHttpParser::ParseResult::Complete);

    REQUIRE(streamed == "Hello World");

    REQUIRE(parser.response().bodySize() == 0);
    REQUIRE(parser.bodyBytesReceived() == 11);
}

TEST_CASE("JobHttpParser body Stream mode requires callback for nonempty body", "[job_net][http_parser][body_mode][stream][error]")
{
    JobHttpParser parser;

    parser.setBodyMode(JobHttpParser::BodyMode::Stream);

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "Hello";

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Error);
    REQUIRE(parser.hasError());
}

TEST_CASE("JobHttpParser body Stream mode converts callback exceptions to parser errors", "[job_net][http_parser][body_mode][stream][error]")
{
    JobHttpParser parser;

    parser.setBodyMode(JobHttpParser::BodyMode::Stream);
    parser.setBodyCallback([](std::span<const std::byte>) {
        throw 42;
    });

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "Hello";

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Error);
    REQUIRE(parser.hasError());
    REQUIRE(parser.lastError() == "HTTP body callback threw an exception");
}

TEST_CASE("JobHttpParser body Discard mode counts body without retaining it", "[job_net][http_parser][body_mode][discard]")
{
    JobHttpParser parser;

    parser.setBodyMode(JobHttpParser::BodyMode::Discard);

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "Hello";

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Complete);

    REQUIRE(parser.bodyMode() == JobHttpParser::BodyMode::Discard);
    REQUIRE(parser.bodyBytesReceived() == 5);

    REQUIRE_FALSE(parser.response().hasBody());
    REQUIRE(parser.response().bodySize() == 0);
}

TEST_CASE("JobHttpParser parses a response fragmented one byte at a time", "[job_net][http_parser][fragmentation][bytewise]")
{
    JobHttpParser parser;

    const std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 11\r\n"
        "X-Test: fragmented\r\n"
        "\r\n"
        "Hello World";

    JobHttpParser::ParseResult result = JobHttpParser::ParseResult::NeedMoreData;

    for (const char c : response) {
        const std::array<std::byte, 1> byte{
            static_cast<std::byte>(static_cast<unsigned char>(c))
        };

        std::span<const std::byte> input{byte};

        result = parser.parse(input);

        REQUIRE(input.empty());

        if (result == JobHttpParser::ParseResult::Complete)
            break;

        REQUIRE(result == JobHttpParser::ParseResult::NeedMoreData);
    }

    REQUIRE(result == JobHttpParser::ParseResult::Complete);
    REQUIRE(parser.isComplete());

    REQUIRE(parser.response().code() == 200);
    REQUIRE(parser.response().headers().value("x-test") == "fragmented");
    REQUIRE(parser.response().bodySize() == 11);

    const auto body = parser.response().bodyView();
    const std::string text(reinterpret_cast<const char *>(body.data()), body.size());

    REQUIRE(text == "Hello World");
}

TEST_CASE("JobHttpParser handles CRLF split across parse calls", "[job_net][http_parser][fragmentation][crlf]")
{
    JobHttpParser parser;

    std::string_view first = "HTTP/1.1 200 OK\r";

    REQUIRE(parser.parse(first) == JobHttpParser::ParseResult::NeedMoreData);
    REQUIRE(first.empty());

    std::string_view second =
        "\n"
        "Content-Length: 0\r";

    REQUIRE(parser.parse(second) == JobHttpParser::ParseResult::NeedMoreData);
    REQUIRE(second.empty());

    std::string_view third = "\n\r";

    REQUIRE(parser.parse(third) == JobHttpParser::ParseResult::NeedMoreData);
    REQUIRE(third.empty());

    std::string_view fourth = "\n";

    REQUIRE(parser.parse(fourth) == JobHttpParser::ParseResult::Complete);
    REQUIRE(fourth.empty());

    REQUIRE(parser.response().code() == 200);
}

TEST_CASE("JobHttpParser rejects malformed status lines", "[job_net][http_parser][status_line][error]")
{
    static constexpr std::string_view badStatusLines[] = {
        "NOTHTTP/1.1 200 OK\r\n\r\n",
        "HTTP/ 200 OK\r\n\r\n",
        "HTTP/1 200 OK\r\n\r\n",
        "HTTP/1.1 OK\r\n\r\n",
        "HTTP/1.1 20 OK\r\n\r\n",
        "HTTP/1.1 abc Nope\r\n\r\n",
        "HTTP/1.1 200OK\r\n\r\n"
    };

    for (const std::string_view bad : badStatusLines) {
        INFO("Status response: " << bad);

        JobHttpParser parser;
        std::string_view input = bad;

        REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Error);
        REQUIRE(parser.hasError());
    }
}

TEST_CASE("JobHttpParser rejects unsupported HTTP versions", "[job_net][http_parser][status_line][version][error]")
{
    static constexpr std::string_view unsupported[] = {
        "HTTP/0.9 200 OK\r\n\r\n",
        "HTTP/1.2 200 OK\r\n\r\n",
        "HTTP/2.0 200 OK\r\n\r\n"
    };

    for (const std::string_view response : unsupported) {
        INFO("Response: " << response);

        JobHttpParser parser;
        std::string_view input = response;

        REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Error);
        REQUIRE(parser.hasError());
    }
}

TEST_CASE("JobHttpParser rejects obsolete folded header fields", "[job_net][http_parser][headers][error][obs_fold]")
{
    JobHttpParser parser;

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "X-Test: one\r\n"
        " two\r\n"
        "\r\n";

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Error);
    REQUIRE(parser.hasError());
}

TEST_CASE("JobHttpParser rejects invalid header syntax", "[job_net][http_parser][headers][error]")
{
    static constexpr std::string_view badHeaders[] = {
        "NoColon\r\n",
        ": value\r\n",
        "Bad Name: value\r\n",
        "Bad\tName: value\r\n"
    };

    for (const std::string_view header : badHeaders) {
        INFO("Header: " << header);

        JobHttpParser parser;

        std::string input = "HTTP/1.1 200 OK\r\n";
        input += header;
        input += "\r\n";

        std::string_view view = input;

        REQUIRE(parser.parse(view) == JobHttpParser::ParseResult::Error);
        REQUIRE(parser.hasError());
    }
}

TEST_CASE("JobHttpParser reset clears message state and preserves parser configuration", "[job_net][http_parser][usage][reset]")
{
    JobHttpParser parser;

    std::string streamed;

    parser.setRequestMethod(HttpMethod::Head);
    parser.setBodyMode(JobHttpParser::BodyMode::Stream);
    parser.setBodyCallback([&](std::span<const std::byte> data) {
        streamed.append(reinterpret_cast<const char *>(data.data()), data.size());
    });

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 5\r\n"
        "\r\n";

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Complete);

    REQUIRE(parser.requestMethod() == HttpMethod::Head);
    REQUIRE(parser.bodyMode() == JobHttpParser::BodyMode::Stream);

    parser.reset();

    REQUIRE(parser.state() == JobHttpParser::State::StatusLine);
    REQUIRE(parser.bodyFraming() == JobHttpParser::BodyFraming::None);

    REQUIRE_FALSE(parser.isComplete());
    REQUIRE_FALSE(parser.hasError());
    REQUIRE(parser.lastError().empty());

    REQUIRE(parser.response().code() == 0);
    REQUIRE(parser.response().headers().isEmpty());
    REQUIRE(parser.response().trailers().isEmpty());
    REQUIRE(parser.response().body().empty());

    REQUIRE(parser.bytesReceived() == 0);
    REQUIRE(parser.bodyBytesReceived() == 0);

    REQUIRE_FALSE(parser.hasContentLength());
    REQUIRE(parser.contentLength() == 0);

    REQUIRE(parser.requestMethod() == HttpMethod::Head);
    REQUIRE(parser.bodyMode() == JobHttpParser::BodyMode::Stream);
}

TEST_CASE("JobHttpParser error state remains sticky until reset", "[job_net][http_parser][edge][error_state]")
{
    JobHttpParser parser;

    std::string_view bad =
        "BROKEN\r\n"
        "\r\n";

    REQUIRE(parser.parse(bad) == JobHttpParser::ParseResult::Error);
    REQUIRE(parser.hasError());

    const std::string originalError = parser.lastError();

    std::string_view good =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 0\r\n"
        "\r\n";

    REQUIRE(parser.parse(good) == JobHttpParser::ParseResult::Error);
    REQUIRE(parser.lastError() == originalError);

    parser.reset();

    REQUIRE_FALSE(parser.hasError());

    REQUIRE(parser.parse(good) == JobHttpParser::ParseResult::Complete);
    REQUIRE(parser.response().code() == 200);
}

TEST_CASE("JobHttpParser shared and unique factories create fresh parsers", "[job_net][http_parser][usage][factory]")
{
    const auto shared = JobHttpParser::createShared();
    const auto unique = JobHttpParser::createUniq();

    REQUIRE(shared);
    REQUIRE(unique);

    REQUIRE(shared->state() == JobHttpParser::State::StatusLine);
    REQUIRE(unique->state() == JobHttpParser::State::StatusLine);

    REQUIRE(shared->bodyMode() == JobHttpParser::BodyMode::Store);
    REQUIRE(unique->bodyMode() == JobHttpParser::BodyMode::Store);

    REQUIRE_FALSE(shared->isComplete());
    REQUIRE_FALSE(unique->isComplete());

    REQUIRE_FALSE(shared->hasError());
    REQUIRE_FALSE(unique->hasError());
}

TEST_CASE("JobHttpParser rejects overflowing chunk sizes", "[job_net][http_parser][chunked][overflow][error]")
{
    JobHttpParser parser;

    std::string_view input =
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "FFFFFFFFFFFFFFFF0\r\n";

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Error);
    REQUIRE(parser.hasError());
    REQUIRE(parser.state() == JobHttpParser::State::Error);
    REQUIRE_FALSE(parser.lastError().empty());
}

TEST_CASE("JobHttpParser rejects header lines larger than its fixed line buffer", "[job_net][http_parser][headers][line_limit][error]")
{
    JobHttpParser parser;

    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "X-Oversized: ";

    response.append(JobHttpParser::kLineBufferSize, 'A');

    std::string_view input = response;

    REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Error);
    REQUIRE(parser.hasError());
    REQUIRE(parser.state() == JobHttpParser::State::Error);
    REQUIRE_FALSE(parser.lastError().empty());
}
#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("JobHttpParser Content-Length response performance", "[job_net][http_parser][benchmark][content_length]")
{
    static const std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 11\r\n"
        "Content-Type: text/plain\r\n"
        "Server: JOB\r\n"
        "\r\n"
        "Hello World";

    BENCHMARK("HTTP/1.1 Content-Length response in one buffer")
    {
        JobHttpParser parser;
        std::string_view input = response;

        return parser.parse(input);
    };
}

TEST_CASE("JobHttpParser fragmented response performance", "[job_net][http_parser][benchmark][fragmentation]")
{
    static const std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 11\r\n"
        "Content-Type: text/plain\r\n"
        "X-Test: fragmented\r\n"
        "\r\n"
        "Hello World";

    BENCHMARK("HTTP/1.1 response one byte at a time")
    {
        JobHttpParser parser;
        JobHttpParser::ParseResult result = JobHttpParser::ParseResult::NeedMoreData;

        for (const char value : response) {
            const std::array<std::byte, 1> byte{
                static_cast<std::byte>(static_cast<unsigned char>(value))
            };

            std::span<const std::byte> input{byte};
            result = parser.parse(input);

            if (result != JobHttpParser::ParseResult::NeedMoreData)
                break;
        }

        return result;
    };
}

TEST_CASE("JobHttpParser chunked response performance", "[job_net][http_parser][benchmark][chunked]")
{
    static const std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "5\r\n"
        "Hello\r\n"
        "1\r\n"
        " \r\n"
        "5\r\n"
        "World\r\n"
        "0\r\n"
        "\r\n";

    BENCHMARK("HTTP/1.1 chunked response")
    {
        JobHttpParser parser;
        std::string_view input = response;

        return parser.parse(input);
    };
}

TEST_CASE("JobHttpParser larger body performance", "[job_net][http_parser][benchmark][body]")
{
    static const std::string body(64 * 1024, 'J');

    static const std::string response =
        std::string{
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 65536\r\n"
            "Content-Type: application/octet-stream\r\n"
            "\r\n"
        } + body;

    BENCHMARK("HTTP/1.1 64 KiB stored body")
    {
        JobHttpParser parser;
        std::string_view input = response;

        return parser.parse(input);
    };
}

TEST_CASE("JobHttpParser repeated lifecycle remains stable", "[job_net][http_parser][stress]")
{
    static constexpr std::size_t kIterations = 10000;

    static const std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 11\r\n"
        "X-Test: stress\r\n"
        "\r\n"
        "Hello World";

    for (std::size_t iteration = 0; iteration < kIterations; ++iteration) {
        INFO("HTTP parser lifecycle iteration: " << iteration);

        JobHttpParser parser;
        std::string_view input = response;

        REQUIRE(parser.parse(input) == JobHttpParser::ParseResult::Complete);
        REQUIRE(parser.response().code() == 200);
        REQUIRE(parser.response().bodySize() == 11);
        REQUIRE(input.empty());
    }
}

#endif








