#include "emitter.h"

#include <job_logger.h>

namespace job::serializer
{

std::pair<std::string, std::string> Emitter::render( const Schema &schema) noexcept
{
    std::pair<std::string, std::string> ret;
    if (!schema.isValid()){
        JOB_LOG_WARN("Schema is invalid not return the header and source");
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
    if (!schema.isValid()){
        JOB_LOG_WARN("Schema is invalid not return the header and source");
        return ret;
    }

    m_lastSchema = schema;
    return renderDecl(schema);
}

Emitter::~Emitter()
{
    if(!m_includes.empty())
        m_includes.clear();
}

Schema Emitter::lastSchema() const
{
    return m_lastSchema;
}

void Emitter::appendIncludes(const std::string &in)
{
    if(!m_includes.contains(in))
        m_includes.insert(in);
}

std::set<std::string> Emitter::getIncludes() const
{
    return m_includes;
}

SerializeLanguage Emitter::language() const
{
    return m_language;
}

void Emitter::setLanguage(SerializeLanguage newLanguage)
{
    m_language = newLanguage;
}

} // namespace job::serializer
// CHECKPOINT: v2
