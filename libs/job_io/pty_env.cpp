#include "pty_env.h"

namespace job::io {

PtyEnv::PtyEnv(std::string_view name, std::string_view value, bool enabled, EnvType envType) :
    m_enabled(enabled),
    m_envType(envType),
    m_name(name),
    m_value(value)
{
}

bool PtyEnv::enabled() const noexcept
{
    return m_enabled;
}

PtyEnv::EnvType PtyEnv::envType() const noexcept
{
    return m_envType;
}

std::string_view PtyEnv::name() const noexcept
{
    return m_name;
}

std::string_view PtyEnv::value() const noexcept
{
    return m_value;
}

std::span<const std::string_view> PtyEnv::values() const
{
    m_values.clear();

    std::size_t begin = 0;

    while (begin <= m_value.size()) {
        const std::size_t end = m_value.find(':', begin);

        if (end == std::string::npos) {
            m_values.emplace_back(m_value.data() + begin, m_value.size() - begin);
            break;
        }

        m_values.emplace_back(m_value.data() + begin, end - begin);
        begin = end + 1;
    }

    return m_values;
}

void PtyEnv::setEnabled(bool enabled) noexcept
{
    m_enabled = enabled;
}

void PtyEnv::setEnvType(EnvType envType) noexcept
{
    m_envType = envType;
}

void PtyEnv::setName(std::string name)
{
    m_name = std::move(name);
}

void PtyEnv::setValue(std::string value)
{
    m_value = std::move(value);
    m_values.clear();
}

} // namespace job::io