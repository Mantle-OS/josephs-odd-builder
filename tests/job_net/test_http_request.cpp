#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <job_http_request.h>

using namespace job::net;

TEST_CASE("JobHttpRequest default state is an invalid empty GET request", "[job_net][http_request][usage][default]")
{
    JobHttpRequest request;

    REQUIRE(request.method() == HttpMethod::Get);
    REQUIRE(request.methodString() == "GET");
    REQUIRE(request.customMethod().empty());

    REQUIRE_FALSE(request.url().isValid());
    REQUIRE(request.headers().isEmpty());

    REQUIRE_FALSE(request.hasBody());
    REQUIRE(request.bodySize() == 0);
    REQUIRE(request.body().empty());
    REQUIRE(request.bodyView().empty());

    REQUIRE_FALSE(request.isValid());
}

TEST_CASE("JobHttpRequest URL constructor creates a GET request", "[job_net][http_request][usage][constructor]")
{
    const JobUrl url("https://example.com/api/items");

    REQUIRE(url.isValid());

    JobHttpRequest request(url);

    REQUIRE(request.method() == HttpMethod::Get);
    REQUIRE(request.methodString() == "GET");
    REQUIRE(request.customMethod().empty());

    REQUIRE(request.url() == url);
    REQUIRE(request.isValid());
}

TEST_CASE("JobHttpRequest method and URL constructor preserves the requested method", "[job_net][http_request][usage][constructor]")
{
    const JobUrl url("https://example.com/api/items");

    JobHttpRequest request(HttpMethod::Post, url);

    REQUIRE(request.method() == HttpMethod::Post);
    REQUIRE(request.methodString() == "POST");
    REQUIRE(request.customMethod().empty());
    REQUIRE(request.url() == url);
    REQUIRE(request.isValid());
}

TEST_CASE("JobHttpRequest custom method constructor creates a custom request", "[job_net][http_request][usage][custom]")
{
    const JobUrl url("https://example.com/resource");

    JobHttpRequest request("PURGE", url);

    REQUIRE(request.method() == HttpMethod::Custom);
    REQUIRE(request.methodString() == "PURGE");
    REQUIRE(request.customMethod() == "PURGE");
    REQUIRE(request.url() == url);
    REQUIRE(request.isValid());
}

TEST_CASE("JobHttpRequest switches between standard and custom methods", "[job_net][http_request][usage][method]")
{
    const JobUrl url("https://example.com/resource");

    JobHttpRequest request(HttpMethod::Post, url);

    REQUIRE(request.method() == HttpMethod::Post);
    REQUIRE(request.methodString() == "POST");
    REQUIRE(request.customMethod().empty());

    request.setCustomMethod("BREW");

    REQUIRE(request.method() == HttpMethod::Custom);
    REQUIRE(request.methodString() == "BREW");
    REQUIRE(request.customMethod() == "BREW");
    REQUIRE(request.isValid());

    request.setMethod(HttpMethod::Put);

    REQUIRE(request.method() == HttpMethod::Put);
    REQUIRE(request.methodString() == "PUT");
    REQUIRE(request.customMethod().empty());
    REQUIRE(request.isValid());
}

TEST_CASE("JobHttpRequest clearCustomMethod returns a custom request to GET", "[job_net][http_request][usage][custom][clear]")
{
    const JobUrl url("https://example.com/resource");

    JobHttpRequest request("PURGE", url);

    REQUIRE(request.method() == HttpMethod::Custom);
    REQUIRE(request.customMethod() == "PURGE");

    request.clearCustomMethod();

    REQUIRE(request.method() == HttpMethod::Get);
    REQUIRE(request.methodString() == "GET");
    REQUIRE(request.customMethod().empty());
    REQUIRE(request.isValid());
}

TEST_CASE("JobHttpRequest empty custom method is invalid", "[job_net][http_request][edge][custom][invalid]")
{
    const JobUrl url("https://example.com/resource");

    JobHttpRequest request(std::string_view{}, url);

    REQUIRE(request.method() == HttpMethod::Custom);
    REQUIRE(request.methodString().empty());
    REQUIRE(request.customMethod().empty());
    REQUIRE_FALSE(request.isValid());

    request.setCustomMethod("PURGE");

    REQUIRE(request.method() == HttpMethod::Custom);
    REQUIRE(request.methodString() == "PURGE");
    REQUIRE(request.isValid());

    request.setCustomMethod("");

    REQUIRE(request.method() == HttpMethod::Custom);
    REQUIRE(request.methodString().empty());
    REQUIRE_FALSE(request.isValid());
}

TEST_CASE("JobHttpRequest setMethod Custom does not invent a custom method", "[job_net][http_request][edge][custom]")
{
    const JobUrl url("https://example.com/resource");

    JobHttpRequest request(HttpMethod::Get, url);

    request.setMethod(HttpMethod::Custom);

    REQUIRE(request.method() == HttpMethod::Custom);
    REQUIRE(request.customMethod().empty());
    REQUIRE(request.methodString().empty());
    REQUIRE_FALSE(request.isValid());
}

TEST_CASE("JobHttpRequest setUrl supports copy and move assignment", "[job_net][http_request][usage][url]")
{
    JobHttpRequest request;

    const JobUrl first("https://example.com/first");

    REQUIRE(first.isValid());

    request.setUrl(first);

    REQUIRE(request.url() == first);
    REQUIRE(request.isValid());

    JobUrl second("http://example.org/second");

    REQUIRE(second.isValid());

    const JobUrl expected = second;

    request.setUrl(std::move(second));

    REQUIRE(request.url() == expected);
    REQUIRE(request.method() == HttpMethod::Get);
    REQUIRE(request.isValid());
}

TEST_CASE("JobHttpRequest exposes mutable headers and supports clearing them", "[job_net][http_request][usage][headers]")
{
    JobHttpRequest request(JobUrl("https://example.com/"));

    REQUIRE(request.headers().isEmpty());

    REQUIRE(request.headers().append("Accept", "application/json"));
    REQUIRE(request.headers().append("X-Test", "one"));

    REQUIRE(request.headers().size() == 2);
    REQUIRE(request.headers().value("accept") == "application/json");
    REQUIRE(request.headers().value("x-test") == "one");

    request.clearHeaders();

    REQUIRE(request.headers().isEmpty());
}

TEST_CASE("JobHttpRequest copies and moves header collections", "[job_net][http_request][usage][headers][copy_move]")
{
    JobHttpHeader headers;

    REQUIRE(headers.append("Accept", "application/json"));
    REQUIRE(headers.append("X-Test", "first"));
    REQUIRE(headers.append("X-Test", "second"));

    JobHttpRequest request(JobUrl("https://example.com/"));

    request.setHeaders(headers);

    REQUIRE(request.headers() == headers);
    REQUIRE(request.headers().count("x-test") == 2);

    JobHttpHeader movedHeaders;

    REQUIRE(movedHeaders.append("Content-Type", "application/octet-stream"));
    REQUIRE(movedHeaders.append("X-Moved", "yes"));

    request.setHeaders(std::move(movedHeaders));

    REQUIRE(request.headers().size() == 2);
    REQUIRE(request.headers().value("content-type") == "application/octet-stream");
    REQUIRE(request.headers().value("x-moved") == "yes");
}

TEST_CASE("JobHttpRequest stores a copied body", "[job_net][http_request][usage][body][copy]")
{
    JobHttpRequest request(JobUrl("https://example.com/upload"));

    const JobHttpRequest::Body body{
        std::byte{0x01},
        std::byte{0x02},
        std::byte{0x03},
        std::byte{0x04}
    };

    request.setBody(body);

    REQUIRE(request.hasBody());
    REQUIRE(request.bodySize() == body.size());
    REQUIRE(request.body() == body);

    const std::span<const std::byte> view = request.bodyView();

    REQUIRE(view.size() == body.size());
    REQUIRE(view[0] == std::byte{0x01});
    REQUIRE(view[1] == std::byte{0x02});
    REQUIRE(view[2] == std::byte{0x03});
    REQUIRE(view[3] == std::byte{0x04});
}

TEST_CASE("JobHttpRequest stores a moved body", "[job_net][http_request][usage][body][move]")
{
    JobHttpRequest request(JobUrl("https://example.com/upload"));

    JobHttpRequest::Body body{
        std::byte{0xaa},
        std::byte{0xbb},
        std::byte{0xcc}
    };

    const JobHttpRequest::Body expected = body;

    request.setBody(std::move(body));

    REQUIRE(request.hasBody());
    REQUIRE(request.bodySize() == expected.size());
    REQUIRE(request.body() == expected);
}

TEST_CASE("JobHttpRequest copies body bytes from a span", "[job_net][http_request][usage][body][span]")
{
    JobHttpRequest request(JobUrl("https://example.com/upload"));

    const std::array<std::byte, 5> source{
        std::byte{0x10},
        std::byte{0x20},
        std::byte{0x00},
        std::byte{0x30},
        std::byte{0xff}
    };

    request.setBody(std::span<const std::byte>{source});

    REQUIRE(request.hasBody());
    REQUIRE(request.bodySize() == source.size());

    const auto view = request.bodyView();

    REQUIRE(view.size() == source.size());

    for (std::size_t i = 0; i < source.size(); ++i)
        REQUIRE(view[i] == source[i]);
}

TEST_CASE("JobHttpRequest body preserves embedded zero bytes", "[job_net][http_request][edge][body][binary]")
{
    JobHttpRequest request(JobUrl("https://example.com/binary"));

    const std::array<std::byte, 7> body{
        std::byte{0x00},
        std::byte{0x41},
        std::byte{0x00},
        std::byte{0x42},
        std::byte{0xff},
        std::byte{0x00},
        std::byte{0x43}
    };

    request.setBody(std::span<const std::byte>{body});

    REQUIRE(request.bodySize() == body.size());
    REQUIRE(request.bodyView()[0] == std::byte{0x00});
    REQUIRE(request.bodyView()[2] == std::byte{0x00});
    REQUIRE(request.bodyView()[5] == std::byte{0x00});
    REQUIRE(request.bodyView()[6] == std::byte{0x43});
}

TEST_CASE("JobHttpRequest clearBody removes body state", "[job_net][http_request][usage][body][clear]")
{
    JobHttpRequest request(JobUrl("https://example.com/upload"));

    const std::array<std::byte, 3> body{
        std::byte{0x01},
        std::byte{0x02},
        std::byte{0x03}
    };

    request.setBody(std::span<const std::byte>{body});

    REQUIRE(request.hasBody());
    REQUIRE(request.bodySize() == 3);

    request.clearBody();

    REQUIRE_FALSE(request.hasBody());
    REQUIRE(request.bodySize() == 0);
    REQUIRE(request.body().empty());
    REQUIRE(request.bodyView().empty());
}

TEST_CASE("JobHttpRequest clear resets all request state", "[job_net][http_request][usage][clear]")
{
    JobHttpRequest request("PURGE", JobUrl("https://example.com/resource"));

    REQUIRE(request.headers().append("X-Test", "yes"));

    const std::array<std::byte, 2> body{
        std::byte{0xaa},
        std::byte{0xbb}
    };

    request.setBody(std::span<const std::byte>{body});

    REQUIRE(request.isValid());
    REQUIRE(request.method() == HttpMethod::Custom);
    REQUIRE_FALSE(request.headers().isEmpty());
    REQUIRE(request.hasBody());

    request.clear();

    REQUIRE(request.method() == HttpMethod::Get);
    REQUIRE(request.methodString() == "GET");
    REQUIRE(request.customMethod().empty());

    REQUIRE_FALSE(request.url().isValid());
    REQUIRE(request.headers().isEmpty());

    REQUIRE_FALSE(request.hasBody());
    REQUIRE(request.bodySize() == 0);

    REQUIRE_FALSE(request.isValid());
}

TEST_CASE("JobHttpRequest validity requires both a valid URL and usable method", "[job_net][http_request][edge][validity]")
{
    JobHttpRequest request;

    REQUIRE_FALSE(request.isValid());

    request.setUrl(JobUrl("https://example.com/"));

    REQUIRE(request.isValid());

    request.setMethod(HttpMethod::Custom);

    REQUIRE_FALSE(request.isValid());

    request.setCustomMethod("BREW");

    REQUIRE(request.isValid());

    request.setUrl(JobUrl{});

    REQUIRE_FALSE(request.isValid());
}

TEST_CASE("JobHttpRequest copy preserves value state", "[job_net][http_request][usage][copy]")
{
    JobHttpRequest original(HttpMethod::Post, JobUrl("https://example.com/api"));

    REQUIRE(original.headers().append("Content-Type", "application/octet-stream"));

    const std::array<std::byte, 3> body{
        std::byte{0x01},
        std::byte{0x02},
        std::byte{0x03}
    };

    original.setBody(std::span<const std::byte>{body});

    JobHttpRequest copy(original);

    REQUIRE(copy.method() == original.method());
    REQUIRE(copy.methodString() == original.methodString());
    REQUIRE(copy.url() == original.url());
    REQUIRE(copy.headers() == original.headers());
    REQUIRE(copy.body() == original.body());
    REQUIRE(copy.isValid());

    copy.setMethod(HttpMethod::Put);
    REQUIRE(copy.headers().set("Content-Type", "text/plain")); // JOSEPH
    copy.clearBody();

    REQUIRE(original.method() == HttpMethod::Post);
    REQUIRE(original.headers().value("content-type") == "application/octet-stream");
    REQUIRE(original.hasBody());
}

TEST_CASE("JobHttpRequest move preserves request contents", "[job_net][http_request][usage][move]")
{
    JobHttpRequest original("PURGE", JobUrl("https://example.com/cache"));

    REQUIRE(original.headers().append("X-Test", "move"));

    const std::array<std::byte, 2> body{
        std::byte{0xde},
        std::byte{0xad}
    };

    original.setBody(std::span<const std::byte>{body});

    JobHttpRequest moved(std::move(original));

    REQUIRE(moved.method() == HttpMethod::Custom);
    REQUIRE(moved.methodString() == "PURGE");
    REQUIRE(moved.url().isValid());
    REQUIRE(moved.headers().value("x-test") == "move");
    REQUIRE(moved.bodySize() == 2);
    REQUIRE(moved.bodyView()[0] == std::byte{0xde});
    REQUIRE(moved.bodyView()[1] == std::byte{0xad});
    REQUIRE(moved.isValid());
}

TEST_CASE("JobHttpRequest shared and unique factories preserve constructor semantics", "[job_net][http_request][usage][factory]")
{
    const JobUrl url("https://example.com/resource");

    const auto sharedDefault = JobHttpRequest::createShared();
    const auto sharedUrl = JobHttpRequest::createShared(url);
    const auto sharedMethod = JobHttpRequest::createShared(HttpMethod::Post, url);
    const auto sharedCustom = JobHttpRequest::createShared("PURGE", url);

    REQUIRE(sharedDefault);
    REQUIRE(sharedUrl);
    REQUIRE(sharedMethod);
    REQUIRE(sharedCustom);

    REQUIRE(sharedDefault->method() == HttpMethod::Get);
    REQUIRE_FALSE(sharedDefault->isValid());

    REQUIRE(sharedUrl->method() == HttpMethod::Get);
    REQUIRE(sharedUrl->url() == url);
    REQUIRE(sharedUrl->isValid());

    REQUIRE(sharedMethod->method() == HttpMethod::Post);
    REQUIRE(sharedMethod->methodString() == "POST");
    REQUIRE(sharedMethod->isValid());

    REQUIRE(sharedCustom->method() == HttpMethod::Custom);
    REQUIRE(sharedCustom->methodString() == "PURGE");
    REQUIRE(sharedCustom->isValid());

    const auto uniqueDefault = JobHttpRequest::createUniq();
    const auto uniqueUrl = JobHttpRequest::createUniq(url);
    const auto uniqueMethod = JobHttpRequest::createUniq(HttpMethod::Delete, url);
    const auto uniqueCustom = JobHttpRequest::createUniq("BREW", url);

    REQUIRE(uniqueDefault);
    REQUIRE(uniqueUrl);
    REQUIRE(uniqueMethod);
    REQUIRE(uniqueCustom);

    REQUIRE(uniqueDefault->method() == HttpMethod::Get);
    REQUIRE_FALSE(uniqueDefault->isValid());

    REQUIRE(uniqueUrl->method() == HttpMethod::Get);
    REQUIRE(uniqueUrl->isValid());

    REQUIRE(uniqueMethod->method() == HttpMethod::Delete);
    REQUIRE(uniqueMethod->methodString() == "DELETE");
    REQUIRE(uniqueMethod->isValid());

    REQUIRE(uniqueCustom->method() == HttpMethod::Custom);
    REQUIRE(uniqueCustom->methodString() == "BREW");
    REQUIRE(uniqueCustom->isValid());
}