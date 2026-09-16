#pragma once

#include "job_unicode_scalar.h"

#include "job_utf8_status.h"
#include "job_utf8_scalar_kernel.h"
#include "job_utf8_kernel.h"
#include "job_ascii_simd_tile.h"
#include "job_ascii_simd_kernel.h"
#include "job_utf8_simd_tile.h"
#include "job_utf8_simd_kernel.h"
#include "job_utf8.h"

#include "job_utf16_status.h"
#include "job_utf16_scalar_kernel.h"
#include "job_utf16_kernel.h"
#include "job_utf16_simd_tile.h"
#include "job_utf16_simd_kernel.h"
#include "job_utf16.h"

#include "job_utf32_status.h"
#include "job_utf32_scalar_kernel.h"
#include "job_utf32_kernel.h"
#include "job_utf32_simd_tile.h"
#include "job_utf32_simd_kernel.h"
#include "job_utf32.h"

#include "job_text_encoding.h"
#include "job_text_transcode_status.h"
#include "job_text_transcoder.h"
#include "job_text_dispatch.h"

#include "job_utf_stream_concept.h"
#include "job_utf_streambuf.h"