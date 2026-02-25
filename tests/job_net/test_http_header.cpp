#include <catch2/catch_test_macros.hpp>

#include <job_http_header.h>
#include <job_iana.h>

using namespace job::net;

TEST_CASE("JobHttpHeader basic insert and retrieval", "[job_http_header][basic]")
{
    JobHttpHeader header;
    REQUIRE(header.isEmpty());

    REQUIRE(header.append("Content-Type", "application/json"));
    REQUIRE(header.append(JobIana::IanaHeaders::UserAgent, "JobLib/1.0"));

    REQUIRE(header.contains("content-type"));
    REQUIRE(header.contains(JobIana::IanaHeaders::UserAgent));
    REQUIRE(header.size() == 2);

    REQUIRE(header.value("Content-Type") == "application/json");
    REQUIRE(header.value(JobIana::IanaHeaders::UserAgent) == "JobLib/1.0");

    // Ensure case-insensitivity
    REQUIRE(header.value("USER-AGENT") == "JobLib/1.0");
}

TEST_CASE("JobHttpHeader append and replace semantics", "[job_http_header][mutate]")
{
    JobHttpHeader header;
    REQUIRE(header.append("Accept", "text/html"));
    REQUIRE(header.append("Accept", "application/json"));

    REQUIRE(header.value("Accept").find("application/json") != std::string_view::npos);

    REQUIRE(header.append("Accept", "text/html"));
    REQUIRE(header.append("Accept-Language", "en-US"));

    REQUIRE(header.replace(0, "Accept", "text/plain"));
    REQUIRE(header.value("Accept") == "text/plain");

    header.removeAll("Accept");
    REQUIRE_FALSE(header.contains("Accept"));
}

TEST_CASE("JobHttpHeader toString format output", "[job_http_header][serialization]")
{
    JobHttpHeader header;
    REQUIRE(header.append("Host", "example.com"));
    REQUIRE(header.append("Connection", "keep-alive"));
    REQUIRE(header.append("User-Agent", "JobTest/1.0"));

    const std::string serialized = header.toString();

    REQUIRE(serialized.find("Host: example.com") != std::string::npos);
    REQUIRE(serialized.find("Connection: keep-alive") != std::string::npos);
    REQUIRE(serialized.find("User-Agent: JobTest/1.0") != std::string::npos);
    // header block termination
    REQUIRE(serialized.ends_with("\r\n\r\n"));
}

TEST_CASE("JobHttpHeader copy and move semantics", "[job_http_header][copy][move]")
{
    JobHttpHeader h1;
    REQUIRE(h1.append("X-Test", "Alpha"));
    REQUIRE(h1.append("Y-Test", "Beta"));

    JobHttpHeader h2(h1);
    REQUIRE(h2.contains("x-test"));
    REQUIRE(h2.value("y-test") == "Beta");

    JobHttpHeader h3 = std::move(h1);
    REQUIRE(h3.contains("x-test"));
    REQUIRE(h3.size() == 2);
}

TEST_CASE("JobHttpHeader integration with JobIana headers", "[job_http_header][iana]")
{
    JobHttpHeader header;
    REQUIRE(header.append(JobIana::IanaHeaders::CacheControl, "no-cache"));
    REQUIRE(header.append(JobIana::IanaHeaders::ContentType, "text/plain"));

    REQUIRE(header.contains("cache-control"));
    REQUIRE(header.value(JobIana::IanaHeaders::ContentType) == "text/plain");

    auto values = header.values("Content-Type");
    REQUIRE(!values.empty());
}

TEST_CASE("JobIana canonical dash formatting", "[job_iana][format]") {
    REQUIRE(JobIana::kIanaHeaderNames.at(JobIana::IanaHeaders::CacheControl) == "Cache-Control");
    REQUIRE(JobIana::kIanaHeaderNames.at(JobIana::IanaHeaders::UserAgent) == "User-Agent");
    REQUIRE(JobIana::kIanaHeaderNames.at(JobIana::IanaHeaders::WWWAuthenticate) == "WWW-Authenticate");
    REQUIRE(JobIana::kIanaHeaderNames.at(JobIana::IanaHeaders::RetryAfter) == "Retry-After");
}

TEST_CASE("JobHttpHeader clear, count, and isEmpty", "[job_http_header][state]")
{
    JobHttpHeader header;
    REQUIRE(header.append("Header1", "A"));
    REQUIRE(header.append("Header2", "B"));
    REQUIRE_FALSE(header.isEmpty());
    REQUIRE(header.count() == header.size());

    header.clear();
    REQUIRE(header.isEmpty());
    REQUIRE(header.count() == 0);
}


TEST_CASE("JobHttpHeader positional operations", "[job_http_header][position]")
{
    JobHttpHeader h;
    REQUIRE(h.append("HeaderA", "One"));
    REQUIRE(h.append("HeaderB", "Two"));
    REQUIRE(h.append("HeaderC", "Three"));
    REQUIRE(h.size() == 3);

    REQUIRE(h.insert("HeaderX", "Inserted", 1));
    REQUIRE(h.size() == 4);
    REQUIRE(h.valueAt(1) == "Inserted");

    REQUIRE(h.replace(1, "HeaderX", "Updated"));
    REQUIRE(h.valueAt(1) == "Updated");

    h.removeAt(1);
    REQUIRE(h.size() == 3);
    REQUIRE_FALSE(h.contains("headerx"));

    h.removeAt(99);
    REQUIRE(h.size() == 3);
}


TEST_CASE("JobHttpHeader invalid positions are handled", "[job_http_header][bounds]")
{
    JobHttpHeader h;
    // out of range
    REQUIRE_FALSE(h.replace(10, "Header", "Nope"));

    // "SHOULD" NOT CRASH
    h.removeAt(5);
    REQUIRE(h.size() == 0);
}

