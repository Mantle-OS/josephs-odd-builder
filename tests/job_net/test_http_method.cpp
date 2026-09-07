#include <catch2/catch_test_macros.hpp>
#include <job_http_method.h>

using namespace job::net;

TEST_CASE("JobHttpMethod converts enum values to HTTP method strings", "[job_net][http_method][conversion][to_string]")
{
    REQUIRE(JobHttpMethod::toString(HttpMethod::Get) == "GET");
    REQUIRE(JobHttpMethod::toString(HttpMethod::Head) == "HEAD");
    REQUIRE(JobHttpMethod::toString(HttpMethod::Post) == "POST");
    REQUIRE(JobHttpMethod::toString(HttpMethod::Put) == "PUT");
    REQUIRE(JobHttpMethod::toString(HttpMethod::Delete) == "DELETE");
    REQUIRE(JobHttpMethod::toString(HttpMethod::Patch) == "PATCH");
    REQUIRE(JobHttpMethod::toString(HttpMethod::Options) == "OPTIONS");
    REQUIRE(JobHttpMethod::toString(HttpMethod::Connect) == "CONNECT");
    REQUIRE(JobHttpMethod::toString(HttpMethod::Trace) == "TRACE");
    REQUIRE(JobHttpMethod::toString(HttpMethod::Custom).empty());
}

TEST_CASE("JobHttpMethod converts HTTP method strings to enum values", "[job_net][http_method][conversion][to_method]")
{
    REQUIRE(JobHttpMethod::toMethod("GET") == HttpMethod::Get);
    REQUIRE(JobHttpMethod::toMethod("HEAD") == HttpMethod::Head);
    REQUIRE(JobHttpMethod::toMethod("POST") == HttpMethod::Post);
    REQUIRE(JobHttpMethod::toMethod("PUT") == HttpMethod::Put);
    REQUIRE(JobHttpMethod::toMethod("DELETE") == HttpMethod::Delete);
    REQUIRE(JobHttpMethod::toMethod("PATCH") == HttpMethod::Patch);
    REQUIRE(JobHttpMethod::toMethod("OPTIONS") == HttpMethod::Options);
    REQUIRE(JobHttpMethod::toMethod("CONNECT") == HttpMethod::Connect);
    REQUIRE(JobHttpMethod::toMethod("TRACE") == HttpMethod::Trace);
}

TEST_CASE("JobHttpMethod treats unknown and non-canonical method strings as custom", "[job_net][http_method][edge][custom]")
{
    REQUIRE(JobHttpMethod::toMethod("") == HttpMethod::Custom);
    REQUIRE(JobHttpMethod::toMethod("CUSTOM") == HttpMethod::Custom);
    REQUIRE(JobHttpMethod::toMethod("PURGE") == HttpMethod::Custom);
    REQUIRE(JobHttpMethod::toMethod("get") == HttpMethod::Custom);
    REQUIRE(JobHttpMethod::toMethod("Get") == HttpMethod::Custom);
    REQUIRE(JobHttpMethod::toMethod(" GET") == HttpMethod::Custom);
    REQUIRE(JobHttpMethod::toMethod("GET ") == HttpMethod::Custom);
}

TEST_CASE("JobHttpMethod converts std strings through toMethodStd", "[job_net][http_method][conversion][std_string]")
{
    const std::string get{"GET"};
    const std::string post{"POST"};
    const std::string trace{"TRACE"};
    const std::string custom{"BREW"};
    const std::string lowercase{"get"};
    const std::string empty;

    REQUIRE(JobHttpMethod::toMethodStd(get) == HttpMethod::Get);
    REQUIRE(JobHttpMethod::toMethodStd(post) == HttpMethod::Post);
    REQUIRE(JobHttpMethod::toMethodStd(trace) == HttpMethod::Trace);
    REQUIRE(JobHttpMethod::toMethodStd(custom) == HttpMethod::Custom);
    REQUIRE(JobHttpMethod::toMethodStd(lowercase) == HttpMethod::Custom);
    REQUIRE(JobHttpMethod::toMethodStd(empty) == HttpMethod::Custom);
}

TEST_CASE("JobHttpMethod std string conversion matches string view conversion", "[job_net][http_method][conversion][std_string]")
{
    REQUIRE(JobHttpMethod::toStdString(HttpMethod::Get) == JobHttpMethod::toString(HttpMethod::Get));
    REQUIRE(JobHttpMethod::toStdString(HttpMethod::Head) == JobHttpMethod::toString(HttpMethod::Head));
    REQUIRE(JobHttpMethod::toStdString(HttpMethod::Post) == JobHttpMethod::toString(HttpMethod::Post));
    REQUIRE(JobHttpMethod::toStdString(HttpMethod::Put) == JobHttpMethod::toString(HttpMethod::Put));
    REQUIRE(JobHttpMethod::toStdString(HttpMethod::Delete) == JobHttpMethod::toString(HttpMethod::Delete));
    REQUIRE(JobHttpMethod::toStdString(HttpMethod::Patch) == JobHttpMethod::toString(HttpMethod::Patch));
    REQUIRE(JobHttpMethod::toStdString(HttpMethod::Options) == JobHttpMethod::toString(HttpMethod::Options));
    REQUIRE(JobHttpMethod::toStdString(HttpMethod::Connect) == JobHttpMethod::toString(HttpMethod::Connect));
    REQUIRE(JobHttpMethod::toStdString(HttpMethod::Trace) == JobHttpMethod::toString(HttpMethod::Trace));
    REQUIRE(JobHttpMethod::toStdString(HttpMethod::Custom).empty());
}

TEST_CASE("JobHttpMethod std string overload converts known and custom methods", "[job_net][http_method][conversion][std_string_overload]")
{
    const std::string get{"GET"};
    const std::string post{"POST"};
    const std::string custom{"BREW"};

    REQUIRE(JobHttpMethod::toMethod(get) == HttpMethod::Get);
    REQUIRE(JobHttpMethod::toMethod(post) == HttpMethod::Post);
    REQUIRE(JobHttpMethod::toMethod(custom) == HttpMethod::Custom);
}

TEST_CASE("JobHttpMethod known methods round trip", "[job_net][http_method][round_trip]")
{
    static constexpr HttpMethod methods[] = {
        HttpMethod::Get,
        HttpMethod::Head,
        HttpMethod::Post,
        HttpMethod::Put,
        HttpMethod::Delete,
        HttpMethod::Patch,
        HttpMethod::Options,
        HttpMethod::Connect,
        HttpMethod::Trace
    };

    for (const HttpMethod method : methods) {
        INFO("HTTP method value: " << static_cast<int>(method));
        REQUIRE(JobHttpMethod::toMethod(JobHttpMethod::toString(method)) == method);
    }
}