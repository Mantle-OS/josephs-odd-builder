#pragma once

#include <cstdint>
#include <memory>

#include <job_ggml_enums.h>

namespace job::model {

class JobIMem;

class JobIMemConfig
{
public:
    using Ptr  = std::shared_ptr<JobIMemConfig>;
    using WPtr = std::weak_ptr<JobIMemConfig>;
    using UPtr = std::unique_ptr<JobIMemConfig>;

    enum class JobCtxType : uint32_t
    {
        Default = 0,
        Mtp,
        Unknown
    };

    JobIMemConfig() = default;
    ~JobIMemConfig() = default;

    JobIMemConfig(const JobIMemConfig &) = default;
    JobIMemConfig &operator=(const JobIMemConfig &) = default;

    JobIMemConfig(JobIMemConfig &&) noexcept = default;
    JobIMemConfig &operator=(JobIMemConfig &&) noexcept = default;

    JobIMemConfig &operator=(JobIMem *other) noexcept
    {
        m_other = other;
        return *this;
    }

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobIMemConfig>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobIMemConfig>();
    }

    [[nodiscard]] ggml::JobGgmlType keyType() const noexcept
    {
        return m_keyType;
    }

    void setKeyType(ggml::JobGgmlType value) noexcept
    {
        m_keyType = value;
    }

    [[nodiscard]] ggml::JobGgmlType valType() const noexcept
    {
        return m_valType;
    }

    void setValType(ggml::JobGgmlType value) noexcept
    {
        m_valType = value;
    }

    [[nodiscard]] bool swa() const noexcept
    {
        return m_swa;
    }

    void setSwa(bool value) noexcept
    {
        m_swa = value;
    }

    [[nodiscard]] JobCtxType ctxType() const noexcept
    {
        return m_ctxType;
    }

    void setCtxType(JobCtxType value) noexcept
    {
        m_ctxType = value;
    }

    [[nodiscard]] JobIMem *other() const noexcept
    {
        return m_other;
    }

    void setOther(JobIMem *value) noexcept
    {
        m_other = value;
    }

    [[nodiscard]] bool isValid() const noexcept
    {
        if (m_keyType == ggml::JobGgmlType::Count ||
            m_valType == ggml::JobGgmlType::Count ||
            m_ctxType == JobCtxType::Unknown)
            return false;
        return true;
    }

    [[nodiscard]] bool reset() noexcept
    {
        m_other   = nullptr;
        m_keyType = ggml::JobGgmlType::Count;
        m_valType = ggml::JobGgmlType::Count;
        m_swa     = false;
        m_ctxType = JobCtxType::Unknown;
        return m_other == nullptr &&
               m_keyType == ggml::JobGgmlType::Count &&
               m_valType == ggml::JobGgmlType::Count &&
               !m_swa &&
               m_ctxType == JobCtxType::Unknown;
    }


private:
    JobIMem *m_other{nullptr};

    ggml::JobGgmlType m_keyType{ggml::JobGgmlType::Count};
    ggml::JobGgmlType m_valType{ggml::JobGgmlType::Count};

    bool m_swa{false}; // Use full-size SWA cache.

    JobCtxType m_ctxType{JobCtxType::Unknown};
};

} // namespace job::model