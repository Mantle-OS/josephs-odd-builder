#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <regex>
#include <memory>
#include <cstdint>

#include "job_token_enums.h"

#include "jobtoken_export.h"

namespace job::token {


class JOBTOKEN_EXPORT RegexSplitter {
public:
    using Ptr  = std::shared_ptr<RegexSplitter>;
    using UPtr = std::unique_ptr<RegexSplitter>;
    using WPtr = std::weak_ptr<RegexSplitter>;

    RegexSplitter();
    explicit RegexSplitter(SplitPattern pattern);
    explicit RegexSplitter(std::string customRegexPattern);

    ~RegexSplitter() = default;

    RegexSplitter(const RegexSplitter &);
    RegexSplitter &operator=(const RegexSplitter &);
    RegexSplitter(RegexSplitter&&) noexcept = default;
    RegexSplitter &operator=(RegexSplitter&&) noexcept = default;

    [[nodiscard]] static UPtr createUniq(SplitPattern pattern = SplitPattern::LLaMA3)
    {
        return std::make_unique<RegexSplitter>(pattern);
    }

    [[nodiscard]] static UPtr createCustom(std::string customRegexPattern)
    {
        return std::make_unique<RegexSplitter>(std::move(customRegexPattern));
    }

    // Configures the splitter pattern
    void setPattern(SplitPattern pattern);
    void setCustomPattern(std::string customRegexPattern);
    void split(std::string_view text, std::vector<std::string_view> &outChunks) const;

    [[nodiscard]] std::vector<std::string_view> split(std::string_view text) const
    {
        std::vector<std::string_view> chunks;
        split(text, chunks);
        return chunks;
    }

    [[nodiscard]] SplitPattern patternType() const noexcept
    {
        return m_patternType;
    }
    [[nodiscard]] const std::string& patternString() const noexcept
    {
        return m_patternStr;
    }
    [[nodiscard]] bool isValid() const noexcept
    {
        return m_valid;
    }


    // I hate my life....
    //
    // Regex pre-tokenization patterns.
    // Some of these require Unicode-property/lookaround/possessive-regex
    // features that std::regex may not support. They are kept here anyway
    // as the canonical tokenizer pattern definitions.

    // GPT-2 / r50k_base / p50k_base / p50k_edit.
    //
    // OpenAI tiktoken currently uses this optimized equivalent of the
    // original GPT-2 pattern for all four encodings.
    static constexpr std::string_view kPatternGPT2 =
        R"('(?:[sdmt]|ll|ve|re)| ?\p{L}++| ?\p{N}++| ?[^\s\p{L}\p{N}]++|\s++$|\s+(?!\S)|\s)";

    static constexpr std::string_view kPatternR50K = kPatternGPT2;
    static constexpr std::string_view kPatternP50K = kPatternGPT2;
    static constexpr std::string_view kPatternP50KEdit = kPatternGPT2;

    // cl100k_base
    static constexpr std::string_view kPatternCL100K =
        R"('(?i:[sdmt]|ll|ve|re)|[^\r\n\p{L}\p{N}]?+\p{L}++|\p{N}{1,3}+| ?[^\s\p{L}\p{N}]++[\r\n]*+|\s++$|\s*[\r\n]|\s+(?!\S)|\s)";

    // GPT-4-era compatibility name.
    static constexpr std::string_view kPatternGPT4 = kPatternCL100K;

    // LLaMA 3
    static constexpr std::string_view kPatternLLaMA3 =
        R"((?:'[sS]|'[tT]|'[rR][eE]|'[vV][eE]|'[mM]|'[lL][lL]|'[dD])|[^\r\n\p{L}\p{N}]?\p{L}+|\p{N}{1,3}| ?[^\s\p{L}\p{N}]+[\r\n]*|\s*[\r\n]+|\s+(?!\S)|\s+)";

    // Qwen 2
    static constexpr std::string_view kPatternQwen2 =
        R"((?i:'s|'t|'re|'ve|'m|'ll|'d)|[^\r\n\p{L}\p{N}]?\p{L}+|\p{N}| ?[^\s\p{L}\p{N}]+[\r\n]*|\s*[\r\n]+|\s+(?!\S)|\s+)";

    // o200k_base
    static constexpr std::string_view kPatternO200K =
        R"([^\r\n\p{L}\p{N}]?[\p{Lu}\p{Lt}\p{Lm}\p{Lo}\p{M}]*[\p{Ll}\p{Lm}\p{Lo}\p{M}]+(?i:'s|'t|'re|'ve|'m|'ll|'d)?|[^\r\n\p{L}\p{N}]?[\p{Lu}\p{Lt}\p{Lm}\p{Lo}\p{M}]+[\p{Ll}\p{Lm}\p{Lo}\p{M}]*(?i:'s|'t|'re|'ve|'m|'ll|'d)?|\p{N}{1,3}| ?[^\s\p{L}\p{N}]+[\r\n/]*|\s*[\r\n]+|\s+(?!\S)|\s+)";

    // o200k_harmony uses the same pre-tokenization pattern as o200k_base.
    static constexpr std::string_view kPatternO200KHarmony = kPatternO200K;


private:
    void compileRegex();

    SplitPattern m_patternType{SplitPattern::None};
    std::string  m_patternStr;
    std::regex   m_regex;
    bool         m_valid{false};
};

} // namespace job::token