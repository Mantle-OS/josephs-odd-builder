#pragma once

#include <file_io.h>

#include "job_serializer_utils.h"
#include "schema.h"
#include "emitters/emitter.h"
#include "runtime_object.h"
#include "iserializer.h"

namespace job::serializer {

class Writer : public io::FileIO {
public:
    explicit Writer(const std::filesystem::path &path);
    ~Writer()
    {
        if(isOpen())
            closeDevice();
    }

    [[nodiscard]] bool writeSchema(const Schema &schema,
                                   SerializeFormat mode = SerializeFormat::Unknown) noexcept;

    [[nodiscard]] virtual bool writeEmitter(Emitter &emitter,
                                    const Schema &schema,
                                    const std::filesystem::path &header_file,
                                    const std::filesystem::path &source_file) noexcept;

    [[nodiscard]] bool writeRuntime(ISerializer &ser, const Schema &schema,
                                    const RuntimeObject &object,
                                    SerializeFormat fmt = SerializeFormat::Unknown) noexcept;

protected:
    [[nodiscard]] virtual bool writeYaml(const Schema &schema) noexcept;
    [[nodiscard]] virtual bool writeJson(const Schema &schema) noexcept;
    [[nodiscard]] virtual bool writeText(const Schema &schema) noexcept;
    [[nodiscard]] virtual bool writeBinary(const Schema &schema) noexcept;
};

} // namespace job::serializer

