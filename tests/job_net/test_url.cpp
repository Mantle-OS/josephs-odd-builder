#include <catch2/catch_test_macros.hpp>
#include <job_url.h>

using namespace job::net;

TEST_CASE("JobUrl basic parse and toString roundtrip", "[job_url][parse][roundtrip]") {
    std::string urlStr = "https://user:pass@example.com:8080/path/to/file?key=value#frag";
    JobUrl url(urlStr);

    REQUIRE(url.isValid());
    REQUIRE(url.scheme() == JobUrl::Scheme::Https);
    REQUIRE(url.host() == "example.com");
    REQUIRE(url.port() == 8080);
    REQUIRE(url.path() == "/path/to/file");
    REQUIRE(url.query() == "key=value");
    REQUIRE(url.fragment() == "frag");
    REQUIRE(url.username() == "user");

    // Verify that password masking works by default
    std::string fullMasked = url.toString(static_cast<uint8_t>(JobUrl::FormatOption::IncludePassword));
    REQUIRE(fullMasked.find("********") != std::string::npos);

    // Verify no double slash artifacts
    std::string plain = url.toString();
    REQUIRE(plain.rfind("https://", 0) == 0);
}

TEST_CASE("JobUrl scheme conversion functions work", "[job_url][scheme]") {
    REQUIRE(JobUrl::schemeToString(JobUrl::Scheme::Http) == "http");
    REQUIRE(JobUrl::schemeFromString("SSH") == JobUrl::Scheme::Ssh);
    REQUIRE(JobUrl::schemeFromString("unknown") == JobUrl::Scheme::Unknown);
}

TEST_CASE("JobUrl query encoding and decoding", "[job_url][query][encode]") {
    JobUrl url;
    url.setScheme("https");
    url.setHost("example.com");

    std::map<std::string, std::string> items = {
        {"key 1", "value 1"},
        {"param", "special&chars"}
    };
    url.setQueryItems(items);

    std::string encoded = url.query();
    REQUIRE(encoded.find("%20") != std::string::npos);
    REQUIRE(encoded.find("&") != 0); // '&' should separate items

    auto parsed = url.queryItems();
    REQUIRE(parsed.at("key 1") == "value 1");
    REQUIRE(parsed.at("param") == "special&chars");
}

TEST_CASE("JobUrl encode/decode components are symmetric", "[job_url][encoding]") {
    std::string original = "some text!*()~";
    std::string encoded = JobUrl::encodeComponent(original);
    std::string decoded = JobUrl::decodeComponent(encoded);
    REQUIRE(decoded == original);
}

TEST_CASE("JobUrl supports base64-encoded password output", "[job_url][password][base64]") {
    JobUrl url;
    url.setScheme(JobUrl::Scheme::Ssh);
    url.setHost("securehost");
    url.setUsername("admin");
    url.setPassword("s3cr3t");

    url.setbase64EncodePwd(true);
    url.setPasswdMode(JobUrl::PasswdMode::Strict);

    std::string base64String = url.password(true);
    REQUIRE(!base64String.empty());
    REQUIRE(base64String.find("s3cr3t") == std::string::npos); // must be encoded

    // Lenient mode returns decoded readable form
    url.setPasswdMode(JobUrl::PasswdMode::Lenient);
    std::string readable = url.password(true);
    REQUIRE(readable == "s3cr3t");
}

TEST_CASE("JobUrl handles missing components gracefully", "[job_url][minimal]") {
    JobUrl url("file:///etc/hosts");
    REQUIRE(url.scheme() == JobUrl::Scheme::File);
    REQUIRE(url.path() == "/etc/hosts");
    REQUIRE(url.host().empty());
    REQUIRE(url.isValid());

    JobUrl tcpUrl("tcp://127.0.0.1:22");
    REQUIRE(tcpUrl.scheme() == JobUrl::Scheme::Tcp);
    REQUIRE(tcpUrl.host() == "127.0.0.1");
    REQUIRE(tcpUrl.port() == 22);
}

TEST_CASE("JobUrl copy and move semantics preserve state", "[job_url][copy][move]") {
    JobUrl original("https://user:pw@copyhost/path");
    REQUIRE(original.isValid());

    JobUrl copy(original);
    REQUIRE(copy.host() == "copyhost");
    REQUIRE(copy.username() == "user");

    JobUrl moved(std::move(copy));
    REQUIRE(moved.host() == "copyhost");
    REQUIRE(moved.username() == "user");
}

TEST_CASE("JobUrl authority building", "[job_url][authority]") {
    JobUrl url("http://user:pass@server.com:8080/path");
    std::string auth = url.authority(true);

    REQUIRE(auth.find("user") != std::string::npos);
    REQUIRE(auth.find("server.com") != std::string::npos);
    REQUIRE(auth.find("8080") != std::string::npos);
}

TEST_CASE("JobUrl invalid URL handling", "[job_url][invalid]") {
    JobUrl url;
    url.setPasswdMode(JobUrl::PasswdMode::Lenient);

    REQUIRE_FALSE(url.parse("bad://no//authority")); //line 120
    REQUIRE_FALSE(url.isValid());

    url.setPasswdMode(JobUrl::PasswdMode::Strict);
    REQUIRE_THROWS_AS(url.parse("bad://no//authority"), std::invalid_argument);
}
