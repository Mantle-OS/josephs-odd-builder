#include "pty_env_posix.h"

#include <array>
#include <cstddef>

namespace job::io {

namespace {

using Env = PtyEnvPosix::Env;
using ValueType = PtyEnvPosix::ValueType;
using Requirement = PtyEnvPosix::Requirement;

struct EnvMeta {
    std::string_view name;
    ValueType valueType;
    Requirement requirement;
    char separator;
};

constexpr std::array<EnvMeta, static_cast<std::size_t>(Env::Count)> envMeta{{
    {"ARFLAGS",    ValueType::Flags,              Requirement::UtilityDefined,   '\0'},
    {"CC",         ValueType::Scalar,             Requirement::UtilityDefined,   '\0'},
    {"CDPATH",     ValueType::PathList,           Requirement::Historical,       ':'},
    {"CFLAGS",     ValueType::Flags,              Requirement::UtilityDefined,   '\0'},
    {"CHARSET",    ValueType::Scalar,             Requirement::Historical,       '\0'},
    {"COLUMNS",    ValueType::PositiveInteger,    Requirement::Historical,       '\0'},
    {"DATEMSK",    ValueType::Path,               Requirement::Historical,       '\0'},
    {"DEAD",       ValueType::Scalar,             Requirement::Historical,       '\0'},
    {"EDITOR",     ValueType::Scalar,             Requirement::Historical,       '\0'},
    {"ENV",        ValueType::ShellCodePath,      Requirement::Historical,       '\0'},
    {"EXINIT",     ValueType::Scalar,             Requirement::Historical,       '\0'},
    {"FC",         ValueType::Scalar,             Requirement::UtilityDefined,   '\0'},
    {"FCEDIT",     ValueType::Scalar,             Requirement::Historical,       '\0'},
    {"FFLAGS",     ValueType::Flags,              Requirement::UtilityDefined,   '\0'},
    {"GET",        ValueType::Scalar,             Requirement::Historical,       '\0'},
    {"GFLAGS",     ValueType::Flags,              Requirement::Historical,       '\0'},
    {"HISTFILE",   ValueType::Path,               Requirement::Historical,       '\0'},
    {"HISTORY",    ValueType::Scalar,             Requirement::Historical,       '\0'},
    {"HISTSIZE",   ValueType::PositiveInteger,    Requirement::Historical,       '\0'},
    {"HOME",       ValueType::Path,               Requirement::LoginInitialized, '\0'},
    {"IFS",        ValueType::Scalar,             Requirement::Historical,       '\0'},
    {"LANG",       ValueType::Locale,             Requirement::Optional,         '\0'},
    {"LC_ALL",     ValueType::Locale,             Requirement::Optional,         '\0'},
    {"LC_COLLATE", ValueType::Locale,             Requirement::Optional,         '\0'},
    {"LC_CTYPE",   ValueType::Locale,             Requirement::Optional,         '\0'},
    {"LC_MESSAGES",ValueType::Locale,             Requirement::Optional,         '\0'},
    {"LC_MONETARY",ValueType::Locale,             Requirement::Optional,         '\0'},
    {"LC_NUMERIC", ValueType::Locale,             Requirement::Optional,         '\0'},
    {"LC_TIME",    ValueType::Locale,             Requirement::Optional,         '\0'},
    {"LDFLAGS",    ValueType::Flags,              Requirement::UtilityDefined,   '\0'},
    {"LEX",        ValueType::Scalar,             Requirement::UtilityDefined,   '\0'},
    {"LFLAGS",     ValueType::Flags,              Requirement::UtilityDefined,   '\0'},
    {"LINENO",     ValueType::PositiveInteger,    Requirement::Historical,       '\0'},
    {"LINES",      ValueType::PositiveInteger,    Requirement::Optional,         '\0'},
    {"LISTER",     ValueType::Scalar,             Requirement::Historical,       '\0'},
    {"LOGNAME",    ValueType::Scalar,             Requirement::LoginInitialized, '\0'},
    {"LPDEST",     ValueType::Scalar,             Requirement::Historical,       '\0'},
    {"MAIL",       ValueType::Path,               Requirement::Historical,       '\0'},
    {"MAILCHECK",  ValueType::PositiveInteger,    Requirement::Historical,       '\0'},
    {"MAILER",     ValueType::Path,               Requirement::Historical,       '\0'},
    {"MAILPATH",   ValueType::PathList,           Requirement::Historical,       ':'},
    {"MAILRC",     ValueType::Path,               Requirement::Historical,       '\0'},
    {"MAKEFLAGS",  ValueType::Flags,              Requirement::UtilityDefined,   '\0'},
    {"MAKESHELL",  ValueType::Path,               Requirement::Historical,       '\0'},
    {"MANPATH",    ValueType::PathList,           Requirement::Historical,       ':'},
    {"MBOX",       ValueType::Path,               Requirement::Historical,       '\0'},
    {"MORE",       ValueType::Flags,              Requirement::Historical,       '\0'},
    {"MSGVERB",    ValueType::Scalar,             Requirement::Optional,         '\0'},
    {"NLSPATH",    ValueType::MessageCatalogPath, Requirement::Optional,         ':'},
    {"NPROC",      ValueType::PositiveInteger,    Requirement::Historical,       '\0'},
    {"OLDPWD",     ValueType::Path,               Requirement::ShellInitialized, '\0'},
    {"OPTARG",     ValueType::Scalar,             Requirement::ShellInitialized, '\0'},
    {"OPTERR",     ValueType::Integer,            Requirement::Historical,       '\0'},
    {"OPTIND",     ValueType::PositiveInteger,    Requirement::ShellInitialized, '\0'},
    {"PAGER",      ValueType::Scalar,             Requirement::Historical,       '\0'},
    {"PATH",       ValueType::PathList,           Requirement::Optional,         ':'},
    {"PPID",       ValueType::PositiveInteger,    Requirement::ShellInitialized, '\0'},
    {"PRINTER",    ValueType::Scalar,             Requirement::Historical,       '\0'},
    {"PROCLANG",   ValueType::Locale,             Requirement::Historical,       '\0'},
    {"PROJECTDIR", ValueType::Path,               Requirement::Historical,       '\0'},
    {"PS1",        ValueType::Scalar,             Requirement::ShellInitialized, '\0'},
    {"PS2",        ValueType::Scalar,             Requirement::ShellInitialized, '\0'},
    {"PS3",        ValueType::Scalar,             Requirement::Historical,       '\0'},
    {"PS4",        ValueType::Scalar,             Requirement::ShellInitialized, '\0'},
    {"PWD",        ValueType::Path,               Requirement::ShellInitialized, '\0'},
    {"RANDOM",     ValueType::Integer,            Requirement::Historical,       '\0'},
    {"SECONDS",    ValueType::PositiveInteger,    Requirement::Historical,       '\0'},
    {"SHELL",      ValueType::Path,               Requirement::LoginInitialized, '\0'},
    {"TERM",       ValueType::Scalar,             Requirement::Optional,         '\0'},
    {"TERMCAP",    ValueType::Scalar,             Requirement::Historical,       '\0'},
    {"TERMINFO",   ValueType::Path,               Requirement::Historical,       '\0'},
    {"TMPDIR",     ValueType::Path,               Requirement::Optional,         '\0'},
    {"TZ",         ValueType::Timezone,           Requirement::Optional,         '\0'},
    {"USER",       ValueType::Scalar,             Requirement::Historical,       '\0'},
    {"VISUAL",     ValueType::Scalar,             Requirement::Historical,       '\0'},
    {"YACC",       ValueType::Scalar,             Requirement::UtilityDefined,   '\0'},
    {"YFLAGS",     ValueType::Flags,              Requirement::UtilityDefined,   '\0'}
}};

[[nodiscard]] constexpr const EnvMeta *meta(Env env) noexcept
{
    const auto index = static_cast<std::size_t>(env);

    if (index >= envMeta.size())
        return nullptr;

    return &envMeta[index];
}

} // namespace

std::string_view PtyEnvPosix::name(Env env) noexcept
{
    const EnvMeta *entry = meta(env);
    return entry ? entry->name : std::string_view{};
}

PtyEnvPosix::ValueType PtyEnvPosix::valueType(Env env) noexcept
{
    const EnvMeta *entry = meta(env);
    return entry ? entry->valueType : ValueType::Scalar;
}

PtyEnvPosix::Requirement PtyEnvPosix::requirement(Env env) noexcept
{
    const EnvMeta *entry = meta(env);
    return entry ? entry->requirement : Requirement::Optional;
}

bool PtyEnvPosix::isList(Env env) noexcept
{
    const EnvMeta *entry = meta(env);

    if (!entry)
        return false;

    return entry->separator != '\0';
}

char PtyEnvPosix::separator(Env env) noexcept
{
    const EnvMeta *entry = meta(env);
    return entry ? entry->separator : '\0';
}

} // namespace job::io