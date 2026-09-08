#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "test_job_json_fixtures.h"

    namespace job::json::tests {

    [[nodiscard]] inline JsonRoundTripFixture makeNlohmannBenchmarkFixture()
    {
        JsonRoundTripFixture value;

        value.id = 42;
        value.ratio = 1.25;
        value.enabled = true;
        value.name = "Cake Court 世界 😀";
        value.optionalCount = 7;
        value.values = {
            10,
            20,
            30,
            40,
            50,
            60,
            70,
            80
        };
        value.nested.port = 8080;
        value.nested.host = "localhost";

        return value;
    }

    inline constexpr std::string_view NlohmannBenchmarkJson =
        R"({"id":42,"ratio":1.25,"enabled":true,"name":"Cake Court 世界 😀","optionalCount":7,"values":[10,20,30,40,50,60,70,80],"nested":{"port":8080,"host":"localhost"}})";

    inline void to_json(nlohmann::json &json, const ParserNestedObject &value)
    {
        json = nlohmann::json{
            {"port", value.port},
            {"host", value.host}
        };
    }

    inline void from_json(const nlohmann::json &json, ParserNestedObject &value)
    {
        json.at("port").get_to(value.port);
        json.at("host").get_to(value.host);
    }

    inline void to_json(nlohmann::json &json, const JsonRoundTripFixture &value)
    {
        json = nlohmann::json{
            {"id", value.id},
            {"ratio", value.ratio},
            {"enabled", value.enabled},
            {"name", value.name},
            {"optionalCount", value.optionalCount},
            {"values", value.values},
            {"nested", value.nested}
        };
    }

    inline void from_json(const nlohmann::json &json, JsonRoundTripFixture &value)
    {
        json.at("id").get_to(value.id);
        json.at("ratio").get_to(value.ratio);
        json.at("enabled").get_to(value.enabled);
        json.at("name").get_to(value.name);

        const auto &optionalCount = json.at("optionalCount");

        if (optionalCount.is_null()) {
            value.optionalCount.reset();
        } else {
            value.optionalCount = optionalCount.get<int>();
        }

        json.at("values").get_to(value.values);
        json.at("nested").get_to(value.nested);
    }

    [[nodiscard]] inline bool benchmarkFixturesEqual(
        const JsonRoundTripFixture &lhs,
        const JsonRoundTripFixture &rhs)
    {
        return
            lhs.id == rhs.id &&
            lhs.ratio == rhs.ratio &&
            lhs.enabled == rhs.enabled &&
            lhs.name == rhs.name &&
            lhs.optionalCount == rhs.optionalCount &&
            lhs.values == rhs.values &&
            lhs.nested.port == rhs.nested.port &&
            lhs.nested.host == rhs.nested.host;
    }

} // namespace job::json::tests

