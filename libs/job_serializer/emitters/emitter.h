#pragma once

#include <memory>
#include <set>
#include <string>
#include <utility>

#include "jobserializer_export.h"
#include "schema.h"

namespace job::serializer {

class JOBSERIALIZER_EXPORT Emitter
{
public:
    using Ptr  = std::shared_ptr<Emitter>;
    using WPtr = std::weak_ptr<Emitter>;
    using UPtr = std::unique_ptr<Emitter>;

    Emitter() = default;
    virtual ~Emitter() = default;

    Emitter(const Emitter &) = delete;
    Emitter &operator=(const Emitter &) = delete;
    Emitter(Emitter &&) noexcept = default;
    Emitter &operator=(Emitter &&) noexcept = default;

    [[nodiscard]] std::pair<std::string, std::string> render(const Schema &schema) noexcept;
    [[nodiscard]] std::string renderSingle(const Schema &schema) noexcept;

    [[nodiscard]] virtual std::string renderDecl(const Schema &schema) = 0;
    [[nodiscard]] virtual std::string renderImply(const Schema &schema) = 0;
    [[nodiscard]] virtual std::string languageType(const Field &field) = 0;

    [[nodiscard]] Schema lastSchema() const;

    void appendIncludes(const std::string &in);
    [[nodiscard]] std::set<std::string> getIncludes() const;

    [[nodiscard]] SerializeLanguage language() const noexcept;
    void setLanguage(SerializeLanguage newLanguage) noexcept;

protected:
    Schema                  m_lastSchema;
    std::set<std::string>   m_includes;
    SerializeLanguage       m_language{SerializeLanguage::LANG_CPP};
};

} // namespace job::serializer