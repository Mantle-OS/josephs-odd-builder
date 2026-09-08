#pragma once

#include <string_view>

#include "job_yaml_object_reader.h"

namespace job::yaml {

class YamlObjectSink
{
public:
    template <typename T>
    static constexpr bool member(T &destination, std::string_view key, std::string_view value) noexcept
    {
        return YamlObjectReader::readScalar(destination, key, value);
    }
};

} // namespace job::yaml