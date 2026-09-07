#pragma once

#include <string_view>

#include "job_yaml_alias_stack.h"
#include "job_yaml_anchor_table.h"
#include "job_yaml_diagnostic.h"

namespace job::yaml {

struct YamlParseContext
{
    std::string_view    source{};
    YamlAnchorTable     anchors{};
    YamlAliasStack      aliases{};
    YamlDiagnostic      diagnostic{};

    [[nodiscard]] constexpr bool hasSource() const noexcept
    {
        return !source.empty();
    }

    void clearDocument() noexcept
    {
        anchors.clear();
        aliases.clear();
    }

    void clear() noexcept
    {
        source = {};
        clearDocument();
        diagnostic.clear();
    }

    void reset(std::string_view newSource) noexcept
    {
        clear();
        source = newSource;
    }
};

} // namespace job::yaml