#include "runtime_object.h"

#include <job_logger.h>

namespace job::serializer {

bool RuntimeObject::hasField(const std::string &name) const noexcept
{
    bool ret = false;

    do {
        if (name.empty())
            break;

        ret = (m_fields.find(name) != m_fields.end());
    } while (0);

    return ret;
}

std::optional<FieldValue> RuntimeObject::getField(const std::string &name) const noexcept
{
    std::optional<FieldValue> ret;

    do {
        if (name.empty())
            break;

        auto it = m_fields.find(name);
        if (it == m_fields.end())
            break;

        ret = it->second;
    } while (0);

    return ret;
}

bool RuntimeObject::setField(const std::string &name, const FieldValue &val) noexcept
{
    bool ret = false;

    do {
        if (name.empty()) {
            JOB_LOG_WARN("[runtime] cannot set empty field name");
            break;
        }

        try {
            m_fields[name] = val;
            ret = true;
        } catch (const std::exception &e) {
            JOB_LOG_ERROR("[runtime] failed to set field '{}': {}", name, e.what());
        }

    } while (0);

    return ret;
}

bool RuntimeObject::removeField(const std::string &name) noexcept
{
    bool ret = false;

    do {
        if (name.empty())
            break;

        auto it = m_fields.find(name);
        if (it == m_fields.end())
            break;

        m_fields.erase(it);
        ret = true;
    } while (0);

    return ret;
}

void RuntimeObject::clear() noexcept
{
    m_fields.clear();
    JOB_LOG_DEBUG("[runtime] cleared all fields");
}

const std::unordered_map<std::string, FieldValue> &RuntimeObject::fields() const noexcept
{
    return m_fields;
}

std::unordered_map<std::string, FieldValue> &RuntimeObject::fields() noexcept
{
    return m_fields;
}

} // namespace job::serializer

