#include <catch2/catch_test_macros.hpp>
#include "socket/job_ipaddr.h"

using namespace job::io;

TEST_CASE("JobIpAddr IPv4 parsing and string conversion", "[job_ipaddr][ipv4]") {
    JobIpAddr addr("192.168.1.10", 8080);

    REQUIRE(addr.isValid());
    REQUIRE(addr.family() == JobIpAddr::Family::IPv4);
    REQUIRE(addr.port() == 8080);

    std::string str = addr.toString();
    REQUIRE(str == "192.168.1.10:8080");

    REQUIRE_FALSE(addr.isGlobal());
    REQUIRE(addr.isValidPort(8080));
    REQUIRE_FALSE(addr.isNull());
    REQUIRE_FALSE(addr.isMulticast());
    REQUIRE_FALSE(addr.isBroadcast());
}

TEST_CASE("JobIpAddr IPv6 parsing and conversion", "[job_ipaddr][ipv6]") {
    JobIpAddr addr("2001:0db8::1", 443);

    REQUIRE(addr.isValid());
    REQUIRE(addr.family() == JobIpAddr::Family::IPv6);
    REQUIRE(addr.port() == 443);

    std::string str = addr.toString();
    REQUIRE(str == "[2001:db8::1]:443");

    REQUIRE(addr.isGlobal());
    REQUIRE_FALSE(addr.isLoopback());
    REQUIRE_FALSE(addr.isMulticast());
    REQUIRE_FALSE(addr.isNull());
}

TEST_CASE("JobIpAddr UNIX socket parsing", "[job_ipaddr][unix]") {
    JobIpAddr addr("/tmp/test.sock");

    REQUIRE(addr.isValid());
    REQUIRE(addr.family() == JobIpAddr::Family::Unix);
    REQUIRE(addr.toString(false) == "/tmp/test.sock");
    REQUIRE(addr.isLocal());
    REQUIRE(addr.isUnixPermitted());
}

TEST_CASE("JobIpAddr loopback and local detection", "[job_ipaddr][loopback][local]") {
    JobIpAddr loop4("127.0.0.1");
    JobIpAddr loop6("::1");

    REQUIRE(loop4.isLoopback());
    REQUIRE(loop4.isLocal());
    REQUIRE_FALSE(loop4.isGlobal());
    REQUIRE_FALSE(loop4.isBroadcast());

    REQUIRE(loop6.isLoopback());
    REQUIRE(loop6.isLocal());
    REQUIRE_FALSE(loop6.isGlobal());
}

TEST_CASE("JobIpAddr multicast and broadcast detection", "[job_ipaddr][multicast][broadcast]") {
    JobIpAddr multi4("224.0.0.1");
    JobIpAddr bcast("255.255.255.255");

    REQUIRE(multi4.isMulticast());
    REQUIRE_FALSE(multi4.isBroadcast());
    REQUIRE(bcast.isBroadcast());
    REQUIRE_FALSE(bcast.isMulticast());
}

TEST_CASE("JobIpAddr invalid address handling", "[job_ipaddr][invalid]") {
    JobIpAddr addr("999.999.999.999");
    REQUIRE_FALSE(addr.isValid());
    REQUIRE(addr.family() == JobIpAddr::Family::Unknown);

    JobIpAddr empty("");
    REQUIRE_FALSE(empty.isValid());
    REQUIRE(empty.family() == JobIpAddr::Family::Unknown);

    REQUIRE_FALSE(JobIpAddr::isValidPort(0));
    REQUIRE(JobIpAddr::isValidPort(80));
    REQUIRE_FALSE(JobIpAddr::isValidPort(70000));
}

TEST_CASE("JobIpAddr classification: global vs local", "[job_ipaddr][global][local]") {
    JobIpAddr local1("10.0.0.1");
    JobIpAddr local2("172.20.0.5");
    JobIpAddr local3("192.168.5.10");
    JobIpAddr global("8.8.8.8");

    REQUIRE_FALSE(local1.isGlobal());
    REQUIRE_FALSE(local2.isGlobal());
    REQUIRE_FALSE(local3.isGlobal());
    REQUIRE(global.isGlobal());
}

TEST_CASE("JobIpAddr null and unspecified address", "[job_ipaddr][null]") {
    JobIpAddr null4("0.0.0.0");
    JobIpAddr null6("::");

    REQUIRE(null4.isNull());
    REQUIRE(null6.isNull());
    REQUIRE_FALSE(null4.isGlobal());
    REQUIRE_FALSE(null6.isGlobal());
}

TEST_CASE("JobIpAddr version string utility", "[job_ipaddr][utils]") {
    REQUIRE(JobIpAddr::versionString(JobIpAddr::Family::IPv4) == "IPv4");
    REQUIRE(JobIpAddr::versionString(JobIpAddr::Family::IPv6) == "IPv6");
    REQUIRE(JobIpAddr::versionString(JobIpAddr::Family::Unix) == "Unix");
    REQUIRE(JobIpAddr::versionString(JobIpAddr::Family::Unknown) == "Unknown");
}
