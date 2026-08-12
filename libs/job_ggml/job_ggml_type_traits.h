#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include <ggml.h>

#include "job_ggml_enums.h"
#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlTypeTraits
{
public:
    using Ptr  = std::shared_ptr<JobGgmlTypeTraits>;
    using WPtr = std::weak_ptr<JobGgmlTypeTraits>;
    using UPtr = std::unique_ptr<JobGgmlTypeTraits>;

    explicit JobGgmlTypeTraits(JobGgmlType type = JobGgmlType::F32 );
    explicit JobGgmlTypeTraits(enum ggml_type type);
    explicit JobGgmlTypeTraits(const struct ggml_type_traits *typeTraits, enum ggml_type type);
    ~JobGgmlTypeTraits() = default;

    [[nodiscard]] static Ptr createShared(JobGgmlType type = JobGgmlType::F32) { return std::make_shared<JobGgmlTypeTraits>(type); }
    [[nodiscard]] static Ptr createShared(enum ggml_type type) { return std::make_shared<JobGgmlTypeTraits>(type); }
    [[nodiscard]] static Ptr createShared(const struct ggml_type_traits *typeTraits, enum ggml_type type) { return std::make_shared<JobGgmlTypeTraits>(typeTraits, type); }

    [[nodiscard]] static UPtr createUniq(JobGgmlType type = JobGgmlType::F32) { return std::make_unique<JobGgmlTypeTraits>(type); }
    [[nodiscard]] static UPtr createUniq(enum ggml_type type) { return std::make_unique<JobGgmlTypeTraits>(type); }
    [[nodiscard]] static UPtr createUniq(const struct ggml_type_traits *typeTraits, enum ggml_type type) { return std::make_unique<JobGgmlTypeTraits>( typeTraits, type ); }

    JobGgmlTypeTraits(const JobGgmlTypeTraits &) = delete;
    JobGgmlTypeTraits &operator=(const JobGgmlTypeTraits &) = delete;
    JobGgmlTypeTraits(JobGgmlTypeTraits &&) = delete;
    JobGgmlTypeTraits &operator=(JobGgmlTypeTraits &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] JobGgmlType type() const noexcept;
    [[nodiscard]] enum ggml_type ggmlType() const noexcept;

    void setType(JobGgmlType type);
    void setGgmlType(enum ggml_type type);

    [[nodiscard]] const std::string &typeName() const noexcept;

    [[nodiscard]] std::int64_t blockSize() const noexcept;
    [[nodiscard]] std::int64_t blockSizeInterleave() const noexcept;

    [[nodiscard]] std::size_t typeSize() const noexcept;

    [[nodiscard]] bool isQuantized() const noexcept;

    [[nodiscard]] ggml_to_float_t toFloatFunction() const noexcept;
    [[nodiscard]] ggml_from_float_t fromFloatReferenceFunction() const noexcept;

    [[nodiscard]] bool canConvertToFloat() const noexcept;
    [[nodiscard]] bool canConvertFromFloat() const noexcept;



    void convertToFloat(const void *source, float *destination, std::int64_t elementCount) const;
    void convertFromFloatReference(const float *source, void *destination, std::int64_t elementCount) const;
    void setTypeTraits(const struct ggml_type_traits *typeTraits, enum ggml_type type);

    [[nodiscard]] const struct ggml_type_traits *typeTraits() const noexcept;

    void resetTypeTraits();

    [[nodiscard]] static constexpr JobGgmlType fromGgmlType(enum ggml_type type) noexcept { return static_cast<JobGgmlType>(type); }
    [[nodiscard]] static constexpr enum ggml_type toGgmlType(JobGgmlType type) noexcept { return static_cast<enum ggml_type>(type); }


    // IEEE 754-2008 half-precision float16
    [[nodiscard]] static float fp16ToFp32(ggml_fp16_t value) noexcept
    {
        return ggml_fp16_to_fp32(value);
    }

    [[nodiscard]] static ggml_fp16_t fp32ToFp16(float value) noexcept
    {
        return ggml_fp32_to_fp16(value);
    }

    static void fp16ToFp32Row(const ggml_fp16_t *source, float *destination, std::int64_t count)
    {
        ggml_fp16_to_fp32_row(source, destination, count);
    }

    static void fp32ToFp16Row(const float *source, ggml_fp16_t *destination, std::int64_t count)
    {
        ggml_fp32_to_fp16_row(source, destination, count);
    }

    // Google Brain bfloat16
    [[nodiscard]] static ggml_bf16_t fp32ToBf16(float value) noexcept
    {
        return ggml_fp32_to_bf16(value);
    }

    [[nodiscard]] static float bf16ToFp32(ggml_bf16_t value) noexcept
    {
        return ggml_bf16_to_fp32(value);
    }

    static void bf16ToFp32Row(const ggml_bf16_t *source, float *destination, std::int64_t count)
    {
        ggml_bf16_to_fp32_row(source, destination, count);
    }

    static void fp32ToBf16RowRef(const float *source, ggml_bf16_t *destination, std::int64_t count)
    {
        ggml_fp32_to_bf16_row_ref(source, destination, count);
    }

    static void fp32ToBf16Row(const float *source, ggml_bf16_t *destination, std::int64_t count)
    {
        ggml_fp32_to_bf16_row(source, destination, count);
    }

private:
    void fillTypeTraits();
    void clearTypeTraits() noexcept;

    const struct ggml_type_traits   *m_typeTraits{nullptr};     // Borrowed from GGML's process-wide type-traits table.
    enum ggml_type                  m_ggmlType{GGML_TYPE_F32};
    JobGgmlType                     m_type{JobGgmlType::F32};
    std::string                     m_typeName{"f32"};
    std::int64_t                    m_blockSize{0};
    std::int64_t                    m_blockSizeInterleave{0};
    std::size_t                     m_typeSize{0};
    bool                            m_isQuantized{false};
    ggml_to_float_t                 m_toFloat{nullptr};         // Borrowed GGML conversion callback.
    ggml_from_float_t               m_fromFloatRef{nullptr};    // Borrowed GGML reference conversion callback.
};

} // namespace job::ggml