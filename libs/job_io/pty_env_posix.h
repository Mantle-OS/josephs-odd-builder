#pragma once

#include <cstdint>
#include <string_view>

// #include "jobio_export.h"
// JOBIO_EXPORT
namespace job::io {

class  PtyEnvPosix final
{
public:
    PtyEnvPosix() = delete;
    ~PtyEnvPosix() = delete;

    enum class Env : std::uint8_t {
        Arflags = 0,
        Cc,
        Cdpath,
        Cflags,
        Charset,
        Columns,
        Datemsk,
        Dead,
        Editor,
        Env,
        Exinit,
        Fc,
        Fcedit,
        Fflags,
        Get,
        Gflags,
        Histfile,
        History,
        Histsize,
        Home,
        Ifs,
        Lang,
        LcAll,
        LcCollate,
        LcCtype,
        LcMessages,
        LcMonetary,
        LcNumeric,
        LcTime,
        Ldflags,
        Lex,
        Lflags,
        Lineno,
        Lines,
        Lister,
        Logname,
        Lpdest,
        Mail,
        Mailcheck,
        Mailer,
        Mailpath,
        Mailrc,
        Makeflags,
        Makeshell,
        Manpath,
        Mbox,
        More,
        Msgverb,
        Nlspath,
        Nproc,
        Oldpwd,
        Optarg,
        Opterr,
        Optind,
        Pager,
        Path,
        Ppid,
        Printer,
        Proclang,
        Projectdir,
        Ps1,
        Ps2,
        Ps3,
        Ps4,
        Pwd,
        Random,
        Seconds,
        Shell,
        Term,
        Termcap,
        Terminfo,
        Tmpdir,
        Tz,
        User,
        Visual,
        Yacc,
        Yflags,

        Count
    };

    enum class ValueType : std::uint8_t {
        Scalar = 0,
        Path,
        PathList,
        Locale,
        PositiveInteger,
        Integer,
        Timezone,
        MessageCatalogPath,
        ShellCodePath,
        Flags
    };

    enum class Requirement : std::uint8_t {
        Optional = 0,
        LoginInitialized,
        ShellInitialized,
        UtilityDefined,
        Historical
    };

    [[nodiscard]] static std::string_view name(Env env) noexcept;
    [[nodiscard]] static ValueType valueType(Env env) noexcept;
    [[nodiscard]] static Requirement requirement(Env env) noexcept;

    [[nodiscard]] static bool isList(Env env) noexcept;
    [[nodiscard]] static char separator(Env env) noexcept;
};

} // namespace job::io