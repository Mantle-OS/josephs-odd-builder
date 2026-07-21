#pragma once
#include <cstdint>

namespace job::threads {


// EPOLLIN     Read
// EPOLLOUT    Write
// EPOLLERR    Error
// EPOLLHUP    HangUp
// EPOLLET     EdgeTriggered
// uint32_t e (callback param)IOEvent e
// e & EPOLLIN (checking a bit)
enum class IOEvent : std::uint32_t {
    None          = 0,
    Read          = 1u << 0,
    Write         = 1u << 1,
    Error         = 1u << 2,
    HangUp        = 1u << 3,
    // Advisory: honored by the posix/epoll backend, silently not
    // translated by the win32/WSAPoll backend (WSAPoll is always
    // level-triggered -- see job_io_async_thread_win32.cpp for why
    // that's compatible with existing drain-until-EAGAIN call sites).
    EdgeTriggered = 1u << 4,
};

constexpr IOEvent operator|(IOEvent a, IOEvent b) noexcept
{
    return static_cast<IOEvent>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

constexpr IOEvent operator&(IOEvent a, IOEvent b) noexcept
{
    return static_cast<IOEvent>(static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b));
}

constexpr IOEvent &operator|=(IOEvent &a, IOEvent b) noexcept
{
    a = a | b;
    return a;
}

[[nodiscard]] constexpr bool hasEvent(IOEvent val, IOEvent bit) noexcept
{
    return (static_cast<std::uint32_t>(val) & static_cast<std::uint32_t>(bit)) != 0;
}

} // namespace job::threads