#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "job_yaml_block_scalar_decoder.h"
#include "job_yaml_concepts.h"
#include "job_yaml_indent_stack.h"
#include "job_yaml_lex.h"
#include "job_yaml_node_properties.h"
#include "job_yaml_parse_context.h"
#include "job_yaml_scalar_decoder.h"
#include "job_yaml_scalar_kernel.h"

namespace job::yaml {

class YamlParserUtils
{
public:
    YamlParserUtils() = delete;
    ~YamlParserUtils() = delete;

    YamlParserUtils(const YamlParserUtils &) = delete;
    YamlParserUtils &operator=(const YamlParserUtils &) = delete;
    YamlParserUtils(YamlParserUtils &&) = delete;
    YamlParserUtils &operator=(YamlParserUtils &&) = delete;

    enum class LineResult : std::uint8_t {
        Content,
        End,
        Error
    };

    struct Line
    {
        std::size_t indent{};
        std::size_t start{};
        YamlLexeme first{};
    };

    [[nodiscard]] static constexpr bool findBlockScalarEnd(std::string_view source,
                                                           std::size_t bodyStart,
                                                           std::size_t parentIndent,
                                                           const YamlBlockScalarHeader &header,
                                                           std::size_t &bodyEnd) noexcept
    {
        if (bodyStart > source.size())
            return false;

        std::size_t contentIndent = header.indentation == 0 ? 0 : parentIndent + header.indentation;
        std::size_t offset = bodyStart;
        bool hasContent = false;

        while (offset < source.size()) {
            const std::size_t lineStart = offset;
            std::size_t indentation = 0;

            while (offset < source.size() && source[offset] == ' ') {
                ++offset;
                ++indentation;
            }

            if (offset < source.size() && source[offset] == '\t') {
                bodyEnd = offset;
                return false;
            }

            const bool blank = offset >= source.size() || source[offset] == '\n' || source[offset] == '\r';

            if (!blank) {
                if (contentIndent == 0) {
                    if (indentation <= parentIndent) {
                        bodyEnd = lineStart;
                        return true;
                    }

                    contentIndent = indentation;
                } else if (indentation < contentIndent) {
                    // An explicit indentation indicator is a block-scalar requirement.
                    // If the first content line cannot satisfy it, this is malformed
                    // block-scalar syntax rather than a normal dedent ending the node.
                    if (!hasContent && header.indentation != 0)
                        return false;

                    bodyEnd = lineStart;
                    return true;
                }

                hasContent = true;
            }

            while (offset < source.size() && source[offset] != '\n' && source[offset] != '\r')
                ++offset;

            if (offset < source.size()) {
                if (source[offset] == '\r' && offset + 1 < source.size() && source[offset + 1] == '\n')
                    offset += 2;
                else
                    ++offset;
            }
        }

        bodyEnd = source.size();
        return true;
    }

    [[nodiscard]] static constexpr bool nextFlowToken(YamlLex &lexer, YamlLexeme &token) noexcept
    {
        while (true) {
            token = lexer.next();

            switch (token.type) {
            case YamlLexType::Indent:
            case YamlLexType::LineBreak:
            case YamlLexType::Comment:
                continue;

            case YamlLexType::End:
                return false;

            default:
                return true;
            }
        }
    }

    [[nodiscard]] static constexpr bool consumeNodeProperties(YamlLex &lexer,
                                                              YamlParseContext &context,
                                                              YamlNodeProperties &properties,
                                                              YamlLexeme &token,
                                                              bool registerAnchors)
    {
        while (token.type == YamlLexType::Anchor || token.type == YamlLexType::Tag) {
            if (token.text.size() <= 1)
                return false;

            if (token.type == YamlLexType::Anchor) {
                if (properties.hasAnchor())
                    return false;

                properties.anchor = token.text.substr(1);

                if (registerAnchors && !context.anchors.reserve(properties.anchor))
                    return false;
            } else {
                if (properties.hasTag())
                    return false;

                properties.tag = token.text;
            }

            token = lexer.next();
        }

        if (token.type == YamlLexType::Alias && !properties.empty())
            return false;

        return true;
    }

    [[nodiscard]] static constexpr bool completeAnchor(const YamlNodeProperties &properties,
                                                       std::size_t nodeStart,
                                                       std::size_t nodeIndent,
                                                       const YamlLex &lexer,
                                                       const Line &line,
                                                       bool hasLine,
                                                       YamlParseContext &context,
                                                       bool registerAnchors)
    {
        const std::size_t nodeEnd = hasLine ? line.start : lexer.source().size();
        return completeAnchorAt(properties, nodeStart, nodeEnd, nodeIndent, context, registerAnchors);
    }

    [[nodiscard]] static constexpr bool completeAnchorAt(const YamlNodeProperties &properties,
                                                         std::size_t nodeStart,
                                                         std::size_t nodeEnd,
                                                         std::size_t nodeIndent,
                                                         YamlParseContext &context,
                                                         bool registerAnchors)
    {
        if (!registerAnchors || !properties.hasAnchor())
            return true;

        if (nodeEnd < nodeStart)
            return false;

        return context.anchors.complete(properties.anchor,
                                        {
                                            .offset = nodeStart,
                                            .size = nodeEnd - nodeStart
                                        },
                                        nodeIndent);
    }

    template <YamlParseDestination Destination>
    [[nodiscard]] static constexpr bool emitScalar(Destination &destination, const YamlLexeme &scalar)
    {
        if (scalar.type != YamlLexType::Scalar)
            return false;

        YamlDecodedScalar decoded;

        if (!YamlScalarDecoder::decode(scalar.text, scalar.scalarStyle, decoded))
            return false;

        // NOTE copy land can/will be updated later.
        if (decoded.transformed())
            return destination.scalarOwned(std::string{decoded.value()});

        return destination.scalar(decoded.value());
    }

    template <YamlParseDestination Destination>
    [[nodiscard]] static constexpr bool emitKey(Destination &destination, const YamlLexeme &scalar)
    {
        if (scalar.type != YamlLexType::Scalar)
            return false;

        YamlDecodedScalar decoded;

        if (!YamlScalarDecoder::decode(scalar.text, scalar.scalarStyle, decoded))
            return false;

        // NOTE copy land can/will be updated later.
        if (decoded.transformed())
            return destination.keyOwned(std::string{decoded.value()});

        return destination.key(decoded.value());
    }

    [[nodiscard]] static constexpr bool enterIndent(YamlIndentStack &indents, std::size_t indent) noexcept
    {
        if (indents.empty())
            return indent == 0 && indents.push(indent);

        if (indents.relation(indent) != YamlIndentRelation::Deeper)
            return false;

        return indents.push(indent);
    }

    [[nodiscard]] static constexpr bool advanceAfterTail(YamlLex &lexer,
                                                         YamlLexeme tail,
                                                         Line &line,
                                                         bool &hasLine,
                                                         std::size_t baseIndent,
                                                         bool &firstLine)
    {
        switch (tail.type) {
        case YamlLexType::End:
            hasLine = false;
            return true;

        case YamlLexType::LineBreak:
            break;

        case YamlLexType::Comment:
            tail = lexer.next();

            if (tail.type == YamlLexType::End) {
                hasLine = false;
                return true;
            }

            if (tail.type != YamlLexType::LineBreak)
                return false;

            break;

        default:
            return false;
        }

        const LineResult result = nextLine(lexer, line, baseIndent, firstLine);

        if (result == LineResult::Error)
            return false;

        hasLine = result == LineResult::Content;
        return true;
    }

    [[nodiscard]] static constexpr LineResult nextLine(YamlLex &lexer,
                                                       Line &line,
                                                       std::size_t baseIndent,
                                                       bool &firstLine) noexcept
    {
        while (true) {
            YamlLexeme token = lexer.next();
            std::size_t indent = 0;
            std::size_t start = token.offset;

            if (token.type == YamlLexType::Indent) {
                indent = token.size();
                start = token.offset;
                token = lexer.next();
            }

            switch (token.type) {
            case YamlLexType::End:
                return LineResult::End;

            case YamlLexType::LineBreak:
                continue;

            case YamlLexType::Comment: {
                const YamlLexeme lineEnd = lexer.next();

                if (lineEnd.type == YamlLexType::End)
                    return LineResult::End;

                if (lineEnd.type != YamlLexType::LineBreak)
                    return LineResult::Error;

                continue;
            }

            case YamlLexType::Indent:
                return LineResult::Error;

            default:
                if (!firstLine) {
                    if (indent < baseIndent)
                        return LineResult::Error;

                    indent -= baseIndent;
                }

                firstLine = false;

                line = {
                    .indent = indent,
                    .start = start,
                    .first = token
                };
                return LineResult::Content;
            }
        }
    }

    [[nodiscard]] static constexpr bool isNullScalar(const YamlLexeme &scalar) noexcept
    {
        return scalar.type == YamlLexType::Scalar &&
               scalar.scalarStyle == YamlScalarStyle::Plain &&
               YamlScalarKernel::isNull(scalar.text);
    }

};

} // namespace job::yaml
