#pragma once

#include <memory>
#include <string>

#include <emitters/cpp_emitter.h>

#include "jobserializermsgpack_export.h"

namespace job::serializer::msg_pack {

class JOBSERIALIZERMSGPACK_EXPORT JobEmitterMsgPack final : public CppEmitter
{
public:
    using Ptr  = std::shared_ptr<JobEmitterMsgPack>;
    using WPtr = std::weak_ptr<JobEmitterMsgPack>;
    using UPtr = std::unique_ptr<JobEmitterMsgPack>;

    JobEmitterMsgPack()
    {
        setLanguage(SerializeLanguage::LANG_CPP);
        appendIncludes("msgpack.hpp");
    }

    ~JobEmitterMsgPack() override = default;

    JobEmitterMsgPack(const JobEmitterMsgPack &) = delete;
    JobEmitterMsgPack &operator=(const JobEmitterMsgPack &) = delete;
    JobEmitterMsgPack(JobEmitterMsgPack &&) noexcept = default;
    JobEmitterMsgPack &operator=(JobEmitterMsgPack &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobEmitterMsgPack>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobEmitterMsgPack>();
    }

    [[nodiscard]] std::string appendDecl(const Schema &schema) noexcept override;
    [[nodiscard]] std::string appendImply(const Schema &schema) noexcept override;
};

}