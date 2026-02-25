#pragma once

#include <job_logger.h>

#include <emitters/cpp_emitter.h>

namespace job::serializer::msg_pack {

using namespace job::serializer;

class JobEmitterMsgPack final : public CppEmitter
{
public:
    JobEmitterMsgPack()
    {
        setLanguage(SerializeLanguage::LANG_CPP);
        appendIncludes("msgpack.hpp");
    }
    ~JobEmitterMsgPack() = default;

    JobEmitterMsgPack(const JobEmitterMsgPack &) = delete;
    JobEmitterMsgPack &operator=(const JobEmitterMsgPack &) = delete;

    [[nodiscard]] std::string appendDecl(const Schema &schema) noexcept override;
    [[nodiscard]] std::string appendImply(const Schema &schema) noexcept override;
};

} // namespace job::serializer::msg_pack
