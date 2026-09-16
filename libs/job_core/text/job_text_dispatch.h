#pragma once

#include "job_text_encoding.h"
#include "job_utf8.h"
#include "job_utf16.h"
#include "job_utf32.h"

namespace job::core::text {

template <TextEncoding Encoding>
struct TextCodecDispatch;

template <>
struct TextCodecDispatch<TextEncoding::Utf8>
{
    using Type = Utf8;
};

template <>
struct TextCodecDispatch<TextEncoding::Utf16>
{
    using Type = Utf16;
};

template <>
struct TextCodecDispatch<TextEncoding::Utf32>
{
    using Type = Utf32;
};

template <TextEncoding Encoding>
using TextCodec = typename TextCodecDispatch<Encoding>::Type;

} // namespace job::core::text