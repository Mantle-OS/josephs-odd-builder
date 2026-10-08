#pragma once

#include "job_fb_fixed_info.h"
#include "job_fb_format_result.h"
#include "job_fb_variable_info.h"

namespace job::render {

class JobFbFormat
{
public:
    using FourCc = JobFbFormatResult::FourCc;

    JobFbFormat() = delete;
    ~JobFbFormat() = delete;

    JobFbFormat(const JobFbFormat &) = delete;
    JobFbFormat &operator=(const JobFbFormat &) = delete;

    JobFbFormat(JobFbFormat &&) = delete;
    JobFbFormat &operator=(JobFbFormat &&) = delete;

    [[nodiscard]] static JobFbFormatResult detect(const JobFbFixedInfo &fixed, const JobFbVariableInfo &variable) noexcept;

    [[nodiscard]] static core::FormatType fromFourCc(FourCc fourCc) noexcept;

private:
    [[nodiscard]] static JobFbFormatResult detectFourCc(const JobFbFixedInfo &fixed, const JobFbVariableInfo &variable) noexcept;
    [[nodiscard]] static JobFbFormatResult detectLegacy(const JobFbFixedInfo &fixed, const JobFbVariableInfo &variable) noexcept;
    [[nodiscard]] static core::FormatType detectTrueColor(const JobFbVariableInfo &variable) noexcept;
    [[nodiscard]] static core::FormatType detectMonochrome(const JobFbFixedInfo &fixed, const JobFbVariableInfo &variable) noexcept;
    [[nodiscard]] static core::FormatType detectPseudoColor(const JobFbFixedInfo &fixed, const JobFbVariableInfo &variable) noexcept;
};

}