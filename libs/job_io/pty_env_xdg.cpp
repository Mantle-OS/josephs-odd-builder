#include "pty_env_xdg.h"

#include <array>
#include <cstddef>

namespace job::io {

struct PtyEnvXdgMeta {
    std::string_view        name;
    PtyEnvXdg::ValueType    valueType;
    PtyEnvXdg::DefaultType  defaultType;
    std::string_view        defaultValue;
    PtyEnvXdg::SearchGroup  searchGroup;
    std::uint8_t            searchPriority;
    PtyEnvXdg::Constraint   constraints;
    char                    separator;
};

static constexpr std::array<PtyEnvXdgMeta, static_cast<std::size_t>(PtyEnvXdg::Env::Count)> ptyEnvXdgMeta{{
    {
        "XDG_DATA_HOME",
        PtyEnvXdg::ValueType::Path,
        PtyEnvXdg::DefaultType::HomeRelative,
        ".local/share",
        PtyEnvXdg::SearchGroup::Data,
        0,
        PtyEnvXdg::Constraint::AbsolutePath,
        '\0'
    },
    {
        "XDG_CONFIG_HOME",
        PtyEnvXdg::ValueType::Path,
        PtyEnvXdg::DefaultType::HomeRelative,
        ".config",
        PtyEnvXdg::SearchGroup::Config,
        0,
        PtyEnvXdg::Constraint::AbsolutePath,
        '\0'
    },
    {
        "XDG_STATE_HOME",
        PtyEnvXdg::ValueType::Path,
        PtyEnvXdg::DefaultType::HomeRelative,
        ".local/state",
        PtyEnvXdg::SearchGroup::None,
        0,
        PtyEnvXdg::Constraint::AbsolutePath,
        '\0'
    },
    {
        "XDG_CACHE_HOME",
        PtyEnvXdg::ValueType::Path,
        PtyEnvXdg::DefaultType::HomeRelative,
        ".cache",
        PtyEnvXdg::SearchGroup::None,
        0,
        PtyEnvXdg::Constraint::AbsolutePath,
        '\0'
    },
    {
        "XDG_RUNTIME_DIR",
        PtyEnvXdg::ValueType::Path,
        PtyEnvXdg::DefaultType::None,
        "",
        PtyEnvXdg::SearchGroup::None,
        0,
        PtyEnvXdg::Constraint::AbsolutePath |
            PtyEnvXdg::Constraint::UserOwned |
            PtyEnvXdg::Constraint::PrivateMode |
            PtyEnvXdg::Constraint::LocalFilesystem |
            PtyEnvXdg::Constraint::SessionLifetime |
            PtyEnvXdg::Constraint::WarnOnFallback,
        '\0'
    },
    {
        "XDG_DATA_DIRS",
        PtyEnvXdg::ValueType::PathList,
        PtyEnvXdg::DefaultType::Absolute,
        "/usr/local/share:/usr/share",
        PtyEnvXdg::SearchGroup::Data,
        1,
        PtyEnvXdg::Constraint::AbsolutePath,
        ':'
    },
    {
        "XDG_CONFIG_DIRS",
        PtyEnvXdg::ValueType::PathList,
        PtyEnvXdg::DefaultType::Absolute,
        "/etc/xdg",
        PtyEnvXdg::SearchGroup::Config,
        1,
        PtyEnvXdg::Constraint::AbsolutePath,
        ':'
    }
}};

struct PtyEnvXdgBaseMeta {
    std::string_view path;
};

static constexpr std::array<PtyEnvXdgBaseMeta, static_cast<std::size_t>(PtyEnvXdg::BaseDir::Count)> ptyEnvXdgBaseMeta{{
    {".local/share"},
    {".config"},
    {".local/state"},
    {".cache"},
    {""},
    {".local/bin"}
}};

static constexpr const PtyEnvXdgMeta *ptyEnvXdgEntry(PtyEnvXdg::Env env) noexcept
{
    const auto index = static_cast<std::size_t>(env);

    if (index >= ptyEnvXdgMeta.size())
        return nullptr;

    return &ptyEnvXdgMeta[index];
}

static constexpr const PtyEnvXdgBaseMeta *ptyEnvXdgBaseEntry(PtyEnvXdg::BaseDir dir) noexcept
{
    const auto index = static_cast<std::size_t>(dir);

    if (index >= ptyEnvXdgBaseMeta.size())
        return nullptr;

    return &ptyEnvXdgBaseMeta[index];
}

std::string_view PtyEnvXdg::name(Env env) noexcept
{
    const PtyEnvXdgMeta *entry = ptyEnvXdgEntry(env);
    return entry ? entry->name : std::string_view{};
}

PtyEnvXdg::ValueType PtyEnvXdg::valueType(Env env) noexcept
{
    const PtyEnvXdgMeta *entry = ptyEnvXdgEntry(env);
    return entry ? entry->valueType : ValueType::Path;
}

PtyEnvXdg::DefaultType PtyEnvXdg::defaultType(Env env) noexcept
{
    const PtyEnvXdgMeta *entry = ptyEnvXdgEntry(env);
    return entry ? entry->defaultType : DefaultType::None;
}

std::string_view PtyEnvXdg::defaultValue(Env env) noexcept
{
    const PtyEnvXdgMeta *entry = ptyEnvXdgEntry(env);
    return entry ? entry->defaultValue : std::string_view{};
}

PtyEnvXdg::SearchGroup PtyEnvXdg::searchGroup(Env env) noexcept
{
    const PtyEnvXdgMeta *entry = ptyEnvXdgEntry(env);
    return entry ? entry->searchGroup : SearchGroup::None;
}

std::uint8_t PtyEnvXdg::searchPriority(Env env) noexcept
{
    const PtyEnvXdgMeta *entry = ptyEnvXdgEntry(env);
    return entry ? entry->searchPriority : 0;
}

PtyEnvXdg::Constraint PtyEnvXdg::constraints(Env env) noexcept
{
    const PtyEnvXdgMeta *entry = ptyEnvXdgEntry(env);
    return entry ? entry->constraints : Constraint::None;
}

bool PtyEnvXdg::isList(Env env) noexcept
{
    const PtyEnvXdgMeta *entry = ptyEnvXdgEntry(env);
    return entry && entry->valueType == ValueType::PathList;
}

char PtyEnvXdg::separator(Env env) noexcept
{
    const PtyEnvXdgMeta *entry = ptyEnvXdgEntry(env);
    return entry ? entry->separator : '\0';
}

std::string_view PtyEnvXdg::basePath(BaseDir dir) noexcept
{
    const PtyEnvXdgBaseMeta *entry = ptyEnvXdgBaseEntry(dir);
    return entry ? entry->path : std::string_view{};
}

} // namespace job::io