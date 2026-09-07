#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "job_yaml_decoded_scalar.h"

namespace job::yaml {

enum class YamlBlockScalarStyle : unsigned char {
    Literal,
    Folded
};

enum class YamlBlockScalarChomping : unsigned char {
    Clip,
    Strip,
    Keep
};

struct YamlBlockScalarHeader
{
    YamlBlockScalarStyle style{YamlBlockScalarStyle::Literal};
    YamlBlockScalarChomping chomping{YamlBlockScalarChomping::Clip};
    std::size_t indentation{};
};

class YamlBlockScalarDecoder
{
public:
    YamlBlockScalarDecoder() = delete;
    ~YamlBlockScalarDecoder() = delete;

    YamlBlockScalarDecoder(const YamlBlockScalarDecoder &) = delete;
    YamlBlockScalarDecoder &operator=(const YamlBlockScalarDecoder &) = delete;

    YamlBlockScalarDecoder(YamlBlockScalarDecoder &&) = delete;
    YamlBlockScalarDecoder &operator=(YamlBlockScalarDecoder &&) = delete;

    [[nodiscard]] static bool parseHeader(std::string_view source,
                                          YamlBlockScalarHeader &header) noexcept
    {
        if (source.empty())
            return false;

        switch (source.front()) {
        case '|':
            header.style = YamlBlockScalarStyle::Literal;
            break;

        case '>':
            header.style = YamlBlockScalarStyle::Folded;
            break;

        default:
            return false;
        }

        header.chomping = YamlBlockScalarChomping::Clip;
        header.indentation = 0;

        bool hasChomping = false;
        bool hasIndentation = false;

        for (std::size_t i = 1; i < source.size(); ++i) {
            const char c = source[i];

            if (c == '+' || c == '-') {
                if (hasChomping)
                    return false;

                header.chomping = c == '+'
                                      ? YamlBlockScalarChomping::Keep
                                      : YamlBlockScalarChomping::Strip;
                hasChomping = true;
                continue;
            }

            if (c >= '1' && c <= '9') {
                if (hasIndentation)
                    return false;

                header.indentation = static_cast<std::size_t>(c - '0');
                hasIndentation = true;
                continue;
            }

            return false;
        }

        return true;
    }

    [[nodiscard]] static bool decode(std::string_view headerSource,
                                     std::string_view body,
                                     std::size_t parentIndentation,
                                     YamlDecodedScalar &result)
    {
        YamlBlockScalarHeader header;

        if (!parseHeader(headerSource, header))
            return false;

        return decode(header, body, parentIndentation, result);
    }

    [[nodiscard]] static bool decode(const YamlBlockScalarHeader &header,
                                     std::string_view body,
                                     std::size_t parentIndentation,
                                     YamlDecodedScalar &result)
    {
        result.clear();

        std::vector<Line> lines;

        if (!splitLines(body, lines))
            return false;

        std::size_t contentIndentation{};

        if (!resolveContentIndentation(header,
                                       lines,
                                       parentIndentation,
                                       contentIndentation))
            return false;

        if (!normalizeLines(lines, contentIndentation))
            return false;

        std::string storage;
        storage.reserve(body.size());

        switch (header.style) {
        case YamlBlockScalarStyle::Literal:
            appendLiteral(lines, storage);
            break;

        case YamlBlockScalarStyle::Folded:
            appendFolded(lines, storage);
            break;
        }

        applyChomping(header.chomping,
                      hasTrailingBreak(lines),
                      storage);

        result.setTransformed(std::move(storage));
        return true;
    }

private:
    struct Line
    {
        std::string_view source{};
        std::string_view content{};
        std::size_t indentation{};
        bool blank{};
        bool moreIndented{};
        bool hadBreak{};
    };

    [[nodiscard]] static bool splitLines(std::string_view body,
                                         std::vector<Line> &lines)
    {
        lines.clear();

        std::size_t offset = 0;

        while (offset < body.size()) {
            const std::size_t start = offset;

            while (offset < body.size() &&
                   body[offset] != '\n' &&
                   body[offset] != '\r')
                ++offset;

            const std::size_t end = offset;
            bool hadBreak = false;

            if (offset < body.size()) {
                hadBreak = true;

                if (body[offset] == '\r' &&
                    offset + 1 < body.size() &&
                    body[offset + 1] == '\n')
                    offset += 2;
                else
                    ++offset;
            }

            lines.push_back({
                .source = body.substr(start, end - start),
                .content = {},
                .indentation = 0,
                .blank = false,
                .moreIndented = false,
                .hadBreak = hadBreak
            });
        }

        return true;
    }

    [[nodiscard]] static bool resolveContentIndentation(const YamlBlockScalarHeader &header,
                                                        const std::vector<Line> &lines,
                                                        std::size_t parentIndentation,
                                                        std::size_t &contentIndentation) noexcept
    {
        if (header.indentation != 0) {
            contentIndentation = parentIndentation + header.indentation;
            return true;
        }

        for (const Line &line : lines) {
            std::size_t indentation = 0;

            while (indentation < line.source.size() &&
                   line.source[indentation] == ' ')
                ++indentation;

            if (indentation == line.source.size())
                continue;

            if (line.source[indentation] == '\t')
                return false;

            if (indentation <= parentIndentation)
                return false;

            contentIndentation = indentation;
            return true;
        }

        contentIndentation = parentIndentation + 1;
        return true;
    }

    [[nodiscard]] static bool normalizeLines(std::vector<Line> &lines,
                                             std::size_t contentIndentation) noexcept
    {
        for (Line &line : lines) {
            std::size_t indentation = 0;

            while (indentation < line.source.size() &&
                   line.source[indentation] == ' ')
                ++indentation;

            if (indentation < line.source.size() &&
                line.source[indentation] == '\t')
                return false;

            const bool blank = indentation == line.source.size();

            if (blank) {
                line.content = {};
                line.indentation = indentation;
                line.blank = true;
                line.moreIndented = false;
                continue;
            }

            if (indentation < contentIndentation)
                return false;

            line.content = line.source.substr(contentIndentation);
            line.indentation = indentation;
            line.blank = false;
            line.moreIndented = indentation > contentIndentation;
        }

        return true;
    }

    static void appendLiteral(const std::vector<Line> &lines,
                              std::string &storage)
    {
        for (const Line &line : lines) {
            storage.append(line.content);

            if (line.hadBreak)
                storage.push_back('\n');
        }
    }

    static void appendFolded(const std::vector<Line> &lines,
                             std::string &storage)
    {
        for (std::size_t i = 0; i < lines.size(); ++i) {
            const Line &line = lines[i];

            storage.append(line.content);

            if (!line.hadBreak)
                continue;

            if (i + 1 >= lines.size()) {
                storage.push_back('\n');
                continue;
            }

            const Line &next = lines[i + 1];

            if (line.blank) {
                storage.push_back('\n');
                continue;
            }

            if (next.blank) {
                if (!hasNonBlankAfter(lines, i + 1) ||
                    line.moreIndented ||
                    nextNonBlankIsMoreIndented(lines, i + 1))
                    storage.push_back('\n');

                continue;
            }

            if (line.moreIndented || next.moreIndented) {
                storage.push_back('\n');
                continue;
            }

            storage.push_back(' ');
        }
    }

    [[nodiscard]] static bool hasNonBlankAfter(const std::vector<Line> &lines,
                                               std::size_t offset) noexcept
    {
        for (std::size_t i = offset; i < lines.size(); ++i) {
            if (!lines[i].blank)
                return true;
        }

        return false;
    }

    [[nodiscard]] static bool nextNonBlankIsMoreIndented(const std::vector<Line> &lines,
                                                         std::size_t offset) noexcept
    {
        for (std::size_t i = offset; i < lines.size(); ++i) {
            if (!lines[i].blank)
                return lines[i].moreIndented;
        }

        return false;
    }

    [[nodiscard]] static bool hasTrailingBreak(const std::vector<Line> &lines) noexcept
    {
        return !lines.empty() && lines.back().hadBreak;
    }

    static void applyChomping(YamlBlockScalarChomping chomping,
                              bool hadTrailingBreak,
                              std::string &storage) noexcept
    {
        switch (chomping) {
        case YamlBlockScalarChomping::Strip:
            trimTrailingBreaks(storage);
            return;

        case YamlBlockScalarChomping::Clip:
            trimTrailingBreaks(storage);

            if (hadTrailingBreak)
                storage.push_back('\n');

            return;

        case YamlBlockScalarChomping::Keep:
            return;
        }
    }

    static void trimTrailingBreaks(std::string &storage) noexcept
    {
        while (!storage.empty() && storage.back() == '\n')
            storage.pop_back();
    }
};

} // namespace job::yaml