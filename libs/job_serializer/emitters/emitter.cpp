#include "emitter.h"

#include <job_logger.h>

namespace job::serializer {

std::pair<std::string, std::string> Emitter::render(const Schema &schema) noexcept
{
    std::pair<std::string, std::string> ret;

    if (!schema.isValid()) {
        JOB_LOG_WARN("[emitter] schema is invalid, cannot render declaration and implementation");
        return ret;
    }

    m_lastSchema = schema;
    ret.first = renderDecl(schema);
    ret.second = renderImply(schema);

    return ret;
}

std::string Emitter::renderSingle(const Schema &schema) noexcept
{
    std::string ret;

    if (!schema.isValid()) {
        JOB_LOG_WARN("[emitter] schema is invalid, cannot render declaration");
        return ret;
    }

    m_lastSchema = schema;
    return renderDecl(schema);
}

Schema Emitter::lastSchema() const
{
    return m_lastSchema;
}

void Emitter::appendIncludes(const std::string &in)
{
    m_includes.insert(in);
}

std::set<std::string> Emitter::getIncludes() const
{
    return m_includes;
}

SerializeLanguage Emitter::language() const noexcept
{
    return m_language;
}

void Emitter::setLanguage(SerializeLanguage newLanguage) noexcept
{
    m_language = newLanguage;
}

} // namespace job::serializer