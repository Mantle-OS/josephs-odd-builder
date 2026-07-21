#include "job_zstd_options.h"
#include <utility>
#include <zstd.h>

namespace job::zstd {

const std::string &JobZstdOptions::magicDirString()
{
    static const std::string kMagic = "JOBZCRYPDIR";
    return kMagic;
}

const std::string &JobZstdOptions::magicEmptyDirString()
{
    static const std::string kMagic = "JOBZCRYPEMPTYDIR";
    return kMagic;
}

const std::string &JobZstdOptions::magicFileString()
{
    static const std::string kMagic = "JOBZCRYPFILE";
    return kMagic;
}

const std::string &JobZstdOptions::magicLinkString()
{
    static const std::string kMagic = "JOBZCRYPLINK";
    return kMagic;
}

int JobZstdOptions::minCompressionLevel() noexcept
{
    return ZSTD_minCLevel();
}

int JobZstdOptions::maxCompressionLevel() noexcept
{
    return ZSTD_maxCLevel();
}

const std::string &JobZstdOptions::input() const noexcept
{
    return m_input;
}

bool JobZstdOptions::setInput(const std::string &newInput)
{
    if (m_input == newInput)
        return false;

    m_input = newInput;
    return true;
}

const std::string &JobZstdOptions::output() const noexcept
{
    return m_output;
}

bool JobZstdOptions::setOutput(const std::string &newOutput)
{
    if (m_output == newOutput)
        return false;

    m_output = newOutput;
    return true;
}

int JobZstdOptions::current() const noexcept
{
    return m_current;
}

bool JobZstdOptions::setCurrent(int newCurrent)
{
    if (m_current == newCurrent)
        return false;

    m_current = newCurrent;
    return true;
}

int JobZstdOptions::total() const noexcept
{
    return m_total;
}

bool JobZstdOptions::setTotal(int newTotal)
{
    if (m_total == newTotal)
        return false;

    m_total = newTotal;
    return true;
}

int JobZstdOptions::compressionLevel() const noexcept
{
    return m_compressionLevel;
}

bool JobZstdOptions::setCompressionLevel(int newCompressionLevel)
{
    int const minLevel = minCompressionLevel();
    int const maxLevel = maxCompressionLevel();

    int const resolved = (newCompressionLevel >= minLevel && newCompressionLevel <= maxLevel) ?
                             newCompressionLevel :
                             kDefaultCompressionLevel;

    if (m_compressionLevel == resolved)
        return false;

    m_compressionLevel = resolved;
    return true;
}

const std::string &JobZstdOptions::errorString() const noexcept
{
    return m_errorString;
}

bool JobZstdOptions::setErrorString(const std::string &newErrorString)
{
    if (m_errorString == newErrorString)
        return false;

    m_errorString = newErrorString;
    return true;
}

bool JobZstdOptions::recursiveDirectories() const noexcept
{
    return m_recursiveDirectories;
}

bool JobZstdOptions::setRecursiveDirectories(bool value)
{
    if (m_recursiveDirectories == value)
        return false;

    m_recursiveDirectories = value;
    return true;
}

bool JobZstdOptions::preserveEmptyDirectories() const noexcept
{
    return m_preserveEmptyDirectories;
}

bool JobZstdOptions::setPreserveEmptyDirectories(bool value)
{
    if (m_preserveEmptyDirectories == value)
        return false;
    m_preserveEmptyDirectories = value;
    return true;
}

bool JobZstdOptions::preserveSymlinks() const noexcept
{
    return m_preserveSymlinks;
}

bool JobZstdOptions::setPreserveSymlinks(bool value)
{
    if (m_preserveSymlinks == value)
        return false;
    m_preserveSymlinks = value;
    return true;
}

void JobZstdOptions::setOnFinished(FinishedCallback callback)
{
    m_onFinished = std::move(callback);
}

void JobZstdOptions::notifyFinished() const
{
    if (m_onFinished)
        m_onFinished();
}

} // namespace job::zstd