#include <catch2/catch_test_macros.hpp>

#include <job_http_status.h>

using namespace job::net;

TEST_CASE("JobHttpStatus default state is unknown and unclassified", "[job_net][http_status][usage][default]")
{
    JobHttpStatus status;

    REQUIRE(status.code() == 0);
    REQUIRE(status.status() == JobHttpStatusCode::Unknown);
    REQUIRE(status.statusString().empty());
    REQUIRE_FALSE(status.isKnown());

    REQUIRE_FALSE(status.isInformational());
    REQUIRE_FALSE(status.isSuccess());
    REQUIRE_FALSE(status.isRedirection());
    REQUIRE_FALSE(status.isClientError());
    REQUIRE_FALSE(status.isServerError());
}

TEST_CASE("JobHttpStatus constructs from a numeric status code", "[job_net][http_status][usage][numeric]")
{
    JobHttpStatus status(200);

    REQUIRE(status.code() == 200);
    REQUIRE(status.status() == JobHttpStatusCode::Ok);
    REQUIRE(status.statusString() == "OK");
    REQUIRE(status.isKnown());
    REQUIRE(status.isSuccess());

    REQUIRE_FALSE(status.isInformational());
    REQUIRE_FALSE(status.isRedirection());
    REQUIRE_FALSE(status.isClientError());
    REQUIRE_FALSE(status.isServerError());
}



TEST_CASE("JobHttpStatus constructs from a known status enum", "[job_net][http_status][usage][enum]")
{
    JobHttpStatus status(JobHttpStatusCode::NotFound);

    REQUIRE(status.code() == 404);
    REQUIRE(status.status() == JobHttpStatusCode::NotFound);
    REQUIRE(status.statusString() == "Not Found");
    REQUIRE(status.isKnown());
    REQUIRE(status.isClientError());

    REQUIRE_FALSE(status.isInformational());
    REQUIRE_FALSE(status.isSuccess());
    REQUIRE_FALSE(status.isRedirection());
    REQUIRE_FALSE(status.isServerError());
}

TEST_CASE("JobHttpStatus preserves unknown numeric status codes", "[job_net][http_status][edge][unknown]")
{
    JobHttpStatus status(299);

    REQUIRE(status.code() == 299);
    REQUIRE(status.status() == JobHttpStatusCode::Unknown);
    REQUIRE(status.statusString().empty());
    REQUIRE_FALSE(status.isKnown());

    // Classification is numeric and deliberately independent of whether JOB
    // has a named enum value for the exact status code.
    REQUIRE(status.isSuccess());

    status.setCode(418);

    REQUIRE(status.code() == 418);
    REQUIRE(status.status() == JobHttpStatusCode::Unknown);
    REQUIRE(status.statusString().empty());
    REQUIRE_FALSE(status.isKnown());
    REQUIRE(status.isClientError());
}

TEST_CASE("JobHttpStatus classifies numeric status ranges", "[job_net][http_status][classification][boundaries]")
{
    SECTION("Below HTTP status range")
    {
        JobHttpStatus status(99);

        REQUIRE_FALSE(status.isInformational());
        REQUIRE_FALSE(status.isSuccess());
        REQUIRE_FALSE(status.isRedirection());
        REQUIRE_FALSE(status.isClientError());
        REQUIRE_FALSE(status.isServerError());
    }

    SECTION("Informational")
    {
        JobHttpStatus first(100);
        JobHttpStatus last(199);

        REQUIRE(first.isInformational());
        REQUIRE(last.isInformational());

        REQUIRE_FALSE(first.isSuccess());
        REQUIRE_FALSE(last.isSuccess());
    }

    SECTION("Success")
    {
        JobHttpStatus first(200);
        JobHttpStatus last(299);

        REQUIRE(first.isSuccess());
        REQUIRE(last.isSuccess());

        REQUIRE_FALSE(first.isInformational());
        REQUIRE_FALSE(last.isRedirection());
    }

    SECTION("Redirection")
    {
        JobHttpStatus first(300);
        JobHttpStatus last(399);

        REQUIRE(first.isRedirection());
        REQUIRE(last.isRedirection());

        REQUIRE_FALSE(first.isSuccess());
        REQUIRE_FALSE(last.isClientError());
    }

    SECTION("Client error")
    {
        JobHttpStatus first(400);
        JobHttpStatus last(499);

        REQUIRE(first.isClientError());
        REQUIRE(last.isClientError());

        REQUIRE_FALSE(first.isRedirection());
        REQUIRE_FALSE(last.isServerError());
    }

    SECTION("Server error")
    {
        JobHttpStatus first(500);
        JobHttpStatus last(599);

        REQUIRE(first.isServerError());
        REQUIRE(last.isServerError());

        REQUIRE_FALSE(first.isClientError());
    }

    SECTION("Above HTTP status range")
    {
        JobHttpStatus status(600);

        REQUIRE_FALSE(status.isInformational());
        REQUIRE_FALSE(status.isSuccess());
        REQUIRE_FALSE(status.isRedirection());
        REQUIRE_FALSE(status.isClientError());
        REQUIRE_FALSE(status.isServerError());
    }
}

TEST_CASE("JobHttpStatus setters switch between known and unknown codes", "[job_net][http_status][usage][setters]")
{
    JobHttpStatus status;

    status.setStatus(JobHttpStatusCode::Created);

    REQUIRE(status.code() == 201);
    REQUIRE(status.status() == JobHttpStatusCode::Created);
    REQUIRE(status.statusString() == "Created");
    REQUIRE(status.isKnown());
    REQUIRE(status.isSuccess());

    status.setCode(250);

    REQUIRE(status.code() == 250);
    REQUIRE(status.status() == JobHttpStatusCode::Unknown);
    REQUIRE(status.statusString().empty());
    REQUIRE_FALSE(status.isKnown());
    REQUIRE(status.isSuccess());

    status.setStatus(JobHttpStatusCode::InternalServerError);

    REQUIRE(status.code() == 500);
    REQUIRE(status.status() == JobHttpStatusCode::InternalServerError);
    REQUIRE(status.statusString() == "Internal Server Error");
    REQUIRE(status.isKnown());
    REQUIRE(status.isServerError());
}

TEST_CASE("JobHttpStatus compares by numeric code and mapped enum status", "[job_net][http_status][comparison]")
{
    const JobHttpStatus ok(200);
    const JobHttpStatus alsoOk(JobHttpStatusCode::Ok);
    const JobHttpStatus unknownSuccess(299);

    REQUIRE(ok == alsoOk);
    REQUIRE(ok == JobHttpStatusCode::Ok);
    REQUIRE(ok == static_cast<std::uint16_t>(200));

    REQUIRE_FALSE(ok == JobHttpStatusCode::Created);
    REQUIRE_FALSE(ok == static_cast<std::uint16_t>(201));

    REQUIRE(unknownSuccess == static_cast<std::uint16_t>(299));
    REQUIRE(unknownSuccess == JobHttpStatusCode::Unknown);

    REQUIRE(JobHttpStatus{} == JobHttpStatusCode::Unknown);
}
TEST_CASE("JobHttpStatus known representative codes expose their canonical names", "[job_net][http_status][conversion][string]")
{
    struct Case
    {
        JobHttpStatusCode status;
        std::uint16_t code;
        std::string_view text;
    };

    static constexpr Case cases[] = {
        {JobHttpStatusCode::Continue, 100, "Continue"},
        {JobHttpStatusCode::SwitchingProtocols, 101, "Switching Protocols"},
        {JobHttpStatusCode::Ok, 200, "OK"},
        {JobHttpStatusCode::NoContent, 204, "No Content"},
        {JobHttpStatusCode::MovedPermanently, 301, "Moved Permanently"},
        {JobHttpStatusCode::NotModified, 304, "Not Modified"},
        {JobHttpStatusCode::BadRequest, 400, "Bad Request"},
        {JobHttpStatusCode::NotFound, 404, "Not Found"},
        {JobHttpStatusCode::TooManyRequests, 429, "Too Many Requests"},
        {JobHttpStatusCode::InternalServerError, 500, "Internal Server Error"},
        {JobHttpStatusCode::ServiceUnavailable, 503, "Service Unavailable"},
        {JobHttpStatusCode::NetworkAuthenticationRequired, 511, "Network Authentication Required"}
    };

    for (const Case &test : cases) {
        INFO("HTTP status code: " << test.code);

        const JobHttpStatus status(test.status);

        REQUIRE(status.code() == test.code);
        REQUIRE(status.status() == test.status);
        REQUIRE(status.statusString() == test.text);
        REQUIRE(status.isKnown());
    }
}

TEST_CASE("JobHttpStatus shared and unique factories preserve constructor semantics", "[job_net][http_status][usage][factory]")
{
    const auto sharedDefault = JobHttpStatus::createShared();
    const auto sharedCode = JobHttpStatus::createShared(404);
    const auto sharedEnum = JobHttpStatus::createShared(JobHttpStatusCode::Created);

    REQUIRE(sharedDefault);
    REQUIRE(sharedCode);
    REQUIRE(sharedEnum);

    REQUIRE(sharedDefault->code() == 0);
    REQUIRE(sharedCode->code() == 404);
    REQUIRE(sharedCode->status() == JobHttpStatusCode::NotFound);
    REQUIRE(sharedEnum->code() == 201);
    REQUIRE(sharedEnum->status() == JobHttpStatusCode::Created);

    const auto uniqueDefault = JobHttpStatus::createUniq();
    const auto uniqueCode = JobHttpStatus::createUniq(503);
    const auto uniqueEnum = JobHttpStatus::createUniq(JobHttpStatusCode::NoContent);

    REQUIRE(uniqueDefault);
    REQUIRE(uniqueCode);
    REQUIRE(uniqueEnum);

    REQUIRE(uniqueDefault->code() == 0);
    REQUIRE(uniqueCode->code() == 503);
    REQUIRE(uniqueCode->status() == JobHttpStatusCode::ServiceUnavailable);
    REQUIRE(uniqueEnum->code() == 204);
    REQUIRE(uniqueEnum->status() == JobHttpStatusCode::NoContent);
}