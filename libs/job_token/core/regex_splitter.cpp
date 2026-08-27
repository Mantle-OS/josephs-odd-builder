#include "core/regex_splitter.h"
#include <utility>

namespace job::token {

RegexSplitter::RegexSplitter() = default;

RegexSplitter::RegexSplitter(SplitPattern pattern)
{
    setPattern(pattern);
}

RegexSplitter::RegexSplitter(std::string customRegexPattern)
{
    setCustomPattern(std::move(customRegexPattern));
}

void RegexSplitter::setPattern(SplitPattern pattern)
{
    m_patternType = pattern;

    switch (pattern) {
    case SplitPattern::GPT2:        m_patternStr = kPatternGPT2;        break;
    case SplitPattern::R50K:        m_patternStr = kPatternR50K;        break;
    case SplitPattern::P50K:        m_patternStr = kPatternP50K;        break;
    case SplitPattern::P50KEdit:    m_patternStr = kPatternP50KEdit;    break;
    case SplitPattern::CL100K:      m_patternStr = kPatternCL100K;      break;
    case SplitPattern::O200K:       m_patternStr = kPatternO200K;       break;
    case SplitPattern::O200KHarmony:m_patternStr = kPatternO200KHarmony;break;
    case SplitPattern::GPT4:        m_patternStr = kPatternGPT4;        break;
    case SplitPattern::LLaMA3:      m_patternStr = kPatternLLaMA3;      break;
    case SplitPattern::Qwen2:       m_patternStr = kPatternQwen2;       break;

    case SplitPattern::None:
    case SplitPattern::Custom:
    default:
        m_patternStr.clear();
        break;
    }

    compileRegex();
}

void RegexSplitter::setCustomPattern(std::string customRegexPattern)
{
    m_patternType = SplitPattern::Custom;
    m_patternStr = std::move(customRegexPattern);
    compileRegex();
}

void RegexSplitter::compileRegex()
{
    m_valid = false;

    if (m_patternType == SplitPattern::None || m_patternStr.empty())
        return;

    try {
        m_regex = std::regex{
            m_patternStr,
            std::regex::ECMAScript | std::regex::optimize
        };

        m_valid = true;
    } catch (const std::regex_error &) {
        m_valid = false;
    }
}

void RegexSplitter::split(std::string_view text, std::vector<std::string_view> &outChunks) const
{
    if (text.empty())
        return;

    // Unsupported/invalid patterns degrade to one unmodified chunk.
    if (!m_valid || m_patternType == SplitPattern::None || m_patternStr.empty()) {
        outChunks.push_back(text);
        return;
    }

    const char *current = text.data();
    const char *const end = text.data() + text.size();
    std::cmatch match;
    while (current < end && std::regex_search(current, end, match, m_regex)) {
        // Guard against zero-length regex matches.
        if (match.length() == 0) {
            outChunks.emplace_back(current, 1);
            ++current;
            continue;
        }

        // Preserve text not consumed by the regex.
        if (match[0].first > current) {
            outChunks.emplace_back(
                current,
                static_cast<std::size_t>(match[0].first - current));
        }

        outChunks.emplace_back(
            match[0].first,
            static_cast<std::size_t>(match.length()));

        current = match[0].second;
    }

    if (current < end) {
        outChunks.emplace_back(
            current,
            static_cast<std::size_t>(end - current));
    }
}

} // namespace job::token