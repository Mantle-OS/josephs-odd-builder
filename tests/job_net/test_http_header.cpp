#include <catch2/catch_test_macros.hpp>

#include <job_http_header.h>
#include <job_iana.h>

#include <string>
#include <string_view>
#include <utility>

using namespace job::net;

// ---------------------------------------------------------------------------
// Existing behaviour, still expected to hold
// ---------------------------------------------------------------------------

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
    REQUIRE(header.value("missing").empty());
    REQUIRE(header.value("missing", "fallback") == "fallback");
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
    REQUIRE(h2 == h1);

    JobHttpHeader h3 = std::move(h1);
    REQUIRE(h3.contains("x-test"));
    REQUIRE(h3.size() == 2);
    REQUIRE(h3 == h2);

    JobHttpHeader h4;
    h4 = h2;
    REQUIRE(h4 == h2);

    JobHttpHeader h5;
    h5 = std::move(h4);
    REQUIRE(h5 == h2);

    REQUIRE(h5 != JobHttpHeader{});
}

TEST_CASE("JobHttpHeader integration with JobIana headers", "[job_http_header][iana]")
{
    JobHttpHeader header;
    REQUIRE(header.append(JobIana::IanaHeaders::CacheControl, "no-cache"));
    REQUIRE(header.append(JobIana::IanaHeaders::ContentType, "text/plain"));

    REQUIRE(header.contains("cache-control"));
    REQUIRE(header.value(JobIana::IanaHeaders::ContentType) == "text/plain");

    const auto values = header.values("Content-Type");
    REQUIRE(values.size() == 1);
    REQUIRE(values.front() == "text/plain");
}

TEST_CASE("JobIana canonical dash formatting", "[job_iana][format]")
{
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
    REQUIRE(header.toString() == "\r\n");
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
    REQUIRE(h.nameAt(1) == "headerx");

    REQUIRE(h.replace(1, "HeaderX", "Updated"));
    REQUIRE(h.valueAt(1) == "Updated");

    REQUIRE(h.removeAt(1));
    REQUIRE(h.size() == 3);
    REQUIRE_FALSE(h.contains("headerx"));

    REQUIRE_FALSE(h.removeAt(99));
    REQUIRE(h.size() == 3);
}

TEST_CASE("JobHttpHeader invalid positions are handled", "[job_http_header][bounds]")
{
    JobHttpHeader h;
    // out of range
    REQUIRE_FALSE(h.replace(10, "Header", "Nope"));

    // must not crash
    REQUIRE_FALSE(h.removeAt(5));
    REQUIRE(h.size() == 0);

    REQUIRE(h.valueAt(0).empty());
    REQUIRE(h.nameAt(0).empty());
    REQUIRE(h.fieldAt(0) == nullptr);
    REQUIRE(h.indexOf("anything") == JobHttpHeader::npos);
    REQUIRE(h.lastIndexOf("anything") == JobHttpHeader::npos);
}

// ---------------------------------------------------------------------------
// Field-line storage: the point of the refactor
// ---------------------------------------------------------------------------

TEST_CASE("JobHttpHeader append creates independent field lines", "[job_http_header][repeat]")
{
    JobHttpHeader header;
    REQUIRE(header.append("Accept", "text/html"));
    REQUIRE(header.append("Accept", "application/json"));

    // Two lines, not one comma-joined value.
    REQUIRE(header.size() == 2);
    REQUIRE(header.count("Accept") == 2);
    REQUIRE(header.value("Accept") == "text/html");
    REQUIRE(header.lastValue("Accept") == "application/json");

    const auto values = header.values("accept");
    REQUIRE(values.size() == 2);
    REQUIRE(values[0] == "text/html");
    REQUIRE(values[1] == "application/json");

    REQUIRE(header.toString() == "Accept: text/html\r\nAccept: application/json\r\n\r\n");
}

TEST_CASE("JobHttpHeader Set-Cookie lines are never combined", "[job_http_header][set_cookie]")
{
    JobHttpHeader header;
    REQUIRE(header.append(JobIana::IanaHeaders::SetCookie, "foo=a"));
    REQUIRE(header.append("set-cookie", "bar=b; Expires=Wed, 21 Oct 2026 07:28:00 GMT"));

    REQUIRE(header.count(JobIana::IanaHeaders::SetCookie) == 2);

    const auto cookies = header.values(JobIana::IanaHeaders::SetCookie);
    REQUIRE(cookies.size() == 2);
    // Receive order, not reversed.
    REQUIRE(cookies[0] == "foo=a");
    REQUIRE(cookies[1] == "bar=b; Expires=Wed, 21 Oct 2026 07:28:00 GMT");

    // The comma inside the Expires date is exactly why joining is unsafe here.
    REQUIRE_FALSE(JobHttpHeader::isCombinable("Set-Cookie"));
    REQUIRE_FALSE(JobHttpHeader::isCombinable("set-cookie"));
    REQUIRE_FALSE(header.appendToList("Set-Cookie", "baz=c"));
    REQUIRE(header.count("set-cookie") == 2);

    REQUIRE(JobHttpHeader::isCombinable("Accept-Encoding"));
}

TEST_CASE("JobHttpHeader values preserves receive order across many lines", "[job_http_header][repeat]")
{
    JobHttpHeader header;
    REQUIRE(header.append("X-Trace", "1"));
    REQUIRE(header.append("Other", "-"));
    REQUIRE(header.append("x-trace", "2"));
    REQUIRE(header.append("X-TRACE", "3"));

    const auto values = header.values("x-trace");
    REQUIRE(values.size() == 3);
    REQUIRE(values[0] == "1");
    REQUIRE(values[1] == "2");
    REQUIRE(values[2] == "3");

    REQUIRE(header.indexOf("x-trace") == 0);
    REQUIRE(header.lastIndexOf("x-trace") == 3);
    REQUIRE(header.joinedValue("X-Trace") == "1, 2, 3");
    REQUIRE(header.joinedValue("X-Trace", ",") == "1,2,3");
    REQUIRE(header.joinedValue("absent").empty());
}

// ---------------------------------------------------------------------------
// Framing: what parseContentLength() relies on
// ---------------------------------------------------------------------------

TEST_CASE("JobHttpHeader listMembers flattens repeated and comma lists", "[job_http_header][framing]")
{
    SECTION("two agreeing field lines")
    {
        JobHttpHeader h;
        REQUIRE(h.append("Content-Length", "42"));
        REQUIRE(h.append("Content-Length", "42"));

        const auto members = h.listMembers(JobIana::IanaHeaders::ContentLength);
        REQUIRE(members.size() == 2);
        REQUIRE(members[0] == "42");
        REQUIRE(members[1] == "42");
    }

    SECTION("one comma-separated line yields the same view")
    {
        JobHttpHeader h;
        REQUIRE(h.append("Content-Length", "42, 42"));

        const auto members = h.listMembers("content-length");
        REQUIRE(members.size() == 2);
        REQUIRE(members[0] == "42");
        REQUIRE(members[1] == "42");
    }

    SECTION("disagreeing values stay visible so the parser can reject")
    {
        JobHttpHeader h;
        REQUIRE(h.append("Content-Length", "42"));
        REQUIRE(h.append("Content-Length", "123"));

        const auto members = h.listMembers("Content-Length");
        REQUIRE(members.size() == 2);
        REQUIRE(members[0] == "42");
        REQUIRE(members[1] == "123");
        REQUIRE(members[0] != members[1]);
    }

    SECTION("empty members are preserved, not silently dropped")
    {
        JobHttpHeader h;
        REQUIRE(h.append("Content-Length", "42,,42"));

        const auto members = h.listMembers("Content-Length");
        REQUIRE(members.size() == 3);
        REQUIRE(members[0] == "42");
        REQUIRE(members[1].empty());
        REQUIRE(members[2] == "42");
    }

    SECTION("mixed lines and lists, OWS trimmed per member")
    {
        JobHttpHeader h;
        REQUIRE(h.append("Trailer", "  a ,\tb  "));
        REQUIRE(h.append("trailer", " c "));

        const auto members = h.listMembers("Trailer");
        REQUIRE(members.size() == 3);
        REQUIRE(members[0] == "a");
        REQUIRE(members[1] == "b");
        REQUIRE(members[2] == "c");
    }

    SECTION("absent field yields no members")
    {
        JobHttpHeader h;
        REQUIRE(h.listMembers("Content-Length").empty());
    }
}

// ---------------------------------------------------------------------------
// Mutation semantics
// ---------------------------------------------------------------------------

TEST_CASE("JobHttpHeader set collapses duplicates and keeps position", "[job_http_header][mutate]")
{
    JobHttpHeader h;
    REQUIRE(h.append("Accept", "text/html"));
    REQUIRE(h.append("Accept-Language", "en-US"));
    REQUIRE(h.append("accept", "application/json"));
    REQUIRE(h.append("Connection", "keep-alive"));
    REQUIRE(h.size() == 4);

    REQUIRE(h.set("Accept", "text/plain"));

    REQUIRE(h.size() == 3);
    REQUIRE(h.count("Accept") == 1);
    REQUIRE(h.value("Accept") == "text/plain");
    // Collapsed in place: the field keeps the first occurrence's slot.
    REQUIRE(h.nameAt(0) == "accept");
    REQUIRE(h.nameAt(1) == "accept-language");
    REQUIRE(h.nameAt(2) == "connection");

    // set() on an absent name appends.
    REQUIRE(h.set("X-New", "v"));
    REQUIRE(h.nameAt(3) == "x-new");
}

TEST_CASE("JobHttpHeader appendToList performs RFC 9110 list append", "[job_http_header][mutate]")
{
    JobHttpHeader h;
    REQUIRE(h.appendToList(JobIana::IanaHeaders::AcceptEncoding, "gzip"));
    REQUIRE(h.size() == 1);

    REQUIRE(h.appendToList("accept-encoding", "br"));
    REQUIRE(h.size() == 1);
    REQUIRE(h.value("Accept-Encoding") == "gzip, br");

    // Extends the LAST line when the name is already repeated.
    REQUIRE(h.append("Accept-Encoding", "deflate"));
    REQUIRE(h.appendToList("Accept-Encoding", "zstd"));
    REQUIRE(h.count("Accept-Encoding") == 2);
    REQUIRE(h.lastValue("Accept-Encoding") == "deflate, zstd");
}

TEST_CASE("JobHttpHeader prepend and insert add new lines", "[job_http_header][mutate]")
{
    JobHttpHeader h;
    REQUIRE(h.append("B", "2"));
    REQUIRE(h.prepend("A", "1"));
    REQUIRE(h.nameAt(0) == "a");
    REQUIRE(h.nameAt(1) == "b");

    // prepend does not merge into an existing line
    REQUIRE(h.prepend("B", "0"));
    REQUIRE(h.size() == 3);
    REQUIRE(h.count("B") == 2);
    REQUIRE(h.valueAt(0) == "0");
    REQUIRE(h.value("B") == "0");
    REQUIRE(h.lastValue("B") == "2");

    // insert past the end clamps to append
    REQUIRE(h.insert("C", "3", 999));
    REQUIRE(h.nameAt(h.size() - 1) == "c");

    // insert with an existing name adds a line rather than overwriting
    REQUIRE(h.insert("C", "4", 0));
    REQUIRE(h.count("C") == 2);
    REQUIRE(h.nameAt(0) == "c");
}

TEST_CASE("JobHttpHeader replace overwrites a single line", "[job_http_header][mutate]")
{
    JobHttpHeader h;
    REQUIRE(h.append("X", "1"));
    REQUIRE(h.append("X", "2"));

    REQUIRE(h.replace(0, "Y", "changed"));
    REQUIRE(h.size() == 2);
    REQUIRE(h.nameAt(0) == "y");
    REQUIRE(h.nameAt(1) == "x");
    REQUIRE(h.count("X") == 1);

    REQUIRE_FALSE(h.replace(0, "Bad Name", "v"));
    REQUIRE(h.nameAt(0) == "y");
}

TEST_CASE("JobHttpHeader removeAll removes every matching line", "[job_http_header][mutate]")
{
    JobHttpHeader h;
    REQUIRE(h.append("Via", "1.1 a"));
    REQUIRE(h.append("Keep", "me"));
    REQUIRE(h.append("via", "1.1 b"));
    REQUIRE(h.append("VIA", "1.1 c"));

    REQUIRE(h.removeAll("Via") == 3);
    REQUIRE(h.size() == 1);
    REQUIRE(h.nameAt(0) == "keep");
    REQUIRE_FALSE(h.contains("Via"));

    REQUIRE(h.removeAll("Via") == 0);
    REQUIRE(h.removeAll(JobIana::IanaHeaders::SetCookie) == 0);
}

// ---------------------------------------------------------------------------
// Validation / injection
// ---------------------------------------------------------------------------

TEST_CASE("JobHttpHeader rejects malformed field names", "[job_http_header][validation]")
{
    JobHttpHeader h;

    REQUIRE_FALSE(h.append("", "v"));
    REQUIRE_FALSE(h.append("Bad Name", "v"));
    REQUIRE_FALSE(h.append("Bad:Name", "v"));
    REQUIRE_FALSE(h.append("Trailing ", "v"));
    REQUIRE_FALSE(h.append("New\nLine", "v"));
    REQUIRE_FALSE(h.set("Bad Name", "v"));
    REQUIRE_FALSE(h.insert("Bad Name", "v", 0));
    REQUIRE_FALSE(h.prepend("Bad Name", "v"));
    REQUIRE(h.isEmpty());

    REQUIRE(JobHttpHeader::isValidFieldName("X-Custom_Name.1"));
    REQUIRE_FALSE(JobHttpHeader::isValidFieldName(""));
    REQUIRE_FALSE(JobHttpHeader::isValidFieldName("has space"));
}

TEST_CASE("JobHttpHeader rejects header injection in values", "[job_http_header][validation][security]")
{
    JobHttpHeader h;

    REQUIRE_FALSE(h.append("X-Evil", "ok\r\nInjected: yes"));
    REQUIRE_FALSE(h.append("X-Evil", "ok\nInjected: yes"));
    REQUIRE_FALSE(h.append("X-Evil", std::string_view{"ok\0bad", 6}));
    REQUIRE_FALSE(h.set("X-Evil", "ok\r\nInjected: yes"));
    REQUIRE_FALSE(h.appendToList("X-Evil", "ok\r\nInjected: yes"));
    REQUIRE(h.isEmpty());

    REQUIRE(JobHttpHeader::isValidFieldValue("plain value"));
    REQUIRE(JobHttpHeader::isValidFieldValue("has\ttab"));
    REQUIRE_FALSE(JobHttpHeader::isValidFieldValue("has\rcr"));

    // A rejected value must not corrupt the surrounding block.
    REQUIRE(h.append("Host", "example.com"));
    REQUIRE_FALSE(h.append("X-Evil", "a\r\nb"));
    REQUIRE(h.toString() == "Host: example.com\r\n\r\n");
}

TEST_CASE("JobHttpHeader trims OWS around values on store", "[job_http_header][validation]")
{
    JobHttpHeader h;
    REQUIRE(h.append("X-Padded", "  \t value here \t "));
    REQUIRE(h.value("X-Padded") == "value here");

    // Empty and whitespace-only values are legal field values.
    REQUIRE(h.append("X-Empty", "   "));
    REQUIRE(h.value("X-Empty").empty());
    REQUIRE(h.contains("X-Empty"));
    REQUIRE(h.toString().find("X-Empty: \r\n") != std::string::npos);
}

// ---------------------------------------------------------------------------
// Names, casing, iteration
// ---------------------------------------------------------------------------

TEST_CASE("JobHttpHeader preserves display casing but matches case-insensitively",
          "[job_http_header][naming]")
{
    JobHttpHeader h;
    REQUIRE(h.append("X-Custom-Header", "v"));

    REQUIRE(h.nameAt(0) == "x-custom-header");
    REQUIRE(h.fieldAt(0) != nullptr);
    REQUIRE(h.fieldAt(0)->displayName == "X-Custom-Header");
    REQUIRE(h.fieldAt(0)->name == "x-custom-header");
    REQUIRE(h.fieldAt(0)->value == "v");

    REQUIRE(h.contains("X-CUSTOM-HEADER"));
    REQUIRE(h.contains("x-custom-header"));
    REQUIRE(h.toString() == "X-Custom-Header: v\r\n\r\n");

    REQUIRE(JobHttpHeader::normalizeKey("Content-TYPE") == "content-type");
    REQUIRE(JobHttpHeader::equalsIgnoreCase("Accept", "aCCePt"));
    REQUIRE_FALSE(JobHttpHeader::equalsIgnoreCase("Accept", "Accepts"));
}

TEST_CASE("JobHttpHeader iteration exposes every field line", "[job_http_header][iteration]")
{
    JobHttpHeader h;
    REQUIRE(h.append("Set-Cookie", "a=1"));
    REQUIRE(h.append("Set-Cookie", "b=2"));
    REQUIRE(h.append("Host", "example.com"));

    std::size_t seen = 0;
    std::size_t cookies = 0;
    for (const auto &field : h) {
        ++seen;
        if (field.name == "set-cookie")
            ++cookies;
    }

    REQUIRE(seen == h.size());
    REQUIRE(seen == 3);
    REQUIRE(cookies == 2);

    const JobHttpHeader &constRef = h;
    REQUIRE(std::distance(constRef.cbegin(), constRef.cend()) == 3);
}

TEST_CASE("JobHttpHeader equality is order sensitive", "[job_http_header][compare]")
{
    JobHttpHeader a;
    REQUIRE(a.append("Set-Cookie", "x=1"));
    REQUIRE(a.append("Set-Cookie", "y=2"));

    JobHttpHeader b;
    REQUIRE(b.append("Set-Cookie", "y=2"));
    REQUIRE(b.append("Set-Cookie", "x=1"));

    // Same names and values, different order: Set-Cookie order is meaningful.
    REQUIRE(a != b);

    JobHttpHeader c;
    REQUIRE(c.append("Set-Cookie", "x=1"));
    REQUIRE(c.append("Set-Cookie", "y=2"));
    REQUIRE(a == c);
}

TEST_CASE("JobHttpHeader two-argument constructor", "[job_http_header][basic]")
{
    const JobHttpHeader h{"Content-Type", "text/plain"};
    REQUIRE(h.size() == 1);
    REQUIRE(h.value("content-type") == "text/plain");

    // An invalid field leaves the container empty rather than half-built.
    const JobHttpHeader bad{"Bad Name", "v"};
    REQUIRE(bad.isEmpty());
}
