#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <job_base_obj.h>

// #include "jobio_export.h"

namespace job::io {

class  PtyEnv : public core::BaseObject
{
public:
    using Ptr = std::shared_ptr<PtyEnv>;
    using WPtr = std::weak_ptr<PtyEnv>;
    using UPtr = std::unique_ptr<PtyEnv>;

    enum class EnvType : std::uint8_t {
        Posix = 0,
        Xdg,
        Custom
    };

    PtyEnv(std::string_view name,
           std::string_view value,
           bool enabled = false,
           EnvType envType = EnvType::Custom);

    ~PtyEnv() override = default;

    PtyEnv(const PtyEnv &) = default;
    PtyEnv &operator=(const PtyEnv &) = default;
    PtyEnv(PtyEnv &&) noexcept = default;
    PtyEnv &operator=(PtyEnv &&) noexcept = default;

    template <typename... Args>
    [[nodiscard]] static Ptr createShared(Args &&...args)
    {
        return std::make_shared<PtyEnv>(std::forward<Args>(args)...);
    }

    template <typename... Args>
    [[nodiscard]] static UPtr createUniq(Args &&...args)
    {
        return std::make_unique<PtyEnv>(std::forward<Args>(args)...);
    }

    [[nodiscard]] virtual bool enabled() const noexcept;
    [[nodiscard]] virtual EnvType envType() const noexcept;

    [[nodiscard]] virtual std::string_view name() const noexcept;
    [[nodiscard]] virtual std::string_view value() const noexcept;

    [[nodiscard]] virtual std::span<const std::string_view> values() const;

    virtual void setEnabled(bool enabled) noexcept;
    virtual void setEnvType(EnvType envType) noexcept;
    virtual void setName(std::string name);
    virtual void setValue(std::string value);

private:
    bool m_enabled{false};
    EnvType m_envType{EnvType::Custom};

    std::string m_name;
    std::string m_value;

    [[=core::NoSerialize{}]]
    mutable std::vector<std::string_view> m_values;
};

} // namespace job::io