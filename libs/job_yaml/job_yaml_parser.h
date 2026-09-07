#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "job_yaml_block_scalar_decoder.h"
#include "job_yaml_concepts.h"
#include "job_yaml_diagnostic.h"
#include "job_yaml_indent_stack.h"
#include "job_yaml_lex.h"
#include "job_yaml_node_properties.h"
#include "job_yaml_parse_context.h"
#include "job_yaml_parser_utils.h"
#include "job_yaml_scalar_decoder.h"

namespace job::yaml {
class YamlParser
{
public:
    YamlParser() = delete;
    ~YamlParser() = delete;

    YamlParser(const YamlParser &) = delete;
    YamlParser &operator=(const YamlParser &) = delete;
    YamlParser(YamlParser &&) = delete;
    YamlParser &operator=(YamlParser &&) = delete;

    template <YamlParseDestination Destination>
    [[nodiscard]] static constexpr bool parse(std::string_view source, Destination &destination)
    {
        YamlDiagnostic diagnostic;
        return parse(source, destination, diagnostic);
    }

    template <YamlParseDestination Destination>
    [[nodiscard]] static constexpr bool parse(std::string_view source, Destination &destination, YamlDiagnostic &diagnostic)
    {
        YamlParseContext context;
        context.reset(source);

        const bool result = parseDocument(source, destination, context);

        if (!result && !context.diagnostic.valid())
            (void) fail(context, YamlDiagnosticKind::InvalidDocument, {.offset = source.size(), .size = 0});

        diagnostic = context.diagnostic;
        return result;
    }

private:
    using Utils = YamlParserUtils;
    using Line = Utils::Line;
    using LineResult = Utils::LineResult;


    [[nodiscard]] static constexpr YamlSourceRange directiveDiagnosticRange(const YamlParseContext &context,
                                                                            const YamlLexeme &directive) noexcept
    {
        YamlSourceRange range = directive.range();
        std::size_t end = directive.endOffset();

        while (end < context.source.size()) {
            const char c = context.source[end];

            if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
                break;

            ++end;
        }

        range.size = end - range.offset;
        return range;
    }

    [[nodiscard]] static constexpr YamlSourceRange invalidScalarDiagnosticRange(const YamlLexeme &scalar) noexcept
    {
        YamlSourceRange range = scalar.range();

        if (scalar.scalarStyle != YamlScalarStyle::SingleQuoted &&
            scalar.scalarStyle != YamlScalarStyle::DoubleQuoted)
            return range;

        while (range.size > 0) {
            const char c = scalar.text[range.size - 1];

            if (c != '\n' && c != '\r')
                break;

            --range.size;
        }

        return range;
    }

    [[nodiscard]] static constexpr YamlSourceRange missingFlowMappingSeparatorRange(const YamlLexeme &key) noexcept
    {
        for (std::size_t index = 0; index < key.text.size(); ++index) {
            if (key.text[index] != ' ' && key.text[index] != '\t')
                continue;

            while (index < key.text.size() && (key.text[index] == ' ' || key.text[index] == '\t'))
                ++index;

            if (index < key.text.size())
                return {.offset = key.offset + index, .size = key.text.size() - index};

            break;
        }

        return key.range();
    }

    [[nodiscard]] static constexpr bool fail(YamlParseContext &context,
                                             YamlDiagnosticKind kind,
                                             YamlSourceRange range) noexcept
    {
        if (!context.diagnostic.valid())
            context.diagnostic.set(kind, range, context.source);

        return false;
    }

    [[nodiscard]] static constexpr bool fail(YamlParseContext &context,
                                             YamlDiagnosticKind kind,
                                             const YamlLexeme &lexeme) noexcept
    {
        if (lexeme.type == YamlLexType::Directive)
            return fail(context, kind, directiveDiagnosticRange(context, lexeme));

        return fail(context, kind, lexeme.range());
    }

    [[nodiscard]] static constexpr YamlDiagnosticKind documentTokenKind(YamlLexType type) noexcept
    {
        return type == YamlLexType::Directive ? YamlDiagnosticKind::InvalidDirective : YamlDiagnosticKind::InvalidDocument;
    }

    [[nodiscard]] static constexpr LineResult nextLine(YamlLex &lexer,
                                                       Line &line,
                                                       std::size_t baseIndent,
                                                       bool &firstLine) noexcept
    {
        const LineResult result = Utils::nextLine(lexer, line, baseIndent, firstLine);

        if (result == LineResult::Content && line.first.type == YamlLexType::DocumentEnd)
            return LineResult::End;

        return result;
    }

    [[nodiscard]] static constexpr bool consumeDirectiveLine(YamlLex &lexer,
                                                             YamlParseContext &context,
                                                             const YamlLexeme &directive)
    {
        YamlLexeme token = lexer.next();

        if (token.type == YamlLexType::End || token.type == YamlLexType::LineBreak || token.type == YamlLexType::Comment)
            return fail(context, YamlDiagnosticKind::InvalidDirective, directive);

        while (true) {
            token = lexer.next();

            switch (token.type) {
            case YamlLexType::End:
            case YamlLexType::LineBreak:
                return true;

            case YamlLexType::Comment: {
                const YamlLexeme lineEnd = lexer.next();
                if (lineEnd.type == YamlLexType::End || lineEnd.type == YamlLexType::LineBreak)
                    return true;

                return fail(context, YamlDiagnosticKind::InvalidDirective, lineEnd);
            }

            default:
                break;
            }
        }
    }

    [[nodiscard]] static constexpr bool lineAfterDocumentStart(YamlLex &lexer,
                                                               Line &line,
                                                               bool &firstLine,
                                                               YamlParseContext &context,
                                                               const YamlLexeme &documentStart)
    {
        YamlLexeme token = lexer.next();

        if (token.type == YamlLexType::Comment) {
            const YamlLexeme lineEnd = lexer.next();
            if (lineEnd.type == YamlLexType::End)
                return fail(context, YamlDiagnosticKind::InvalidDocument, documentStart);

            if (lineEnd.type != YamlLexType::LineBreak)
                return fail(context, YamlDiagnosticKind::InvalidDocument, lineEnd);

            const LineResult result = Utils::nextLine(lexer, line, 0, firstLine);
            if (result != LineResult::Content)
                return fail(context, YamlDiagnosticKind::InvalidDocument, {.offset = lexer.cursor().offset(), .size = 0});

            return true;
        }

        if (token.type == YamlLexType::LineBreak) {
            const LineResult result = Utils::nextLine(lexer, line, 0, firstLine);
            if (result != LineResult::Content)
                return fail(context, YamlDiagnosticKind::InvalidDocument, {.offset = lexer.cursor().offset(), .size = 0});

            return true;
        }

        if (token.type == YamlLexType::End || token.type == YamlLexType::DocumentEnd)
            return fail(context, YamlDiagnosticKind::InvalidDocument, token.type == YamlLexType::End ? YamlSourceRange{.offset = token.offset, .size = 0} : token.range());

        if (token.type == YamlLexType::DocumentStart || token.type == YamlLexType::Directive)
            return fail(context, documentTokenKind(token.type), token);

        firstLine = false;
        line = {
            .indent = 0,
            .start = token.offset,
            .first = token
        };
        return true;
    }

    [[nodiscard]] static constexpr bool validateDocumentTail(YamlLex &lexer,
                                                             YamlParseContext &context)
    {
        while (true) {
            const YamlLexeme token = lexer.next();

            switch (token.type) {
            case YamlLexType::End:
                return true;

            case YamlLexType::Indent:
            case YamlLexType::LineBreak:
            case YamlLexType::Comment:
                continue;

            case YamlLexType::Directive:
            default:
                return fail(context, YamlDiagnosticKind::InvalidDocument, token);
            }
        }
    }

    template <YamlParseDestination Destination>
    [[nodiscard]] static constexpr bool parseDocument(std::string_view source,
                                                      Destination &destination,
                                                      YamlParseContext &context)
    {
        YamlLex lexer{source};
        YamlIndentStack indents;
        Line line;
        bool firstLine = true;

        LineResult result = Utils::nextLine(lexer, line, 0, firstLine);
        if (result == LineResult::Error)
            return fail(context, YamlDiagnosticKind::InvalidIndentation, {.offset = lexer.cursor().offset(), .size = 0});

        if (result == LineResult::End)
            return fail(context, YamlDiagnosticKind::InvalidDocument, {.offset = source.size(), .size = 0});

        bool hasDirectives = false;

        while (line.first.type == YamlLexType::Directive) {
            hasDirectives = true;
            const YamlLexeme directive = line.first;

            if (line.indent != 0 || !consumeDirectiveLine(lexer, context, directive))
                return false;

            result = Utils::nextLine(lexer, line, 0, firstLine);
            if (result != LineResult::Content)
                return fail(context, YamlDiagnosticKind::InvalidDirective, {.offset = lexer.cursor().offset(), .size = 0});
        }

        if (line.first.type == YamlLexType::DocumentEnd)
            return fail(context, YamlDiagnosticKind::InvalidDocument, line.first);

        if (line.first.type == YamlLexType::DocumentStart) {
            const YamlLexeme documentStart = line.first;
            if (line.indent != 0)
                return fail(context, YamlDiagnosticKind::InvalidDocument, documentStart);

            if (!lineAfterDocumentStart(lexer, line, firstLine, context, documentStart))
                return false;
        } else if (hasDirectives) {
            return fail(context, YamlDiagnosticKind::InvalidDirective, line.first);
        }

        if (line.first.type == YamlLexType::DocumentStart || line.first.type == YamlLexType::DocumentEnd || line.first.type == YamlLexType::Directive)
            return fail(context, documentTokenKind(line.first.type), line.first);

        if (line.indent != 0)
            return fail(context, YamlDiagnosticKind::InvalidIndentation, line.first);

        context.clearDocument();
        bool hasLine = true;

        if (!parseRootNode(lexer, destination, indents, line, hasLine, context, 0, firstLine, true))
            return false;

        if (line.first.type == YamlLexType::DocumentEnd)
            return validateDocumentTail(lexer, context);

        return true;
    }


    [[nodiscard]] static constexpr bool advanceAfterTail(YamlLex &lexer,
                                                         YamlLexeme tail,
                                                         Line &line,
                                                         bool &hasLine,
                                                         YamlParseContext &context,
                                                         std::size_t baseIndent,
                                                         bool &firstLine)
    {
        switch (tail.type) {
        case YamlLexType::End:
            hasLine = false;
            return true;

        case YamlLexType::DocumentEnd:
            line = {.indent = 0, .start = tail.offset, .first = tail};
            hasLine = false;
            return true;

        case YamlLexType::DocumentStart:
            return fail(context, YamlDiagnosticKind::InvalidDocument, tail);

        case YamlLexType::Directive:
            return fail(context, YamlDiagnosticKind::InvalidDirective, tail);

        case YamlLexType::LineBreak:
            break;

        case YamlLexType::Comment:
            tail = lexer.next();

            if (tail.type == YamlLexType::End) {
                hasLine = false;
                return true;
            }

            if (tail.type != YamlLexType::LineBreak)
                return fail(context, YamlDiagnosticKind::UnexpectedToken, tail);

            break;

        default:
            return fail(context, YamlDiagnosticKind::UnexpectedToken, tail);
        }

        const LineResult result = nextLine(lexer, line, baseIndent, firstLine);

        if (result == LineResult::Error)
            return fail(context, YamlDiagnosticKind::InvalidIndentation, {.offset = lexer.cursor().offset(), .size = 0});

        hasLine = result == LineResult::Content;
        return true;
    }

    [[nodiscard]] static constexpr bool consumeNodeProperties(YamlLex &lexer,
                                                              YamlParseContext &context,
                                                              YamlNodeProperties &properties,
                                                              YamlLexeme &token,
                                                              bool registerAnchors)
    {
        while (token.type == YamlLexType::Anchor || token.type == YamlLexType::Tag) {
            if (token.text.size() <= 1)
                return fail(context,
                            token.type == YamlLexType::Anchor ? YamlDiagnosticKind::InvalidAnchor : YamlDiagnosticKind::InvalidTag,
                            token);

            if (token.type == YamlLexType::Anchor) {
                if (properties.hasAnchor())
                    return fail(context, YamlDiagnosticKind::InvalidAnchor, token);

                properties.anchor = token.text.substr(1);

                if (registerAnchors && !context.anchors.reserve(properties.anchor))
                    return fail(context, YamlDiagnosticKind::InvalidAnchor, token);
            } else {
                if (properties.hasTag())
                    return fail(context, YamlDiagnosticKind::InvalidTag, token);

                properties.tag = token.text;
            }

            token = lexer.next();
        }

        if (token.type == YamlLexType::Alias && !properties.empty())
            return fail(context, YamlDiagnosticKind::InvalidAlias, token);

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
        const std::size_t nodeEnd = hasLine || line.first.type == YamlLexType::DocumentEnd ? line.start : lexer.source().size();
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
            return fail(context, YamlDiagnosticKind::InvalidAnchor, {.offset = nodeStart, .size = 0});

        if (!context.anchors.complete(properties.anchor,
                                      {.offset = nodeStart, .size = nodeEnd - nodeStart},
                                      nodeIndent))
            return fail(context, YamlDiagnosticKind::InvalidAnchor, {.offset = nodeStart, .size = nodeEnd - nodeStart});

        return true;
    }

    template <YamlParseDestination Destination>
    [[nodiscard]] static constexpr bool emitScalar(Destination &destination,
                                                   const YamlLexeme &scalar,
                                                   YamlParseContext &context)
    {
        if (scalar.type != YamlLexType::Scalar)
            return fail(context, YamlDiagnosticKind::InvalidScalar, scalar);

        if (Utils::isNullScalar(scalar)) {
            if (!destination.null())
                return fail(context, YamlDiagnosticKind::DestinationRejected, scalar);

            return true;
        }

        YamlDecodedScalar decoded;
        if (!YamlScalarDecoder::decode(scalar.text, scalar.scalarStyle, decoded))
            return fail(context, YamlDiagnosticKind::InvalidScalar, invalidScalarDiagnosticRange(scalar));

        const bool accepted = decoded.transformed()
                                  ? destination.scalarOwned(std::string{decoded.value()})
                                  : destination.scalar(decoded.value());

        if (!accepted)
            return fail(context, YamlDiagnosticKind::DestinationRejected, scalar);

        return true;
    }

    template <YamlParseDestination Destination>
    [[nodiscard]] static constexpr bool emitKey(Destination &destination,
                                                const YamlLexeme &scalar,
                                                YamlParseContext &context)
    {
        if (scalar.type != YamlLexType::Scalar)
            return fail(context, YamlDiagnosticKind::InvalidScalar, scalar);

        YamlDecodedScalar decoded;
        if (!YamlScalarDecoder::decode(scalar.text, scalar.scalarStyle, decoded))
            return fail(context, YamlDiagnosticKind::InvalidScalar, invalidScalarDiagnosticRange(scalar));

        const bool accepted = decoded.transformed()
                                  ? destination.keyOwned(std::string{decoded.value()})
                                  : destination.key(decoded.value());

        if (!accepted)
            return fail(context, YamlDiagnosticKind::DestinationRejected, scalar);

        return true;
    }

    [[nodiscard]] static constexpr bool nextFlowToken(YamlLex &lexer,
                                                      YamlLexeme &token,
                                                      YamlParseContext &context) noexcept
    {
        while (true) {
            token = lexer.next();

            switch (token.type) {
            case YamlLexType::Indent:
            case YamlLexType::LineBreak:
            case YamlLexType::Comment:
                continue;

            case YamlLexType::End:
            case YamlLexType::DocumentEnd:
                return fail(context, YamlDiagnosticKind::UnexpectedEnd, token);

            default:
                return true;
            }
        }
    }

    template <YamlParseDestination Destination>
    [[nodiscard]] static constexpr bool parseSource(std::string_view source,
                                                    Destination &destination,
                                                    YamlParseContext &context,
                                                    std::size_t baseIndent,
                                                    bool registerAnchors)
    {
        YamlLex lexer{source};
        YamlIndentStack indents;
        Line line;
        bool firstLine = true;

        const LineResult result = nextLine(lexer, line, baseIndent, firstLine);

        if (result != LineResult::Content)
            return false;

        if (line.indent != 0)
            return false;

        bool hasLine = true;

        return parseRootNode(lexer, destination, indents, line, hasLine, context, baseIndent, firstLine, registerAnchors);
    }

    template <YamlParseDestination Destination>
    [[nodiscard]] static constexpr bool parseRootNode(YamlLex &lexer,
                                                      Destination &destination,
                                                      YamlIndentStack &indents,
                                                      Line &line,
                                                      bool &hasLine,
                                                      YamlParseContext &context,
                                                      std::size_t baseIndent,
                                                      bool &firstLine,
                                                      bool registerAnchors)
    {
        YamlNodeProperties properties;
        YamlLexeme node = line.first;

        if (!consumeNodeProperties(lexer, context, properties, node, registerAnchors))
            return false;

        if (node.type == YamlLexType::LineBreak) {
            const LineResult result = nextLine(lexer, line, baseIndent, firstLine);

            if (result != LineResult::Content) {
                hasLine = false;
                return false;
            }

            hasLine = true;
            node = line.first;

            if (node.type == YamlLexType::Anchor || node.type == YamlLexType::Tag) {
                if (!consumeNodeProperties(lexer, context, properties, node, registerAnchors))
                    return false;
            }
        }

        line.first = node;

        const std::size_t nodeStart = node.offset;
        const std::size_t nodeIndent = line.indent;
        std::size_t explicitNodeEnd = 0;
        bool hasExplicitNodeEnd = false;
        bool success = false;

        if (node.type == YamlLexType::SequenceEntry) {
            success = parseSequence(lexer, destination, indents, line, hasLine, context, baseIndent, firstLine, registerAnchors);
        } else if (node.type == YamlLexType::FlowSequenceStart || node.type == YamlLexType::FlowMappingStart) {
            success = parseFlowNodeFromToken(lexer, destination, context, node, registerAnchors, nodeIndent, explicitNodeEnd);
            if (success) {
                hasExplicitNodeEnd = true;
                const YamlLexeme tail = lexer.next();
                success = advanceAfterTail(lexer, tail, line, hasLine, context, baseIndent, firstLine) && !hasLine;
            }
        } else if (node.type == YamlLexType::Alias) {
            success = parseAliasNode(lexer, destination, line, hasLine, context, baseIndent, firstLine, node);
        } else if (node.type == YamlLexType::Literal || node.type == YamlLexType::Folded) {
            success = parseBlockScalar(lexer, destination, line, hasLine, context, baseIndent, firstLine, node, nodeIndent, explicitNodeEnd);
            if (success) {
                hasExplicitNodeEnd = true;
                success = !hasLine;
            }
        } else if (node.type == YamlLexType::Scalar) {
            const YamlLexeme next = lexer.next();

            if (next.type == YamlLexType::MappingValue) {
                success = parseMapping(lexer, destination, indents, line, hasLine, context, baseIndent, firstLine, registerAnchors, true);
            } else {
                success = parseRootScalar(lexer, destination, line, hasLine, context, baseIndent, firstLine, node, next);
            }
        } else {
            return false;
        }

        if (!success)
            return false;

        if (hasExplicitNodeEnd)
            return completeAnchorAt(properties, nodeStart, explicitNodeEnd, nodeIndent, context, registerAnchors);

        return completeAnchor(properties, nodeStart, nodeIndent, lexer, line, hasLine, context, registerAnchors);
    }

    template <YamlParseDestination Destination>
    [[nodiscard]] static constexpr bool parseRootScalar(YamlLex &lexer,
                                                        Destination &destination,
                                                        Line &line,
                                                        bool &hasLine,
                                                        YamlParseContext &context,
                                                        std::size_t baseIndent,
                                                        bool &firstLine,
                                                        const YamlLexeme &scalar,
                                                        YamlLexeme tail)
    {
        if (!emitScalar(destination, scalar, context))
            return false;

        if (!advanceAfterTail(lexer, tail, line, hasLine, context, baseIndent, firstLine))
            return false;

        if (hasLine)
            return fail(context, documentTokenKind(line.first.type), line.first);

        return true;
    }

    template <YamlParseDestination Destination>
    [[nodiscard]] static constexpr bool parseBlock(YamlLex &lexer,
                                                   Destination &destination,
                                                   YamlIndentStack &indents,
                                                   Line &line,
                                                   bool &hasLine,
                                                   YamlParseContext &context,
                                                   std::size_t baseIndent,
                                                   bool &firstLine,
                                                   bool registerAnchors)
    {
        if (!hasLine)
            return false;

        YamlNodeProperties properties;
        YamlLexeme node = line.first;

        if (!consumeNodeProperties(lexer, context, properties, node, registerAnchors))
            return false;

        if (node.type == YamlLexType::LineBreak) {
            const std::size_t propertyIndent = line.indent;
            const LineResult result = nextLine(lexer, line, baseIndent, firstLine);

            if (result != LineResult::Content) {
                hasLine = false;
                return false;
            }

            if (line.indent <= propertyIndent)
                return false;

            hasLine = true;
            node = line.first;

            if (node.type == YamlLexType::Anchor || node.type == YamlLexType::Tag) {
                if (!consumeNodeProperties(lexer, context, properties, node, registerAnchors))
                    return false;
            }
        }

        line.first = node;

        const std::size_t nodeStart = node.offset;
        const std::size_t nodeIndent = line.indent;

        if (node.type == YamlLexType::FlowSequenceStart || node.type == YamlLexType::FlowMappingStart) {
            std::size_t nodeEnd = 0;

            if (!parseFlowNodeFromToken(lexer, destination, context, node, registerAnchors, nodeIndent, nodeEnd))
                return false;

            const YamlLexeme tail = lexer.next();

            if (!advanceAfterTail(lexer, tail, line, hasLine, context, baseIndent, firstLine))
                return false;

            return completeAnchorAt(properties, nodeStart, nodeEnd, nodeIndent, context, registerAnchors);
        }

        if (node.type == YamlLexType::Literal || node.type == YamlLexType::Folded) {
            std::size_t nodeEnd = 0;

            if (!parseBlockScalar(lexer, destination, line, hasLine, context, baseIndent, firstLine, node, nodeIndent, nodeEnd))
                return false;

            return completeAnchorAt(properties, nodeStart, nodeEnd, nodeIndent, context, registerAnchors);
        }

        bool success = false;

        if (node.type == YamlLexType::SequenceEntry) {
            success = parseSequence(lexer, destination, indents, line, hasLine, context, baseIndent, firstLine, registerAnchors);
        } else if (node.type == YamlLexType::Alias) {
            success = parseAliasNode(lexer, destination, line, hasLine, context, baseIndent, firstLine, node);
        } else if (node.type == YamlLexType::Scalar) {
            const YamlLexeme separator = lexer.next();

            if (separator.type != YamlLexType::MappingValue)
                return false;

            success = parseMapping(lexer, destination, indents, line, hasLine, context, baseIndent, firstLine, registerAnchors, true);
        } else {
            return false;
        }

        if (!success)
            return false;

        return completeAnchor(properties, nodeStart, nodeIndent, lexer, line, hasLine, context, registerAnchors);
    }

    template <YamlParseDestination Destination>
    [[nodiscard]] static constexpr bool parseMapping(YamlLex &lexer,
                                                     Destination &destination,
                                                     YamlIndentStack &indents,
                                                     Line &line,
                                                     bool &hasLine,
                                                     YamlParseContext &context,
                                                     std::size_t baseIndent,
                                                     bool &firstLine,
                                                     bool registerAnchors,
                                                     bool separatorConsumed)
    {
        const std::size_t indent = line.indent;

        if (!Utils::enterIndent(indents, indent))
            return fail(context, YamlDiagnosticKind::InvalidIndentation, line.first);

        if (!destination.beginMapping())
            return fail(context, YamlDiagnosticKind::DestinationRejected, line.first);

        while (hasLine) {
            if (line.indent < indent)
                break;

            if (line.indent > indent)
                return fail(context, YamlDiagnosticKind::InvalidIndentation, line.first);

            if (line.first.type != YamlLexType::Scalar) {
                if (line.first.type == YamlLexType::DocumentStart || line.first.type == YamlLexType::DocumentEnd)
                    return fail(context, YamlDiagnosticKind::InvalidDocument, line.first);
                if (line.first.type == YamlLexType::Directive)
                    return fail(context, YamlDiagnosticKind::InvalidDirective, line.first);
                return fail(context, YamlDiagnosticKind::UnexpectedToken, line.first);
            }

            if (!separatorConsumed) {
                const YamlLexeme separator = lexer.next();

                if (separator.type != YamlLexType::MappingValue)
                    return fail(context, YamlDiagnosticKind::UnexpectedToken, separator);
            }

            separatorConsumed = false;

            if (!emitKey(destination, line.first, context))
                return false;

            if (!parseMappingValue(lexer, destination, indents, line, hasLine, context, baseIndent, firstLine, registerAnchors, indent))
                return false;
        }

        if (!destination.endMapping())
            return fail(context, YamlDiagnosticKind::DestinationRejected, line.first);

        return indents.pop();
    }

    template <YamlParseDestination Destination>
    [[nodiscard]] static constexpr bool parseMappingValue(YamlLex &lexer,
                                                          Destination &destination,
                                                          YamlIndentStack &indents,
                                                          Line &line,
                                                          bool &hasLine,
                                                          YamlParseContext &context,
                                                          std::size_t baseIndent,
                                                          bool &firstLine,
                                                          bool registerAnchors,
                                                          std::size_t parentIndent)
    {
        YamlLexeme value = lexer.next();
        YamlNodeProperties properties;

        if (!consumeNodeProperties(lexer, context, properties, value, registerAnchors))
            return false;

        if (value.type == YamlLexType::Alias) {
            const std::size_t nodeStart = value.offset;

            if (!parseAliasNode(lexer, destination, line, hasLine, context, baseIndent, firstLine, value))
                return false;

            return completeAnchor(properties, nodeStart, parentIndent, lexer, line, hasLine, context, registerAnchors);
        }

        if (value.type == YamlLexType::FlowSequenceStart || value.type == YamlLexType::FlowMappingStart) {
            const std::size_t nodeStart = value.offset;
            std::size_t nodeEnd = 0;

            if (!parseFlowNodeFromToken(lexer, destination, context, value, registerAnchors, parentIndent, nodeEnd))
                return false;

            const YamlLexeme tail = lexer.next();

            if (!advanceAfterTail(lexer, tail, line, hasLine, context, baseIndent, firstLine))
                return false;

            return completeAnchorAt(properties, nodeStart, nodeEnd, parentIndent, context, registerAnchors);
        }

        if (value.type == YamlLexType::Literal || value.type == YamlLexType::Folded) {
            const std::size_t nodeStart = value.offset;
            std::size_t nodeEnd = 0;

            if (!parseBlockScalar(lexer, destination, line, hasLine, context, baseIndent, firstLine, value, parentIndent, nodeEnd))
                return false;

            return completeAnchorAt(properties, nodeStart, nodeEnd, parentIndent, context, registerAnchors);
        }

        if (value.type == YamlLexType::Scalar) {
            const std::size_t nodeStart = value.offset;

            if (!emitScalar(destination, value, context))
                return false;

            const YamlLexeme tail = lexer.next();
            if (!advanceAfterTail(lexer, tail, line, hasLine, context, baseIndent, firstLine))
                return false;

            return completeAnchor(properties, nodeStart, parentIndent, lexer, line, hasLine, context, registerAnchors);
        }

        if (value.type == YamlLexType::Comment) {
            const YamlLexeme lineEnd = lexer.next();

            if (lineEnd.type == YamlLexType::End) {
                hasLine = false;
                return false;
            }

            if (lineEnd.type != YamlLexType::LineBreak)
                return false;

            const LineResult result = nextLine(lexer, line, baseIndent, firstLine);

            if (result != LineResult::Content) {
                hasLine = false;
                return false;
            }

            hasLine = true;
        } else if (value.type == YamlLexType::LineBreak) {
            const LineResult result = nextLine(lexer, line, baseIndent, firstLine);
            if (result != LineResult::Content) {
                hasLine = false;
                return false;
            }

            hasLine = true;
        } else {
            return false;
        }

        if (line.indent <= parentIndent)
            return fail(context, YamlDiagnosticKind::InvalidIndentation, line.first);

        const std::size_t nodeStart = line.first.offset;
        const std::size_t nodeIndent = line.indent;

        if (!parseBlock(lexer, destination, indents, line, hasLine, context, baseIndent, firstLine, registerAnchors))
            return false;

        return completeAnchor(properties, nodeStart, nodeIndent, lexer, line, hasLine, context, registerAnchors);
    }

    template <YamlParseDestination Destination>
    [[nodiscard]] static constexpr bool parseSequence(YamlLex &lexer,
                                                      Destination &destination,
                                                      YamlIndentStack &indents,
                                                      Line &line,
                                                      bool &hasLine,
                                                      YamlParseContext &context,
                                                      std::size_t baseIndent,
                                                      bool &firstLine,
                                                      bool registerAnchors)
    {
        const std::size_t indent = line.indent;

        if (!Utils::enterIndent(indents, indent))
            return fail(context, YamlDiagnosticKind::InvalidIndentation, line.first);

        if (!destination.beginSequence())
            return fail(context, YamlDiagnosticKind::DestinationRejected, line.first);

        while (hasLine) {
            if (line.indent < indent)
                break;

            if (line.indent > indent)
                return fail(context, YamlDiagnosticKind::InvalidIndentation, line.first);

            if (line.first.type != YamlLexType::SequenceEntry) {
                if (line.first.type == YamlLexType::DocumentStart || line.first.type == YamlLexType::DocumentEnd)
                    return fail(context, YamlDiagnosticKind::InvalidDocument, line.first);
                if (line.first.type == YamlLexType::Directive)
                    return fail(context, YamlDiagnosticKind::InvalidDirective, line.first);
                return fail(context, YamlDiagnosticKind::UnexpectedToken, line.first);
            }

            if (!parseSequenceEntry(lexer, destination, indents, line, hasLine, context, baseIndent, firstLine, registerAnchors, indent))
                return false;
        }

        if (!destination.endSequence())
            return fail(context, YamlDiagnosticKind::DestinationRejected, line.first);

        return indents.pop();
    }

    template <YamlParseDestination Destination>
    [[nodiscard]] static constexpr bool parseSequenceEntry(YamlLex &lexer,
                                                           Destination &destination,
                                                           YamlIndentStack &indents,
                                                           Line &line,
                                                           bool &hasLine,
                                                           YamlParseContext &context,
                                                           std::size_t baseIndent,
                                                           bool &firstLine,
                                                           bool registerAnchors,
                                                           std::size_t sequenceIndent)
    {
        YamlLexeme value = lexer.next();
        YamlNodeProperties properties;

        if (!consumeNodeProperties(lexer, context, properties, value, registerAnchors))
            return false;

        if (value.type == YamlLexType::Alias) {
            const std::size_t nodeStart = value.offset;

            if (!parseAliasNode(lexer, destination, line, hasLine, context, baseIndent, firstLine, value))
                return false;

            return completeAnchor(properties, nodeStart, sequenceIndent, lexer, line, hasLine, context, registerAnchors);
        }

        if (value.type == YamlLexType::FlowSequenceStart || value.type == YamlLexType::FlowMappingStart) {
            const std::size_t nodeStart = value.offset;
            std::size_t nodeEnd = 0;

            if (!parseFlowNodeFromToken(lexer, destination, context, value, registerAnchors, sequenceIndent, nodeEnd))
                return false;

            const YamlLexeme tail = lexer.next();

            if (!advanceAfterTail(lexer, tail, line, hasLine, context, baseIndent, firstLine))
                return false;

            return completeAnchorAt(properties, nodeStart, nodeEnd, sequenceIndent, context, registerAnchors);
        }

        if (value.type == YamlLexType::Literal || value.type == YamlLexType::Folded) {
            const std::size_t nodeStart = value.offset;
            std::size_t nodeEnd = 0;

            if (!parseBlockScalar(lexer, destination, line, hasLine, context, baseIndent, firstLine, value, sequenceIndent, nodeEnd))
                return false;

            return completeAnchorAt(properties, nodeStart, nodeEnd, sequenceIndent, context, registerAnchors);
        }

        if (value.type == YamlLexType::Scalar) {
            const std::size_t nodeStart = value.offset;
            const YamlLexeme next = lexer.next();
            if (next.type == YamlLexType::MappingValue) {
                Line mappingLine{
                    .indent = value.offset - line.start,
                    .start = line.start,
                    .first = value
                };

                if (mappingLine.indent <= sequenceIndent)
                    return fail(context, YamlDiagnosticKind::InvalidIndentation, value);

                if (!parseMapping(lexer, destination, indents, mappingLine, hasLine, context, baseIndent, firstLine, registerAnchors, true))
                    return false;

                if (hasLine)
                    line = mappingLine;

                return completeAnchor(properties, nodeStart, value.offset - line.start, lexer, line, hasLine, context, registerAnchors);
            }

            if (!emitScalar(destination, value, context))
                return false;

            if (!advanceAfterTail(lexer, next, line, hasLine, context, baseIndent, firstLine))
                return false;

            return completeAnchor(properties, nodeStart, sequenceIndent, lexer, line, hasLine, context, registerAnchors);
        }

        if (value.type == YamlLexType::Comment) {
            const YamlLexeme lineEnd = lexer.next();

            if (lineEnd.type == YamlLexType::End) {
                hasLine = false;
                return false;
            }

            if (lineEnd.type != YamlLexType::LineBreak)
                return false;

            const LineResult result = nextLine(lexer, line, baseIndent, firstLine);
            if (result != LineResult::Content) {
                hasLine = false;
                return false;
            }

            hasLine = true;
        } else if (value.type == YamlLexType::LineBreak) {
            const LineResult result = nextLine(lexer, line, baseIndent, firstLine);

            if (result != LineResult::Content) {
                hasLine = false;
                return false;
            }

            hasLine = true;
        } else {
            return false;
        }

        if (line.indent <= sequenceIndent)
            return fail(context, YamlDiagnosticKind::InvalidIndentation, line.first);

        const std::size_t nodeStart = line.first.offset;
        const std::size_t nodeIndent = line.indent;

        if (!parseBlock(lexer, destination, indents, line, hasLine, context, baseIndent, firstLine, registerAnchors))
            return false;

        return completeAnchor(properties, nodeStart, nodeIndent, lexer, line, hasLine, context, registerAnchors);
    }

    template <YamlParseDestination Destination>
    [[nodiscard]] static constexpr bool parseBlockScalar(YamlLex &lexer,
                                                         Destination &destination,
                                                         Line &line, bool &hasLine, YamlParseContext &context, std::size_t baseIndent, bool &firstLine,
                                                         const YamlLexeme &header,
                                                         std::size_t parentIndent,
                                                         std::size_t &nodeEnd)
    {
        if (header.type != YamlLexType::Literal && header.type != YamlLexType::Folded)
            return fail(context, YamlDiagnosticKind::InvalidBlockScalar, header);

        YamlBlockScalarHeader blockHeader;

        if (!YamlBlockScalarDecoder::parseHeader(header.text, blockHeader))
            return fail(context, YamlDiagnosticKind::InvalidBlockScalar, header);

        YamlLexeme tail = lexer.next();

        if (tail.type == YamlLexType::Comment) {
            if (tail.offset == header.endOffset())
                return fail(context, YamlDiagnosticKind::InvalidBlockScalar, tail);

            tail = lexer.next();
        }

        if (tail.type != YamlLexType::LineBreak)
            return fail(context, YamlDiagnosticKind::InvalidBlockScalar,
                        {.offset = header.offset, .size = tail.endOffset() - header.offset});

        const std::size_t bodyStart = lexer.cursor().offset();
        std::size_t bodyEnd = std::string_view::npos;

        if (!Utils::findBlockScalarEnd(lexer.source(), bodyStart, baseIndent + parentIndent, blockHeader, bodyEnd)) {
            if (bodyEnd != std::string_view::npos)
                return fail(context,
                            YamlDiagnosticKind::InvalidBlockScalar,
                            {.offset = bodyEnd, .size = bodyEnd < lexer.source().size() ? 1u : 0u});

            return fail(context, YamlDiagnosticKind::InvalidBlockScalar, header);
        }
        const std::string_view body = lexer.source().substr(bodyStart, bodyEnd - bodyStart);

        YamlDecodedScalar decoded;
        if (!YamlBlockScalarDecoder::decode(blockHeader, body, baseIndent + parentIndent, decoded))
            return fail(context, YamlDiagnosticKind::InvalidBlockScalar, header);

        if (decoded.transformed()) {
            if (!destination.scalarOwned(std::string{decoded.value()}))
                return fail(context, YamlDiagnosticKind::DestinationRejected, header);
        } else if (!destination.scalar(decoded.value())) {
            return fail(context, YamlDiagnosticKind::DestinationRejected, header);
        }

        nodeEnd = bodyEnd;

        // Block scalar parsing source-scans the complete body; do not use advanceAfterTail().
        lexer.resetAtLineStart(bodyEnd);

        const LineResult result = nextLine(lexer, line, baseIndent, firstLine);

        if (result == LineResult::Error)
            return fail(context, YamlDiagnosticKind::InvalidIndentation, {.offset = lexer.cursor().offset(), .size = 0});

        hasLine = result == LineResult::Content;
        return true;
    }

    template <YamlParseDestination Destination>
    [[nodiscard]] static constexpr bool parseFlowNodeFromToken(YamlLex &lexer,
                                                               Destination &destination,
                                                               YamlParseContext &context,
                                                               YamlLexeme node,
                                                               bool registerAnchors,
                                                               std::size_t nodeIndent,
                                                               std::size_t &nodeEnd)
    {
        YamlNodeProperties properties;
        if (!consumeNodeProperties(lexer, context, properties, node, registerAnchors))
            return false;

        const std::size_t nodeStart = node.offset;
        if (node.type == YamlLexType::Scalar) {
            if (!emitScalar(destination, node, context))
                return false;

            nodeEnd = node.endOffset();
        } else if (node.type == YamlLexType::Alias) {
            if (!replayAlias(destination, context, node))
                return false;

            nodeEnd = node.endOffset();
        } else if (node.type == YamlLexType::FlowSequenceStart) {
            if (!parseFlowSequence(lexer, destination, context, registerAnchors, nodeIndent, nodeEnd))
                return false;
        } else if (node.type == YamlLexType::FlowMappingStart) {
            if (!parseFlowMapping(lexer, destination, context, registerAnchors, nodeIndent, nodeEnd))
                return false;
        } else {
            return fail(context, YamlDiagnosticKind::InvalidFlowCollection, node);
        }

        return completeAnchorAt(properties, nodeStart, nodeEnd, nodeIndent, context, registerAnchors);
    }

    template <YamlParseDestination Destination>
    [[nodiscard]] static constexpr bool parseFlowSequence(YamlLex &lexer,
                                                          Destination &destination,
                                                          YamlParseContext &context,
                                                          bool registerAnchors,
                                                          std::size_t nodeIndent,
                                                          std::size_t &nodeEnd)
    {
        if (!destination.beginSequence())
            return fail(context, YamlDiagnosticKind::DestinationRejected, {.offset = lexer.cursor().offset(), .size = 0});

        YamlLexeme token;

        if (!nextFlowToken(lexer, token, context))
            return false;

        if (token.type == YamlLexType::FlowSequenceEnd) {
            nodeEnd = token.endOffset();
            if (!destination.endSequence())
                return fail(context, YamlDiagnosticKind::DestinationRejected, token);
            return true;
        }

        while (true) {
            std::size_t childEnd = 0;

            if (!parseFlowNodeFromToken(lexer, destination, context, token, registerAnchors, nodeIndent, childEnd))
                return false;

            if (!nextFlowToken(lexer, token, context))
                return false;

            if (token.type == YamlLexType::FlowSequenceEnd) {
                nodeEnd = token.endOffset();
                if (!destination.endSequence())
                    return fail(context, YamlDiagnosticKind::DestinationRejected, token);
                return true;
            }

            if (token.type != YamlLexType::CollectEntry)
                return fail(context, YamlDiagnosticKind::InvalidFlowCollection, token);

            if (!nextFlowToken(lexer, token, context))
                return false;

            if (token.type == YamlLexType::FlowSequenceEnd) {
                nodeEnd = token.endOffset();
                if (!destination.endSequence())
                    return fail(context, YamlDiagnosticKind::DestinationRejected, token);
                return true;
            }
        }
    }

    template <YamlParseDestination Destination>
    [[nodiscard]] static constexpr bool parseFlowMapping(YamlLex &lexer,
                                                         Destination &destination,
                                                         YamlParseContext &context,
                                                         bool registerAnchors,
                                                         std::size_t nodeIndent,
                                                         std::size_t &nodeEnd)
    {
        if (!destination.beginMapping())
            return fail(context, YamlDiagnosticKind::DestinationRejected, {.offset = lexer.cursor().offset(), .size = 0});

        YamlLexeme token;

        if (!nextFlowToken(lexer, token, context))
            return false;

        if (token.type == YamlLexType::FlowMappingEnd) {
            nodeEnd = token.endOffset();
            if (!destination.endMapping())
                return fail(context, YamlDiagnosticKind::DestinationRejected, token);
            return true;
        }

        while (true) {
            YamlNodeProperties keyProperties;

            if (!consumeNodeProperties(lexer, context, keyProperties, token, registerAnchors))
                return false;

            if (!keyProperties.empty() || token.type != YamlLexType::Scalar)
                return fail(context, YamlDiagnosticKind::InvalidFlowCollection, token);

            const YamlLexeme key = token;

            if (!nextFlowToken(lexer, token, context))
                return false;

            if (token.type != YamlLexType::MappingValue) {
                const YamlSourceRange missingSeparator = missingFlowMappingSeparatorRange(key);

                if (missingSeparator.offset != key.offset)
                    return fail(context, YamlDiagnosticKind::InvalidFlowCollection, missingSeparator);

                return fail(context, YamlDiagnosticKind::InvalidFlowCollection, token);
            }

            if (!emitKey(destination, key, context))
                return false;

            if (!nextFlowToken(lexer, token, context))
                return false;

            std::size_t childEnd = 0;

            if (!parseFlowNodeFromToken(lexer, destination, context, token, registerAnchors, nodeIndent, childEnd))
                return false;

            if (!nextFlowToken(lexer, token, context))
                return false;

            if (token.type == YamlLexType::FlowMappingEnd) {
                nodeEnd = token.endOffset();
                if (!destination.endMapping())
                    return fail(context, YamlDiagnosticKind::DestinationRejected, token);
                return true;
            }

            if (token.type != YamlLexType::CollectEntry)
                return fail(context, YamlDiagnosticKind::InvalidFlowCollection, token);

            if (!nextFlowToken(lexer, token, context))
                return false;

            if (token.type == YamlLexType::FlowMappingEnd) {
                nodeEnd = token.endOffset();
                if (!destination.endMapping())
                    return fail(context, YamlDiagnosticKind::DestinationRejected, token);
                return true;
            }
        }
    }

    template <YamlParseDestination Destination>
    [[nodiscard]] static constexpr bool parseAliasNode(YamlLex &lexer,
                                                       Destination &destination,
                                                       Line &line,
                                                       bool &hasLine,
                                                       YamlParseContext &context,
                                                       std::size_t baseIndent,
                                                       bool &firstLine,
                                                       const YamlLexeme &alias)
    {
        if (!replayAlias(destination, context, alias))
            return false;

        const YamlLexeme tail = lexer.next();
        return advanceAfterTail(lexer, tail, line, hasLine, context, baseIndent, firstLine);
    }

    template <YamlParseDestination Destination>
    [[nodiscard]] static constexpr bool replayAlias(Destination &destination, YamlParseContext &context, const YamlLexeme &alias)
    {
        if (alias.text.size() <= 1)
            return fail(context, YamlDiagnosticKind::InvalidAlias, alias);

        const std::string_view name = alias.text.substr(1);
        const YamlAnchorEntry *entry = context.anchors.find(name);

        if (entry == nullptr || !entry->isComplete())
            return fail(context, YamlDiagnosticKind::InvalidAlias, alias);

        const std::string_view anchoredSource = entry->range.view(context.source);
        if (anchoredSource.empty() && !entry->range.empty())
            return fail(context, YamlDiagnosticKind::InvalidAlias, alias);

        const std::size_t aliasDepth = context.aliases.size();
        if (!context.aliases.push(name))
            return fail(context, YamlDiagnosticKind::InvalidAlias, alias);

        const YamlDiagnostic outerDiagnostic = context.diagnostic;
        context.diagnostic.clear();
        const bool replayed = parseSource(anchoredSource, destination, context, entry->indent, false);
        context.diagnostic = outerDiagnostic;
        context.aliases.pop();

        contract_assert(context.aliases.size() == aliasDepth);

        if (!replayed)
            return fail(context, YamlDiagnosticKind::InvalidAlias, alias);

        return true;
    }

};
} // namespace job::yaml