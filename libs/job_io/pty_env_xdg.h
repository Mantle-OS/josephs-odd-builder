#pragma once

#include <cstdint>
#include <string_view>
#include <type_traits>

#include <sys/types.h>

// #include "jobio_export.h" JOBIO_EXPORT

namespace job::io {

class  PtyEnvXdg final
{
public:
    PtyEnvXdg() = delete;
    ~PtyEnvXdg() = delete;

    enum class Env : std::uint8_t {
        DataHome = 0,
        ConfigHome,
        StateHome,
        CacheHome,
        RuntimeDir,
        DataDirs,
        ConfigDirs,

        Count
    };

    enum class BaseDir : std::uint8_t {
        Data = 0,
        Config,
        State,
        Cache,
        Runtime,
        Executable,

        Count
    };

    enum class ValueType : std::uint8_t {
        Path = 0,
        PathList
    };

    enum class DefaultType : std::uint8_t {
        None = 0,
        HomeRelative,
        Absolute
    };

    enum class SearchGroup : std::uint8_t {
        None = 0,
        Data,
        Config
    };

    enum class Constraint : std::uint16_t {
        None            = 0,
        AbsolutePath    = 1 << 0,
        UserOwned       = 1 << 1,
        PrivateMode     = 1 << 2,
        LocalFilesystem = 1 << 3,
        SessionLifetime = 1 << 4,
        WarnOnFallback  = 1 << 5
    };

    [[nodiscard]] static std::string_view name(Env env) noexcept;
    [[nodiscard]] static ValueType valueType(Env env) noexcept;

    [[nodiscard]] static DefaultType defaultType(Env env) noexcept;
    [[nodiscard]] static std::string_view defaultValue(Env env) noexcept;

    [[nodiscard]] static SearchGroup searchGroup(Env env) noexcept;
    [[nodiscard]] static std::uint8_t searchPriority(Env env) noexcept;

    [[nodiscard]] static Constraint constraints(Env env) noexcept;

    [[nodiscard]] static bool isList(Env env) noexcept;
    [[nodiscard]] static char separator(Env env) noexcept;

    [[nodiscard]] static std::string_view basePath(BaseDir dir) noexcept;

    static constexpr mode_t privateDirectoryMode = 0700;
};

[[nodiscard]] constexpr PtyEnvXdg::Constraint operator|(PtyEnvXdg::Constraint lhs,
                                                        PtyEnvXdg::Constraint rhs) noexcept
{
    using T = std::underlying_type_t<PtyEnvXdg::Constraint>;
    return static_cast<PtyEnvXdg::Constraint>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

[[nodiscard]] constexpr PtyEnvXdg::Constraint operator&(PtyEnvXdg::Constraint lhs,
                                                        PtyEnvXdg::Constraint rhs) noexcept
{
    using T = std::underlying_type_t<PtyEnvXdg::Constraint>;
    return static_cast<PtyEnvXdg::Constraint>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

constexpr PtyEnvXdg::Constraint &operator|=(PtyEnvXdg::Constraint &lhs,
                                            PtyEnvXdg::Constraint rhs) noexcept
{
    lhs = lhs | rhs;
    return lhs;
}

[[nodiscard]] constexpr bool hasConstraint(PtyEnvXdg::Constraint value,
                                           PtyEnvXdg::Constraint flag) noexcept
{
    return (value & flag) != PtyEnvXdg::Constraint::None;
}

} // namespace job::io