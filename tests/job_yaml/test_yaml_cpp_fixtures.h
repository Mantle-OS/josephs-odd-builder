#pragma once

#include <string>

namespace job::yaml::tests {

struct YamlCppParserDestinationFixture
{
    std::string name;
    int count{};
    bool enabled{};
    double ratio{};
};

} // namespace job::yaml::tests