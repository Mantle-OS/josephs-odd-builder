#pragma once

#include <cstddef>
#include <cstdint>

namespace job::net {

enum class NetIoStatus : std::uint8_t {
    Ok = 0,      // moved `bytes` (may be fewer than requested)
    WouldBlock,  // moved nothing; retry after the matching readable/writable event
    Closed,      // orderly peer shutdown (read only)
    Error        // failed; consult lastError()
};

struct NetIoResult {
    NetIoStatus    status{NetIoStatus::Error};
    std::size_t bytes{0};

    [[nodiscard]] constexpr bool ok() const noexcept
    {
        return status == NetIoStatus::Ok;
    }

    [[nodiscard]] constexpr bool wouldBlock() const noexcept
    {
        return status == NetIoStatus::WouldBlock;
    }

    [[nodiscard]] constexpr bool closed() const noexcept
    {
        return status == NetIoStatus::Closed;
    }

    [[nodiscard]] constexpr bool failed() const noexcept
    {
        return status == NetIoStatus::Error;
    }

    // True when the whole requested transfer completed in this call.
    [[nodiscard]] constexpr bool complete(std::size_t requested) const noexcept
    {
        return status == NetIoStatus::Ok && bytes == requested;
    }

    [[nodiscard]] static constexpr NetIoResult transferred(std::size_t n) noexcept
    {
        return NetIoResult{NetIoStatus::Ok, n};
    }

    [[nodiscard]] static constexpr NetIoResult retry() noexcept
    {
        return NetIoResult{NetIoStatus::WouldBlock, 0};
    }

    [[nodiscard]] static constexpr NetIoResult eof() noexcept
    {
        return NetIoResult{NetIoStatus::Closed, 0};
    }

    [[nodiscard]] static constexpr NetIoResult error() noexcept
    {
        return NetIoResult{NetIoStatus::Error, 0};
    }
};

} // namespace job::net