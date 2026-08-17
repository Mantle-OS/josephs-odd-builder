#include "byte_encoder_factory.h"

#include "gpt2_byte_encoder.h"
#include "raw_byte_encoder.h"

namespace job::token {

IByteEncoder::UPtr ByteEncoderFactory::create(ByteEncoding encoding)
{
    switch (encoding) {
    case ByteEncoding::Raw:
        return RawByteEncoder::createUniq();

    case ByteEncoding::Gpt2:
        return Gpt2ByteEncoder::createUniq();
    }

    return nullptr;
}

} // namespace job::token