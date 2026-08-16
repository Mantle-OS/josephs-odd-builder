#include "core/regex_splitter.h"

#include <stdexcept>
#include <utility>

namespace job::token {

RegexSplitter::RegexSplitter() :
    m_patternType{SplitPattern::None},
      m_valid{true}
{
}

RegexSplitter::RegexSplitter(SplitPattern pattern) :
    m_patternType{pattern}
{
    setPattern(pattern);
}

RegexSplitter::RegexSplitter(std::string customRegexPattern) :
    m_patternType{SplitPattern::Custom},
    m_patternStr{std::move(customRegexPattern)}
{
    compileRegex();
}

void RegexSplitter::setPattern(SplitPattern pattern)
{
    m_patternType = pattern;

    switch (pattern) {
    case SplitPattern::GPT2:
        m_patternStr = std::string(kPatternGPT2);
        break;
    case SplitPattern::GPT4:
        m_patternStr = std::string(kPatternGPT4);
        break;
    case SplitPattern::LLaMA3:
        m_patternStr = std::string(kPatternLLaMA3);
        break;
    case SplitPattern::Qwen2:
        m_patternStr = std::string(kPatternQwen2);
        break;
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
    m_patternStr  = std::move(customRegexPattern);
    compileRegex();
}

void RegexSplitter::compileRegex()
{
    if (m_patternStr.empty() || m_patternType == SplitPattern::None) {
        m_valid = true;
        return;
    }

    try {
        m_regex = std::regex(m_patternStr, std::regex::ECMAScript | std::regex::optimize);
        m_valid = true;
    } catch (...) {
        m_valid = false;
    }
}

void RegexSplitter::split(std::string_view text, std::vector<std::string_view>& outChunks) const
{
    if (text.empty())
        return;

    // Fast path: if no pattern configured or compilation failed, treat full text as one chunk
    if (!m_valid || m_patternType == SplitPattern::None || m_patternStr.empty()) {
        outChunks.push_back(text);
        return;
    }

    const char* start = text.data();
    const char* const end = text.data() + text.size();
    std::cmatch match;

    while (start < end && std::regex_search(start, end, match, m_regex)) {
        // Zero-length match guard to prevent infinite loops
        if (match.length() == 0) {
            outChunks.emplace_back(start, 1);
            start += 1;
            continue;
        }

        // If there is leading non-matching text between matches, preserve it
        if (match[0].first > start)
            outChunks.emplace_back(start, static_cast<size_t>(match[0].first - start));

        outChunks.emplace_back(match[0].first, static_cast<size_t>(match.length()));
        start = match[0].second;
    }

    // Trailing non-matched chunk
    if (start < end)
        outChunks.emplace_back(start, static_cast<size_t>(end - start));
}

} // namespace job::token