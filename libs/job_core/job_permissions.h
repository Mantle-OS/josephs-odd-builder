#pragma once

#include <cstdint>
#include <string>
#include <cstdio>
#include <sys/stat.h>

namespace job::core {

using PermissionBits = mode_t;

enum class IOPermissions : PermissionBits {
    None = 0,
    OwnerRead           = S_IRUSR,
    OwnerWrite          = S_IWUSR,
    OwnerExec           = S_IXUSR,
    GroupRead           = S_IRGRP,
    GroupWrite          = S_IWGRP,
    GroupExec           = S_IXGRP,
    OtherRead           = S_IROTH,
    OtherWrite          = S_IWOTH,
    OtherExec           = S_IXOTH,
    SetUserId           = S_ISUID,
    SetGroupId          = S_ISGID,
    StickyBit           = S_ISVTX,
    ReadUser            = S_IRUSR,                                                      // 0400
    ReadWriteUser       = S_IRUSR | S_IWUSR,                                            // 0600
    ReadWriteAll        = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH,    // 0666
    DefaultFile         = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH,                        // 0644
    DefaultDirectory    = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH,              // 0755
    BadIdeas            = S_IRWXU | S_IRWXG | S_IRWXO,                                  // 0777 the what the fuck are you doing mode
    ReadWrite           = ReadWriteAll
};

[[nodiscard]] constexpr PermissionBits toMode(IOPermissions perms) noexcept
{
    return static_cast<PermissionBits>(perms);
}

enum class PermissionStringType : uint8_t {
    Both = 0,
    OctalOnly,
    NoOctal
};


// OctalOnly: "0755" | NoOctal:  "rwxr-xr-x" | Both :  "0755 rwxr-xr-x"
[[nodiscard]] inline std::string toString(IOPermissions perms, PermissionStringType octal = PermissionStringType::OctalOnly)
{
    PermissionBits m = static_cast<PermissionBits>(perms);
    std::string ret;

    char sym[10];
    for (char &c : sym)
        c = '-';
    sym[9] = '\0';

    // User
    if (m & S_IRUSR)
        sym[0] = 'r';

    if (m & S_IWUSR)
        sym[1] = 'w';

    {
        char c = (m & S_IXUSR) ? 'x' : '-';
        if (m & S_ISUID)
            c = (c == 'x') ? 's' : 'S';
        sym[2] = c;
    }

    if (m & S_IRGRP)
        sym[3] = 'r';

    if (m & S_IWGRP)
        sym[4] = 'w';

    {
        char c = (m & S_IXGRP) ? 'x' : '-';
        if (m & S_ISGID) {
            // setgid modifies group exec position
            c = (c == 'x') ? 's' : 'S';
        }
        sym[5] = c;
    }

    if (m & S_IROTH)
        sym[6] = 'r';

    if (m & S_IWOTH)
        sym[7] = 'w';

    {
        char c = (m & S_IXOTH) ? 'x' : '-';
        if (m & S_ISVTX) {
            // sticky bit modifies other exec position
            c = (c == 'x') ? 't' : 'T';
        }
        sym[8] = c;
    }

    ret.clear();

    if(octal == PermissionStringType::NoOctal){
        ret.reserve(10);
        ret.append(sym);
    }else{
        char oct[7];
        auto masked = static_cast<unsigned>(m & 07777u);
        std::snprintf(oct, sizeof(oct), "%04o", masked);
        if(octal == PermissionStringType::Both){
            ret.reserve(14);
            ret.append(oct);
            ret.push_back(' ');
            ret.append(sym);
        }else{
            ret.reserve(4);
            ret.append(oct);
        }
    }

    return ret;
}

// maybe add more later
[[nodiscard]] inline const char *toName(IOPermissions perms)
{
    switch (perms) {
    case IOPermissions::None:
        return "None";
    case IOPermissions::ReadUser:
        return "ReadUser";
    case IOPermissions::ReadWriteUser:
        return "ReadWriteUser";
    case IOPermissions::ReadWriteAll:
        return "ReadWriteAll";
    case IOPermissions::DefaultFile:
        return "DefaultFile";
    case IOPermissions::DefaultDirectory:
        return "DefaultDirectory";
    case IOPermissions::BadIdeas:
        return "WhyAreYouDoingThis";
    default:
        return "Custom";
    }
}

} // namespace job::core

