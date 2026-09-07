#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <msgpack.hpp>

#include <iserializer.h>

#include "jobserializermsgpack_export.h"

namespace job::serializer::msg_pack {

class JOBSERIALIZERMSGPACK_EXPORT JobMsgPackSerializer final : public ISerializer
{
public:
    using Ptr  = std::shared_ptr<JobMsgPackSerializer>;
    using WPtr = std::weak_ptr<JobMsgPackSerializer>;
    using UPtr = std::unique_ptr<JobMsgPackSerializer>;

    JobMsgPackSerializer() = default;
    ~JobMsgPackSerializer() override = default;

    JobMsgPackSerializer(const JobMsgPackSerializer &) = delete;
    JobMsgPackSerializer &operator=(const JobMsgPackSerializer &) = delete;
    JobMsgPackSerializer(JobMsgPackSerializer &&) noexcept = default;
    JobMsgPackSerializer &operator=(JobMsgPackSerializer &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobMsgPackSerializer>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobMsgPackSerializer>();
    }

protected:
    [[nodiscard]] bool encodeBinary(const Schema &schema,
                                    const RuntimeObject &object,
                                    std::vector<uint8_t> &outBuffer) noexcept override;

    [[nodiscard]] bool decodeBinary(const Schema &schema,
                                    RuntimeObject &outObject,
                                    const std::vector<uint8_t> &inBuffer) noexcept override;
};

}