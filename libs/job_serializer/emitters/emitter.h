#pragma once

#include <string>
#include <set>

#include "schema.h"

namespace job::serializer {

class Emitter {

public:

    Emitter(){}
    ~Emitter();

    Emitter(const Emitter &) = delete;
    Emitter &operator=(const Emitter &) = delete;

    // Used when we generate c++, c etc where there are imply & decl
    [[nodiscard]] std::pair<std::string, std::string> render(const Schema &schema) noexcept;

    // Used when generating things like py, java, ect where there is only decl
    [[nodiscard]] std::string renderSingle(const Schema &schema) noexcept;

    // Here is where the language emitter overrides (or well statrts to)
    [[nodiscard]] virtual std::string renderDecl(const Schema &schema) = 0;
    [[nodiscard]] virtual std::string renderImply(const Schema &schema) = 0;

    // Language must fill this out. noexcept ? Static ?
    [[nodiscard]] virtual std::string languageType(const Field &field) = 0;

    // The cachhed last schema
    Schema lastSchema() const;

    // imports, #includes etc
    void appendIncludes(const std::string &in);
    std::set<std::string>  getIncludes() const;

    // Simple place to setup the programming language
    SerializeLanguage language() const;
    void setLanguage(SerializeLanguage newLanguage);

protected:
    Schema                  m_lastSchema;
    std::set<std::string>   m_includes;
    SerializeLanguage       m_language{SerializeLanguage::LANG_CPP};
};

} // namespace job::serializer
// CHECKPOINT: v2
