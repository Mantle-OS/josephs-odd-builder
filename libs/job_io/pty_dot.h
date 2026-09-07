#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <job_base_obj.h>

// #include "jobio_export.h" JOBIO_EXPORT

namespace job::io {

class  PtyDot : public core::BaseObject
{
public:
    using Ptr = std::shared_ptr<PtyDot>;
    using WPtr = std::weak_ptr<PtyDot>;
    using UPtr = std::unique_ptr<PtyDot>;

    enum class Role : std::uint8_t {
        SystemEnvironment = 0,
        SystemProfile,
        SystemRc,
        UserProfile,
        UserRc,
        ShellEnv,
        Project,
        Custom
    };

    enum class Format : std::uint8_t {
        Environment = 0,
        Shell,
        DotEnv,
        DirEnv,
        Custom
    };

    PtyDot(std::string path, Role role, Format format, bool enabled = true);
    ~PtyDot() override = default;

    PtyDot(const PtyDot &) = default;
    PtyDot &operator=(const PtyDot &) = default;
    PtyDot(PtyDot &&) noexcept = default;
    PtyDot &operator=(PtyDot &&) noexcept = default;

    template <typename... Args>
    [[nodiscard]] static Ptr createShared(Args &&...args)
    {
        return std::make_shared<PtyDot>(std::forward<Args>(args)...);
    }

    template <typename... Args>
    [[nodiscard]] static UPtr createUniq(Args &&...args)
    {
        return std::make_unique<PtyDot>(std::forward<Args>(args)...);
    }

    [[nodiscard]] bool enabled() const noexcept;
    [[nodiscard]] Role role() const noexcept;
    [[nodiscard]] Format format() const noexcept;

    [[nodiscard]] std::string_view path() const noexcept;
    [[nodiscard]] const std::vector<unsigned char> &hash() const noexcept;

    [[nodiscard]] bool hasHash() const noexcept;

    void setEnabled(bool enabled) noexcept;
    void setRole(Role role) noexcept;
    void setFormat(Format format) noexcept;

    void setPath(std::string path);
    void setHash(std::vector<unsigned char> hash);

    void clearHash() noexcept;

private:
    bool m_enabled{true};

    Role m_role{Role::Custom};
    Format m_format{Format::Custom};

    std::string m_path;
    std::vector<unsigned char> m_hash;
};

} // namespace job::io