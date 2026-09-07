#include "pty_environ.h"

#include <utility>

#include <job_hash.h>

extern char **environ;
namespace job::io {


PtyEnviron::PtyEnviron() = default;

bool PtyEnviron::buildInitList()
{
    m_initial.clear();

    if (!environ) {
        m_environ.clear();
        return true;
    }

    for (char **entry = environ; *entry != nullptr; ++entry) {
        const std::string_view current{*entry};
        const std::size_t separator = current.find('=');

        if (separator == std::string_view::npos)
            continue;

        const std::string_view name = current.substr(0, separator);
        const std::string_view value = current.substr(separator + 1);

        if (name.empty())
            continue;

        const std::size_t index = initialIndexOf(name);

        if (index != npos) {
            m_initial[index].setValue(std::string{value});
            continue;
        }

        m_initial.emplace_back(
            std::string{name},
            std::string{value},
            true,
            PtyEnv::EnvType::Custom
            );
    }

    m_environ = m_initial;
    return true;
}

void PtyEnviron::setResolved(EnvList environ)
{
    m_environ = std::move(environ);
}

void PtyEnviron::resetResolved()
{
    m_environ = m_initial;
}

const PtyEnviron::EnvList &PtyEnviron::initial() const noexcept
{
    return m_initial;
}

bool PtyEnviron::containsInitial(std::string_view name) const noexcept
{
    return initialIndexOf(name) != npos;
}

std::size_t PtyEnviron::initialIndexOf(std::string_view name) const noexcept
{
    for (std::size_t i = 0; i < m_initial.size(); ++i) {
        if (m_initial[i].name() == name)
            return i;
    }

    return npos;
}

const PtyEnv *PtyEnviron::initialEnv(std::string_view name) const noexcept
{
    const std::size_t index = initialIndexOf(name);

    if (index == npos)
        return nullptr;

    return &m_initial[index];
}

PtyEnv *PtyEnviron::initialEnv(std::string_view name) noexcept
{
    const std::size_t index = initialIndexOf(name);

    if (index == npos)
        return nullptr;

    return &m_initial[index];
}

const PtyEnviron::EnvList &PtyEnviron::resolved() const noexcept
{
    return m_environ;
}

PtyEnviron::EnvList &PtyEnviron::resolved() noexcept
{
    return m_environ;
}

bool PtyEnviron::contains(std::string_view name) const noexcept
{
    return indexOf(name) != npos;
}

std::size_t PtyEnviron::indexOf(std::string_view name) const noexcept
{
    for (std::size_t i = 0; i < m_environ.size(); ++i) {
        if (m_environ[i].name() == name)
            return i;
    }

    return npos;
}

const PtyEnv *PtyEnviron::env(std::string_view name) const noexcept
{
    const std::size_t index = indexOf(name);

    if (index == npos)
        return nullptr;

    return &m_environ[index];
}

PtyEnv *PtyEnviron::env(std::string_view name) noexcept
{
    const std::size_t index = indexOf(name);

    if (index == npos)
        return nullptr;

    return &m_environ[index];
}

std::string_view PtyEnviron::value(std::string_view name, std::string_view defaultValue) const noexcept
{
    const PtyEnv *entry = env(name);

    if (!entry || !entry->enabled())
        return defaultValue;

    return entry->value();
}

bool PtyEnviron::add(PtyEnv env)
{
    if (env.name().empty())
        return false;

    if (contains(env.name()))
        return false;

    m_environ.push_back(std::move(env));
    return true;
}

bool PtyEnviron::set(std::string_view name,
                     std::string_view value,
                     bool enabled,
                     PtyEnv::EnvType envType)
{
    if (name.empty())
        return false;

    PtyEnv *entry = env(name);

    if (entry) {
        entry->setValue(std::string{value});
        entry->setEnabled(enabled);
        entry->setEnvType(envType);
        return true;
    }

    m_environ.emplace_back(
        std::string{name},
        std::string{value},
        enabled,
        envType
        );

    return true;
}

bool PtyEnviron::setDefault(std::string_view name,
                            std::string_view value,
                            bool enabled,
                            PtyEnv::EnvType envType)
{
    if (contains(name))
        return true;

    return set(name, value, enabled, envType);
}

bool PtyEnviron::append(std::string_view name,
                        std::string_view value,
                        std::string_view separator)
{
    PtyEnv *entry = env(name);

    if (!entry)
        return set(name, value);

    std::string updated{entry->value()};

    if (!updated.empty() && !value.empty())
        updated.append(separator);

    updated.append(value);
    entry->setValue(std::move(updated));

    return true;
}

bool PtyEnviron::prepend(std::string_view name,
                         std::string_view value,
                         std::string_view separator)
{
    PtyEnv *entry = env(name);

    if (!entry)
        return set(name, value);

    if (value.empty())
        return true;

    std::string updated;
    updated.reserve(value.size() + separator.size() + entry->value().size());

    updated.append(value);

    if (!entry->value().empty())
        updated.append(separator);

    updated.append(entry->value());

    entry->setValue(std::move(updated));
    return true;
}

bool PtyEnviron::remove(std::string_view name)
{
    const std::size_t index = indexOf(name);

    if (index == npos)
        return false;

    m_environ.erase(m_environ.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

void PtyEnviron::clearResolved() noexcept
{
    m_environ.clear();
}

const PtyEnviron::DotList &PtyEnviron::dotFiles() const noexcept
{
    return m_dotFiles;
}

PtyEnviron::DotList &PtyEnviron::dotFiles() noexcept
{
    return m_dotFiles;
}

bool PtyEnviron::addDotFile(PtyDot dot)
{
    if (dot.path().empty())
        return false;

    if (containsDotFile(dot.path()))
        return false;

    m_dotFiles.push_back(std::move(dot));
    return true;
}

bool PtyEnviron::removeDotFile(std::string_view path)
{
    const std::size_t index = dotFileIndexOf(path);

    if (index == npos)
        return false;

    m_dotFiles.erase(m_dotFiles.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

bool PtyEnviron::containsDotFile(std::string_view path) const noexcept
{
    return dotFileIndexOf(path) != npos;
}

std::size_t PtyEnviron::dotFileIndexOf(std::string_view path) const noexcept
{
    for (std::size_t i = 0; i < m_dotFiles.size(); ++i) {
        if (m_dotFiles[i].path() == path)
            return i;
    }

    return npos;
}

const PtyDot *PtyEnviron::dotFile(std::string_view path) const noexcept
{
    const std::size_t index = dotFileIndexOf(path);

    if (index == npos)
        return nullptr;

    return &m_dotFiles[index];
}

PtyDot *PtyEnviron::dotFile(std::string_view path) noexcept
{
    const std::size_t index = dotFileIndexOf(path);

    if (index == npos)
        return nullptr;

    return &m_dotFiles[index];
}

bool PtyEnviron::hashDotFiles()
{
    bool success = true;

    for (PtyDot &dot : m_dotFiles) {
        if (!dot.enabled())
            continue;

        std::vector<unsigned char> hash =
            crypto::JobHash::hashFile(std::string{dot.path()});

        if (hash.empty()) {
            dot.clearHash();
            success = false;
            continue;
        }

        dot.setHash(std::move(hash));
    }

    return success;
}

void PtyEnviron::clearDotHashes() noexcept
{
    for (PtyDot &dot : m_dotFiles)
        dot.clearHash();
}

std::vector<std::string> PtyEnviron::toStrings() const
{
    std::vector<std::string> result;
    result.reserve(m_environ.size());

    for (const PtyEnv &entry : m_environ) {
        if (!entry.enabled())
            continue;

        std::string value;
        value.reserve(entry.name().size() + 1 + entry.value().size());

        value.append(entry.name());
        value.push_back('=');
        value.append(entry.value());

        result.push_back(std::move(value));
    }

    return result;
}

bool PtyEnviron::isEmpty() const noexcept
{
    return m_environ.empty();
}

std::size_t PtyEnviron::size() const noexcept
{
    return m_environ.size();
}

} // namespace job::io