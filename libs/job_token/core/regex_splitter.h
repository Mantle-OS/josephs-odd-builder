#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <regex>
#include <memory>
#include <span>

#include "job_tokenizer_types.h"
#include "jobtoken_export.h"

namespace job::token {

enum class SplitPattern : uint8_t {
    None = 0,
    GPT2,
    GPT4,
    LLaMA3,
    Qwen2,
    Custom
};

class JOBTOKEN_EXPORT RegexSplitter {
public:
    using Ptr  = std::shared_ptr<RegexSplitter>;
    using UPtr = std::unique_ptr<RegexSplitter>;

    RegexSplitter();
    explicit RegexSplitter(SplitPattern pattern);
    explicit RegexSplitter(std::string customRegexPattern);

    ~RegexSplitter() = default;

    RegexSplitter(const RegexSplitter&) = default;
    RegexSplitter& operator=(const RegexSplitter&) = default;
    RegexSplitter(RegexSplitter&&) noexcept = default;
    RegexSplitter& operator=(RegexSplitter&&) noexcept = default;

    [[nodiscard]] static UPtr create(SplitPattern pattern = SplitPattern::LLaMA3)
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
    void split(std::string_view text, std::vector<std::string_view>& outChunks) const;

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
    static constexpr std::string_view kPatternGPT2 =
        R"('s|'t|'re|'ve|'m|'ll|'d| ?[[:alpha:]]+| ?[[:digit:]]+| ?[^\s[:alnum:]]+|\s+(?!\S)|\s+)";

    static constexpr std::string_view kPatternGPT4 =
        R"((?i:'s|'t|'re|'ve|'m|'ll|'d)|[^\r\n\p{L}\p{N}]?\p{L}+|\p{N}{1,3}| ?[^\s\p{L}\p{N}]+[\r\n]*|\s*[\r\n]+|\s+(?!\S)|\s+)";

    static constexpr std::string_view kPatternLLaMA3 =
        R"((?:'[sS]|'[tT]|'[rR][eE]|'[vV][eE]|'[mM]|'[lL][lL]|'[dD])|[^\r\n\p{L}\p{N}]?\p{L}+|\p{N}{1,3}| ?[^\s\p{L}\p{N}]+[\r\n]*|\s*[\r\n]+|\s+(?!\S)|\s+)";

    static constexpr std::string_view kPatternQwen2 =
        R"((?i:'s|'t|'re|'ve|'m|'ll|'d)|[^\r\n\p{L}\p{N}]?\p{L}+|\p{N}| ?[^\s\p{L}\p{N}]+[\r\n]*|\s*[\r\n]+|\s+(?!\S)|\s+)";

private:
    void compileRegex();

    SplitPattern m_patternType{SplitPattern::None};
    std::string  m_patternStr;
    std::regex   m_regex;
    bool         m_valid{true};
};

} // namespace job::token