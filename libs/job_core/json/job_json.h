#pragma once

#include <string_view>
#include <utility>
#include "job_json_diagnostic.h"
#include "job_json_emitter.h"
#include "job_json_parser.h"

namespace job::json {

class JobJson
{
public:
    JobJson() = delete;
    ~JobJson() = delete;

    JobJson(const JobJson &) = delete;
    JobJson &operator=(const JobJson &) = delete;
    JobJson(JobJson &&) = delete;
    JobJson &operator=(JobJson &&) = delete;

    template <JsonValue T>
    [[nodiscard]] static bool fromJson(std::string_view source, T &value)
    {
        JsonParser parser{source};
        return parser.parse(value);
    }

    template <JsonValue T>
    [[nodiscard]] static bool fromJson(std::string_view source, T &value, JsonDiagnostic &diagnostic)
    {
        JsonParser parser{source};

        if (parser.parse(value)) {
            diagnostic = {};
            return true;
        }

        diagnostic = parser.diagnostic();
        return false;
    }

    template <JsonValue T>
    [[nodiscard]] static bool toJson(const T &value, std::string &output)
    {
        output.clear();
        return JsonEmitter::emit(value, output);
    }

    template <JsonValue T, JsonOutputSink Sink>
    [[nodiscard]] static bool emit(const T &value, Sink &sink)
    {
        return JsonEmitter::emit(value, sink);
    }
};

} // namespace job::json