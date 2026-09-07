#pragma once

#include <contracts>

#include "jobyaml_export.h"

namespace job::yaml {

class JOBYAML_EXPORT YamlContracts {
public:
    using Violation = std::contracts::contract_violation;

    YamlContracts() = delete;
    ~YamlContracts() = delete;
    YamlContracts(const YamlContracts &) = delete;
    YamlContracts &operator=(const YamlContracts &) = delete;
    YamlContracts(YamlContracts &&) = delete;
    YamlContracts &operator=(YamlContracts &&) = delete;
};

} // namespace job::yaml