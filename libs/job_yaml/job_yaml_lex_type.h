#pragma once

#include <cstddef>
#include <string_view>

#include "job_yaml_scalar_style.h"
#include "job_yaml_source_range.h"

namespace job::yaml {

enum class YamlLexType : unsigned char {
    End,
    Scalar,
    Indent,
    SequenceEntry,
    MappingKey,
    MappingValue,
    FlowSequenceStart,
    FlowSequenceEnd,
    FlowMappingStart,
    FlowMappingEnd,
    CollectEntry,
    Anchor,
    Alias,
    Tag,
    Literal,
    Folded,
    DocumentStart,
    DocumentEnd,
    Directive,
    Comment,
    LineBreak
};

struct YamlLexeme
{
    YamlLexType type{YamlLexType::End};
    YamlScalarStyle scalarStyle{YamlScalarStyle::None};
    std::string_view text{};
    std::size_t offset{};

    [[nodiscard]] constexpr YamlSourceRange range() const noexcept
    {
        return {
            .offset = offset,
            .size = text.size()
        };
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
        return range().size;
    }

    [[nodiscard]] constexpr std::size_t endOffset() const noexcept
    {
        return range().endOffset();
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return range().empty();
    }
};

} // namespace job::yaml