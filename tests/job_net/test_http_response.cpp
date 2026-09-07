#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>

#include <job_http_response.h>

using namespace job::net;

TEST_CASE("JobHttpResponse default state is empty and invalid", "[job_net][http_response][usage][default]")
{
    JobHttpResponse response;

    REQUIRE(response.code() == 0);
    REQUIRE(response.statusCode() == JobHttpStatusCode::Unknown);
    REQUIRE(response.reasonPhrase().empty());

    REQUIRE(response.headers().isEmpty());
    REQUIRE_FALSE(response.hasTrailers());
    REQUIRE(response.trailers().isEmpty());

    REQUIRE_FALSE(response.hasBody());
    REQUIRE(response.bodySize() == 0);
    REQUIRE(response.body().empty());
    REQUIRE(response.bodyView().empty());

    REQUIRE_FALSE(response.isInformational());
    REQUIRE_FALSE(response.isSuccessful());
    REQUIRE_FALSE(response.isRedirect());
    REQUIRE_FALSE(response.isClientError());
    REQUIRE_FALSE(response.isServerError());

    REQUIRE_FALSE(response.isValid());
}

TEST_CASE("JobHttpResponse constructs from numeric status codes", "[job_net][http_response][usage][constructor][numeric]")
{
    JobHttpResponse response(200);

    REQUIRE(response.code() == 200);
    REQUIRE(response.statusCode() == JobHttpStatusCode::Ok);
    REQUIRE(response.status().code() == 200);
    REQUIRE(response.reasonPhrase().empty());

    REQUIRE(response.isSuccessful());
    REQUIRE(response.isValid());
}

TEST_CASE("JobHttpResponse constructs from status enums", "[job_net][http_response][usage][constructor][enum]")
{
    JobHttpResponse response(JobHttpStatusCode::NotFound);

    REQUIRE(response.code() == 404);
    REQUIRE(response.statusCode() == JobHttpStatusCode::NotFound);
    REQUIRE(response.status().status() == JobHttpStatusCode::NotFound);

    REQUIRE(response.isClientError());
    REQUIRE(response.isValid());
}

TEST_CASE("JobHttpResponse constructors preserve explicit reason phrases", "[job_net][http_response][usage][reason]")
{
    SECTION("Numeric status")
    {
        JobHttpResponse response(299, "Something Interesting");

        REQUIRE(response.code() == 299);
        REQUIRE(response.statusCode() == JobHttpStatusCode::Unknown);
        REQUIRE(response.reasonPhrase() == "Something Interesting");
        REQUIRE(response.isSuccessful());
    }

    SECTION("Enum status")
    {
        JobHttpResponse response(JobHttpStatusCode::BadRequest, "Very Bad Request");

        REQUIRE(response.code() == 400);
        REQUIRE(response.statusCode() == JobHttpStatusCode::BadRequest);
        REQUIRE(response.reasonPhrase() == "Very Bad Request");
        REQUIRE(response.isClientError());
    }
}

TEST_CASE("JobHttpResponse preserves unknown numeric status codes", "[job_net][http_response][edge][unknown]")
{
    JobHttpResponse response(299, "Custom Success");

    REQUIRE(response.code() == 299);
    REQUIRE(response.statusCode() == JobHttpStatusCode::Unknown);
    REQUIRE(response.status().status() == JobHttpStatusCode::Unknown);
    REQUIRE(response.reasonPhrase() == "Custom Success");

    REQUIRE(response.isSuccessful());
    REQUIRE(response.isValid());

    response.setCode(418);

    REQUIRE(response.code() == 418);
    REQUIRE(response.statusCode() == JobHttpStatusCode::Unknown);
    REQUIRE(response.isClientError());
    REQUIRE(response.isValid());
}

TEST_CASE("JobHttpResponse status can be changed through numeric and enum setters", "[job_net][http_response][usage][status]")
{
    JobHttpResponse response;

    response.setCode(201);

    REQUIRE(response.code() == 201);
    REQUIRE(response.statusCode() == JobHttpStatusCode::Created);
    REQUIRE(response.isSuccessful());

    response.setStatusCode(JobHttpStatusCode::TemporaryRedirect);

    REQUIRE(response.code() == 307);
    REQUIRE(response.statusCode() == JobHttpStatusCode::TemporaryRedirect);
    REQUIRE(response.isRedirect());

    response.status().setCode(503);

    REQUIRE(response.code() == 503);
    REQUIRE(response.statusCode() == JobHttpStatusCode::ServiceUnavailable);
    REQUIRE(response.isServerError());
}

TEST_CASE("JobHttpResponse reason phrase can be replaced and cleared", "[job_net][http_response][usage][reason]")
{
    JobHttpResponse response(200);

    REQUIRE(response.reasonPhrase().empty());

    response.setReasonPhrase("OK");

    REQUIRE(response.reasonPhrase() == "OK");

    response.setReasonPhrase("Everything Is Fine");

    REQUIRE(response.reasonPhrase() == "Everything Is Fine");

    response.clearReasonPhrase();

    REQUIRE(response.reasonPhrase().empty());
}

TEST_CASE("JobHttpResponse exposes mutable headers and clears them independently", "[job_net][http_response][usage][headers]")
{
    JobHttpResponse response(200);

    REQUIRE(response.headers().append("Content-Type", "text/plain"));
    REQUIRE(response.headers().append("X-Test", "one"));
    REQUIRE(response.headers().append("X-Test", "two"));

    REQUIRE(response.headers().size() == 3);
    REQUIRE(response.headers().value("content-type") == "text/plain");
    REQUIRE(response.headers().count("x-test") == 2);

    response.clearHeaders();

    REQUIRE(response.headers().isEmpty());
    REQUIRE(response.code() == 200);
}

TEST_CASE("JobHttpResponse copies and moves headers", "[job_net][http_response][usage][headers][copy_move]")
{
    JobHttpHeader headers;

    REQUIRE(headers.append("Content-Type", "application/json"));
    REQUIRE(headers.append("X-Test", "first"));
    REQUIRE(headers.append("X-Test", "second"));

    JobHttpResponse response(200);

    response.setHeaders(headers);

    REQUIRE(response.headers() == headers);
    REQUIRE(response.headers().count("x-test") == 2);

    JobHttpHeader movedHeaders;

    REQUIRE(movedHeaders.append("Server", "JOB"));
    REQUIRE(movedHeaders.append("Cache-Control", "no-cache"));

    response.setHeaders(std::move(movedHeaders));

    REQUIRE(response.headers().size() == 2);
    REQUIRE(response.headers().value("server") == "JOB");
    REQUIRE(response.headers().value("cache-control") == "no-cache");
}

TEST_CASE("JobHttpResponse trailers are stored separately from initial headers", "[job_net][http_response][usage][trailers]")
{
    JobHttpResponse response(200);

    REQUIRE(response.headers().append("Content-Type", "text/plain"));
    REQUIRE(response.trailers().append("Digest", "sha-256=test"));

    REQUIRE(response.hasTrailers());

    REQUIRE(response.headers().size() == 1);
    REQUIRE(response.headers().contains("content-type"));
    REQUIRE_FALSE(response.headers().contains("digest"));

    REQUIRE(response.trailers().size() == 1);
    REQUIRE(response.trailers().contains("digest"));
    REQUIRE_FALSE(response.trailers().contains("content-type"));
}

TEST_CASE("JobHttpResponse copies moves and clears trailers", "[job_net][http_response][usage][trailers][copy_move]")
{
    JobHttpHeader trailers;

    REQUIRE(trailers.append("Digest", "sha-256=first"));
    REQUIRE(trailers.append("X-Trailer", "one"));

    JobHttpResponse response(200);

    response.setTrailers(trailers);

    REQUIRE(response.hasTrailers());
    REQUIRE(response.trailers() == trailers);

    JobHttpHeader movedTrailers;

    REQUIRE(movedTrailers.append("X-Moved-Trailer", "yes"));

    response.setTrailers(std::move(movedTrailers));

    REQUIRE(response.hasTrailers());
    REQUIRE(response.trailers().size() == 1);
    REQUIRE(response.trailers().value("x-moved-trailer") == "yes");

    response.clearTrailers();

    REQUIRE_FALSE(response.hasTrailers());
    REQUIRE(response.trailers().isEmpty());
}

TEST_CASE("JobHttpResponse stores a copied body", "[job_net][http_response][usage][body][copy]")
{
    JobHttpResponse response(200);

    const JobHttpResponse::Body body{
        std::byte{0x01},
        std::byte{0x02},
        std::byte{0x03},
        std::byte{0x04}
    };

    response.setBody(body);

    REQUIRE(response.hasBody());
    REQUIRE(response.bodySize() == body.size());
    REQUIRE(response.body() == body);

    const std::span<const std::byte> view = response.bodyView();

    REQUIRE(view.size() == body.size());
    REQUIRE(view[0] == std::byte{0x01});
    REQUIRE(view[1] == std::byte{0x02});
    REQUIRE(view[2] == std::byte{0x03});
    REQUIRE(view[3] == std::byte{0x04});
}

TEST_CASE("JobHttpResponse stores a moved body", "[job_net][http_response][usage][body][move]")
{
    JobHttpResponse response(200);

    JobHttpResponse::Body body{
        std::byte{0xaa},
        std::byte{0xbb},
        std::byte{0xcc}
    };

    const JobHttpResponse::Body expected = body;

    response.setBody(std::move(body));

    REQUIRE(response.hasBody());
    REQUIRE(response.bodySize() == expected.size());
    REQUIRE(response.body() == expected);
}

TEST_CASE("JobHttpResponse copies binary body bytes from a span", "[job_net][http_response][usage][body][span]")
{
    JobHttpResponse response(200);

    const std::array<std::byte, 6> source{
        std::byte{0x00},
        std::byte{0x10},
        std::byte{0x20},
        std::byte{0x00},
        std::byte{0xfe},
        std::byte{0xff}
    };

    response.setBody(std::span<const std::byte>{source});

    REQUIRE(response.hasBody());
    REQUIRE(response.bodySize() == source.size());

    const auto view = response.bodyView();

    REQUIRE(view.size() == source.size());

    for (std::size_t i = 0; i < source.size(); ++i)
        REQUIRE(view[i] == source[i]);
}

TEST_CASE("JobHttpResponse appendBody preserves chunk order", "[job_net][http_response][usage][body][append]")
{
    JobHttpResponse response(200);

    const std::array<std::byte, 3> first{
        std::byte{0x01},
        std::byte{0x02},
        std::byte{0x03}
    };

    const std::array<std::byte, 2> second{
        std::byte{0x04},
        std::byte{0x05}
    };

    const std::array<std::byte, 3> third{
        std::byte{0x00},
        std::byte{0xfe},
        std::byte{0xff}
    };

    response.appendBody(std::span<const std::byte>{first});
    response.appendBody(std::span<const std::byte>{second});
    response.appendBody(std::span<const std::byte>{third});

    REQUIRE(response.hasBody());
    REQUIRE(response.bodySize() == 8);

    const auto body = response.bodyView();

    REQUIRE(body[0] == std::byte{0x01});
    REQUIRE(body[1] == std::byte{0x02});
    REQUIRE(body[2] == std::byte{0x03});
    REQUIRE(body[3] == std::byte{0x04});
    REQUIRE(body[4] == std::byte{0x05});
    REQUIRE(body[5] == std::byte{0x00});
    REQUIRE(body[6] == std::byte{0xfe});
    REQUIRE(body[7] == std::byte{0xff});
}

TEST_CASE("JobHttpResponse appendBody accepts an empty span without changing state", "[job_net][http_response][edge][body][append]")
{
    JobHttpResponse response(200);

    const std::array<std::byte, 2> body{
        std::byte{0xaa},
        std::byte{0xbb}
    };

    response.setBody(std::span<const std::byte>{body});

    const std::span<const std::byte> empty;
    response.appendBody(empty);

    REQUIRE(response.bodySize() == 2);
    REQUIRE(response.bodyView()[0] == std::byte{0xaa});
    REQUIRE(response.bodyView()[1] == std::byte{0xbb});
}

TEST_CASE("JobHttpResponse clearBody removes only body state", "[job_net][http_response][usage][body][clear]")
{
    JobHttpResponse response(200, "OK");

    REQUIRE(response.headers().append("Content-Type", "application/octet-stream"));

    const std::array<std::byte, 3> body{
        std::byte{0x01},
        std::byte{0x02},
        std::byte{0x03}
    };

    response.setBody(std::span<const std::byte>{body});

    REQUIRE(response.hasBody());

    response.clearBody();

    REQUIRE_FALSE(response.hasBody());
    REQUIRE(response.bodySize() == 0);
    REQUIRE(response.body().empty());
    REQUIRE(response.bodyView().empty());

    REQUIRE(response.code() == 200);
    REQUIRE(response.reasonPhrase() == "OK");
    REQUIRE(response.headers().contains("content-type"));
}

TEST_CASE("JobHttpResponse delegates status classification", "[job_net][http_response][classification]")
{
    JobHttpResponse response;

    response.setCode(100);
    REQUIRE(response.isInformational());
    REQUIRE_FALSE(response.isSuccessful());

    response.setCode(200);
    REQUIRE(response.isSuccessful());
    REQUIRE_FALSE(response.isInformational());

    response.setCode(302);
    REQUIRE(response.isRedirect());
    REQUIRE_FALSE(response.isSuccessful());

    response.setCode(404);
    REQUIRE(response.isClientError());
    REQUIRE_FALSE(response.isRedirect());

    response.setCode(500);
    REQUIRE(response.isServerError());
    REQUIRE_FALSE(response.isClientError());

    response.setCode(600);
    REQUIRE_FALSE(response.isInformational());
    REQUIRE_FALSE(response.isSuccessful());
    REQUIRE_FALSE(response.isRedirect());
    REQUIRE_FALSE(response.isClientError());
    REQUIRE_FALSE(response.isServerError());
}

TEST_CASE("JobHttpResponse validity accepts only HTTP status range", "[job_net][http_response][edge][validity]")
{
    JobHttpResponse response;

    response.setCode(99);
    REQUIRE_FALSE(response.isValid());

    response.setCode(100);
    REQUIRE(response.isValid());

    response.setCode(199);
    REQUIRE(response.isValid());

    response.setCode(200);
    REQUIRE(response.isValid());

    response.setCode(599);
    REQUIRE(response.isValid());

    response.setCode(600);
    REQUIRE_FALSE(response.isValid());

    response.setCode(0);
    REQUIRE_FALSE(response.isValid());
}

TEST_CASE("JobHttpResponse clear resets all response state", "[job_net][http_response][usage][clear]")
{
    JobHttpResponse response(206, "Partial Content");

    REQUIRE(response.headers().append("Content-Type", "application/octet-stream"));
    REQUIRE(response.trailers().append("Digest", "sha-256=test"));

    const std::array<std::byte, 3> body{
        std::byte{0x10},
        std::byte{0x20},
        std::byte{0x30}
    };

    response.setBody(std::span<const std::byte>{body});

    REQUIRE(response.isValid());
    REQUIRE_FALSE(response.reasonPhrase().empty());
    REQUIRE_FALSE(response.headers().isEmpty());
    REQUIRE(response.hasTrailers());
    REQUIRE(response.hasBody());

    response.clear();

    REQUIRE(response.code() == 0);
    REQUIRE(response.statusCode() == JobHttpStatusCode::Unknown);
    REQUIRE(response.reasonPhrase().empty());

    REQUIRE(response.headers().isEmpty());
    REQUIRE_FALSE(response.hasTrailers());
    REQUIRE(response.trailers().isEmpty());

    REQUIRE_FALSE(response.hasBody());
    REQUIRE(response.bodySize() == 0);

    REQUIRE_FALSE(response.isValid());
}

TEST_CASE("JobHttpResponse copy preserves independent value state", "[job_net][http_response][usage][copy]")
{
    JobHttpResponse original(200, "OK");

    REQUIRE(original.headers().append("Content-Type", "application/octet-stream"));
    REQUIRE(original.trailers().append("Digest", "sha-256=test"));

    const std::array<std::byte, 3> body{
        std::byte{0x01},
        std::byte{0x02},
        std::byte{0x03}
    };

    original.setBody(std::span<const std::byte>{body});

    JobHttpResponse copy(original);

    REQUIRE(copy.code() == original.code());
    REQUIRE(copy.statusCode() == original.statusCode());
    REQUIRE(copy.reasonPhrase() == original.reasonPhrase());
    REQUIRE(copy.headers() == original.headers());
    REQUIRE(copy.trailers() == original.trailers());
    REQUIRE(copy.body() == original.body());

    copy.setCode(404);
    copy.setReasonPhrase("Not Found");
    REQUIRE(copy.headers().set("Content-Type", "text/plain")); // JOSEPH
    copy.clearTrailers();
    copy.clearBody();

    REQUIRE(original.code() == 200);
    REQUIRE(original.reasonPhrase() == "OK");
    REQUIRE(original.headers().value("content-type") == "application/octet-stream");
    REQUIRE(original.hasTrailers());
    REQUIRE(original.hasBody());
}

TEST_CASE("JobHttpResponse move preserves response contents", "[job_net][http_response][usage][move]")
{
    JobHttpResponse original(202, "Accepted");

    REQUIRE(original.headers().append("X-Test", "move"));
    REQUIRE(original.trailers().append("Digest", "sha-256=move"));

    const std::array<std::byte, 2> body{
        std::byte{0xde},
        std::byte{0xad}
    };

    original.setBody(std::span<const std::byte>{body});

    JobHttpResponse moved(std::move(original));

    REQUIRE(moved.code() == 202);
    REQUIRE(moved.statusCode() == JobHttpStatusCode::Accepted);
    REQUIRE(moved.reasonPhrase() == "Accepted");

    REQUIRE(moved.headers().value("x-test") == "move");
    REQUIRE(moved.trailers().value("digest") == "sha-256=move");

    REQUIRE(moved.bodySize() == 2);
    REQUIRE(moved.bodyView()[0] == std::byte{0xde});
    REQUIRE(moved.bodyView()[1] == std::byte{0xad});

    REQUIRE(moved.isValid());
}

TEST_CASE("JobHttpResponse shared factories preserve constructor semantics", "[job_net][http_response][usage][factory][shared]")
{
    const auto defaultResponse = JobHttpResponse::createShared();
    const auto codeResponse = JobHttpResponse::createShared(200);
    const auto enumResponse = JobHttpResponse::createShared(JobHttpStatusCode::NotFound);
    const auto codeReasonResponse = JobHttpResponse::createShared(299, "Custom Success");
    const auto enumReasonResponse = JobHttpResponse::createShared(JobHttpStatusCode::BadRequest, "Bad Request");

    REQUIRE(defaultResponse);
    REQUIRE(codeResponse);
    REQUIRE(enumResponse);
    REQUIRE(codeReasonResponse);
    REQUIRE(enumReasonResponse);

    REQUIRE(defaultResponse->code() == 0);
    REQUIRE_FALSE(defaultResponse->isValid());

    REQUIRE(codeResponse->code() == 200);
    REQUIRE(codeResponse->statusCode() == JobHttpStatusCode::Ok);

    REQUIRE(enumResponse->code() == 404);
    REQUIRE(enumResponse->statusCode() == JobHttpStatusCode::NotFound);

    REQUIRE(codeReasonResponse->code() == 299);
    REQUIRE(codeReasonResponse->statusCode() == JobHttpStatusCode::Unknown);
    REQUIRE(codeReasonResponse->reasonPhrase() == "Custom Success");

    REQUIRE(enumReasonResponse->code() == 400);
    REQUIRE(enumReasonResponse->reasonPhrase() == "Bad Request");
}

TEST_CASE("JobHttpResponse unique factories preserve constructor semantics", "[job_net][http_response][usage][factory][unique]")
{
    const auto defaultResponse = JobHttpResponse::createUniq();
    const auto codeResponse = JobHttpResponse::createUniq(204);
    const auto enumResponse = JobHttpResponse::createUniq(JobHttpStatusCode::ServiceUnavailable);
    const auto codeReasonResponse = JobHttpResponse::createUniq(250, "Unknown Success");
    const auto enumReasonResponse = JobHttpResponse::createUniq(JobHttpStatusCode::Created, "Created");

    REQUIRE(defaultResponse);
    REQUIRE(codeResponse);
    REQUIRE(enumResponse);
    REQUIRE(codeReasonResponse);
    REQUIRE(enumReasonResponse);

    REQUIRE(defaultResponse->code() == 0);

    REQUIRE(codeResponse->code() == 204);
    REQUIRE(codeResponse->statusCode() == JobHttpStatusCode::NoContent);

    REQUIRE(enumResponse->code() == 503);
    REQUIRE(enumResponse->statusCode() == JobHttpStatusCode::ServiceUnavailable);

    REQUIRE(codeReasonResponse->code() == 250);
    REQUIRE(codeReasonResponse->statusCode() == JobHttpStatusCode::Unknown);
    REQUIRE(codeReasonResponse->reasonPhrase() == "Unknown Success");

    REQUIRE(enumReasonResponse->code() == 201);
    REQUIRE(enumReasonResponse->statusCode() == JobHttpStatusCode::Created);
    REQUIRE(enumReasonResponse->reasonPhrase() == "Created");
}