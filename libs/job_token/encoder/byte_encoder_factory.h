#pragma once

#include "ibyte_encoder.h"
#include "job_token_enums.h"
#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT ByteEncoderFactory
{
public:
    ByteEncoderFactory() = delete;
    [[nodiscard]] static IByteEncoder::UPtr create(ByteEncoding encoding);
};

} // namespace job::token