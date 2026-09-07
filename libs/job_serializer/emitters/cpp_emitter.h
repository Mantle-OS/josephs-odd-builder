#pragma once

#include <memory>
#include <string>

#include "emitters/emitter.h"
#include "jobserializer_export.h"

namespace job::serializer {

class JOBSERIALIZER_EXPORT CppEmitter : public Emitter
{
public:
    using Ptr  = std::shared_ptr<CppEmitter>;
    using WPtr = std::weak_ptr<CppEmitter>;
    using UPtr = std::unique_ptr<CppEmitter>;

    CppEmitter();
    ~CppEmitter() override = default;

    CppEmitter(const CppEmitter &) = delete;
    CppEmitter &operator=(const CppEmitter &) = delete;
    CppEmitter(CppEmitter &&) noexcept = default;
    CppEmitter &operator=(CppEmitter &&) noexcept = default;

    [[nodiscard]] std::string renderDecl(const Schema &schema) override;
    [[nodiscard]] std::string renderImply(const Schema &schema) override;

    [[nodiscard]] virtual std::string simpleType(const std::string &type);
    [[nodiscard]] std::string languageType(const Field &field) override;

    [[nodiscard]] virtual std::string appendDecl([[maybe_unused]] const Schema &schema) noexcept = 0;
    [[nodiscard]] virtual std::string appendImply([[maybe_unused]] const Schema &schema) noexcept = 0;

    [[nodiscard]] std::string exportMacro() const;
    void setExportMacro(const std::string &newExportMacro);

    [[nodiscard]] std::string exportHeader() const;
    void setExportHeader(const std::string &newExportHeader);

private:
    std::string m_exportMacro;
    std::string m_exportHeader;
};

} // namespace job::serializer