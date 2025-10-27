#include "io_factory.h"

#include <algorithm>
#include <stdexcept>

#include <io_base.h>

#include "pty_io.h"
#include "file_io.h"


namespace job::io {

std::shared_ptr<core::IODevice> IOFactory::createFromType(FactoryType fType, const std::string &target)
{
    switch (fType) {
    case FactoryType::PTY:
        return std::make_shared<PtyIO>();

    case FactoryType::FILE_STD_IN:
        return std::make_shared<FileIO>("stdin", FileMode::StdIn, false);

    case FactoryType::FILE_STD_OUT:
        return std::make_shared<FileIO>("stdout", FileMode::StdOut, true);

    case FactoryType::FILE_STD_ERR:
        return std::make_shared<FileIO>("stderr", FileMode::StdErr, true);

    case FactoryType::FILE_NAME:
        return std::make_shared<FileIO>(target, FileMode::RegularFile, true);

    }

    throw std::runtime_error("Unsupported FactoryType in IOFactory::createFromType()");
}

std::shared_ptr<core::IODevice> IOFactory::createFromURI(const std::string &uri)
{
    const auto sep = uri.find(':');
    if (sep == std::string::npos || sep == 0)
        throw std::invalid_argument("Invalid IO URI format: '" + uri + "'");

    std::string type = uri.substr(0, sep);
    std::string target = (sep < uri.length() - 1) ? uri.substr(sep + 1) : "";
    std::transform(type.begin(), type.end(), type.begin(), ::tolower);

    if (type == "pty"){
        return createFromType(FactoryType::PTY, target);
    }else if (type == "file") {
        if (target == "stdout")
            return createFromType(FactoryType::FILE_STD_OUT, "");
        if (target == "stderr")
            return createFromType(FactoryType::FILE_STD_ERR, "");
        if (target == "stdin")
            return createFromType(FactoryType::FILE_STD_IN, "");
        if (!target.empty())
            return createFromType(FactoryType::FILE_NAME, target);
        throw std::invalid_argument("Missing file target for 'file:' URI");
    }else{
        throw std::runtime_error("Unsupported IO type: '" + type + "'");
    }
}
} // namespace job::io
