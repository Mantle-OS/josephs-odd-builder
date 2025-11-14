#pragma once

#include <memory>
#include <string>

#include <io_base.h>

namespace job::io {

enum class FactoryType {
    PTY,
    FILE_STD_IN,
    FILE_STD_OUT,
    FILE_STD_ERR,
    FILE_NAME
};

class IOFactory {
public:
    [[nodiscard]] static std::shared_ptr<core::IODevice> createFromType(FactoryType fType, const std::string &target);
    [[nodiscard]] static std::shared_ptr<core::IODevice> createFromURI(const std::string &uri);
};

} // namespace job::io
// CHECKPOINT: v1
