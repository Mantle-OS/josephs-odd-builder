#pragma once

#include <string>

#include "emitters/emitter.h"

namespace job::serializer {

class QtEmitter : public Emitter {
public:
    QtEmitter();
    virtual ~QtEmitter();

    QtEmitter(const QtEmitter &) = delete;
    QtEmitter &operator=(const QtEmitter &) = delete;

    [[nodiscard]] virtual std::string renderDecl(const Schema &schema) override;
    [[nodiscard]] virtual std::string renderImply(const Schema &schema) override;

    // parse out the types keep virtual "in case" the plugin does something funky
    [[nodiscard]] virtual std::string simpleType(const std::string &type);
    [[nodiscard]] virtual std::string languageType(const Field &field) override;

    // For the "plugin" type tp fill out
    [[nodiscard]] virtual std::string appendDecl([[maybe_unused]] const Schema &schema) noexcept  = 0;
    [[nodiscard]] virtual std::string appendImply([[maybe_unused]] const Schema &schema) noexcept = 0;

private:
};

} // namespace job::serializer

