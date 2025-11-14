#pragma once
#include <msgpack.hpp>

#include <job_logger.h>

#include <iserializer.h>

namespace job::serializer::msg_pack {
using namespace job::serializer;
class JobMsgPackSerializer final : public ISerializer
{
public:
    JobMsgPackSerializer() = default;
    ~JobMsgPackSerializer() = default;

    JobMsgPackSerializer(const JobMsgPackSerializer &) = delete;
    JobMsgPackSerializer &operator=(const JobMsgPackSerializer &) = delete;

protected:
    [[nodiscard]] bool encodeBinary(const Schema &schema, const RuntimeObject &object,
                                    std::vector<uint8_t> &outBuffer) noexcept override;

    [[nodiscard]] bool decodeBinary( const Schema &schema, RuntimeObject &outObject,
                                    const std::vector<uint8_t> &inBuffer) noexcept override;
};

} // namespace job::serializer::msg_pack
// CHECKPOINT: v1
