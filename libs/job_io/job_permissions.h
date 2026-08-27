#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <sys/stat.h>

namespace job::io {

using PermissionBits = mode_t;

enum class IOPermissions : PermissionBits
{
    None                = 0,

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

enum class PermissionStringType : uint8_t
{
    Both = 0,
    OctalOnly,
    NoOctal
};

namespace permissions {

inline constexpr std::size_t kPermissionValueCount = 4096;
inline constexpr PermissionBits kPermissionMask = 07777;

struct PermissionString final
{
    std::array<char, 5> octal{};
    std::array<char, 10> symbolic{};
    std::array<char, 15> both{};
};

[[nodiscard]] consteval PermissionString makePermissionString(std::size_t value)
{
    PermissionString result{};

    const PermissionBits mode = static_cast<PermissionBits>(value);

    result.octal[0] = static_cast<char>('0' + ((value >> 9) & 07));
    result.octal[1] = static_cast<char>('0' + ((value >> 6) & 07));
    result.octal[2] = static_cast<char>('0' + ((value >> 3) & 07));
    result.octal[3] = static_cast<char>('0' + (value & 07));
    result.octal[4] = '\0';

    result.symbolic = {
        '-',
        '-',
        '-',
        '-',
        '-',
        '-',
        '-',
        '-',
        '-',
        '\0'
    };

    if (mode & S_IRUSR)
        result.symbolic[0] = 'r';

    if (mode & S_IWUSR)
        result.symbolic[1] = 'w';

    if (mode & S_IXUSR)
        result.symbolic[2] = 'x';

    if (mode & S_ISUID)
        result.symbolic[2] = (mode & S_IXUSR) ? 's' : 'S';

    if (mode & S_IRGRP)
        result.symbolic[3] = 'r';

    if (mode & S_IWGRP)
        result.symbolic[4] = 'w';

    if (mode & S_IXGRP)
        result.symbolic[5] = 'x';

    if (mode & S_ISGID)
        result.symbolic[5] = (mode & S_IXGRP) ? 's' : 'S';

    if (mode & S_IROTH)
        result.symbolic[6] = 'r';

    if (mode & S_IWOTH)
        result.symbolic[7] = 'w';

    if (mode & S_IXOTH)
        result.symbolic[8] = 'x';

    if (mode & S_ISVTX)
        result.symbolic[8] = (mode & S_IXOTH) ? 't' : 'T';

    result.both[0] = result.octal[0];
    result.both[1] = result.octal[1];
    result.both[2] = result.octal[2];
    result.both[3] = result.octal[3];
    result.both[4] = ' ';

    for (std::size_t i = 0; i < 9; ++i)
        result.both[i + 5] = result.symbolic[i];

    result.both[14] = '\0';

    return result;
}

[[nodiscard]] consteval auto makePermissionStringTable()
{
    std::array<PermissionString, kPermissionValueCount> table{};

    for (std::size_t i = 0; i < table.size(); ++i)
        table[i] = makePermissionString(i);

    return table;
}

inline constexpr auto kPermissionStrings = makePermissionStringTable();

[[nodiscard]] constexpr std::size_t index(IOPermissions perms) noexcept
{
    return static_cast<std::size_t>(toMode(perms) & kPermissionMask);
}

} // namespace permissions

// No allocation. No snprintf. No formatting work.
//
// OctalOnly: "0755"
// NoOctal:   "rwxr-xr-x"
// Both:      "0755 rwxr-xr-x"
[[nodiscard]] constexpr std::string_view toStringView(
    IOPermissions perms,
    PermissionStringType type = PermissionStringType::OctalOnly) noexcept
{
    const auto &entry = permissions::kPermissionStrings[permissions::index(perms)];

    switch (type) {
    case PermissionStringType::Both:
        return std::string_view(entry.both.data(), 14);

    case PermissionStringType::NoOctal:
        return std::string_view(entry.symbolic.data(), 9);

    case PermissionStringType::OctalOnly:
    default:
        return std::string_view(entry.octal.data(), 4);
    }
}

// Compatibility API.
// std::string will generally use SSO for all three representations,
// so this should still be dramatically cheaper than formatting them.
[[nodiscard]] inline std::string toString(
    IOPermissions perms,
    PermissionStringType type = PermissionStringType::OctalOnly)
{
    return std::string(toStringView(perms, type));
}

[[nodiscard]] constexpr std::string_view toName(IOPermissions perms) noexcept
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

} // namespace job::io