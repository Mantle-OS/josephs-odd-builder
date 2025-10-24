#pragma once

#include "io_base.h"

#include <memory>
#include <string>

namespace job::io {

enum class FactoryType {
    PTY,
    FILE_STD_IN,
    FILE_STD_OUT,
    FILE_STD_ERR,
    FILE_NAME,
    SERIAL
};

class IOFactory {
public:
    [[nodiscard]] static std::shared_ptr<IODevice> createFromType(FactoryType fType, const std::string &target);
    [[nodiscard]] static std::shared_ptr<IODevice> createFromURI(const std::string &uri);
};

} // namespace job::io
