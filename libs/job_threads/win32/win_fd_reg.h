#pragma once

#include "jobthreads_export.h"

#include <winsock2.h>

#include <shared_mutex>
#include <vector>

namespace job::threads {

/*
 * Oh Windows ....
 * POSIX The whole everything is a file no matter what on unix like systems where POSIX is implemented.
 * Windows on the other hand .... Winsock uses SOCKET, whose native type is pointer-sized and cannot safely be narrowed into an int(thanks bill gates).
 * The rest of JOB intentionally uses integer (The whole everything is a file fd design) identities.
 * WinFdReg preserves that interface by mapping:
 *     JOB integer fd/token -> native Win32 SOCKET
 * WinFdReg does not own socket lifetime and never calls closesocket().
 * The component that creates or adopts a socket remains responsible for:
 *     * unregistering it from JobIoAsyncThread;
 *     * releasing its WinFdReg token;
 *     * closing the native socket.
 */

class JOBTHREADS_EXPORT WinFdReg final {
public:
    [[nodiscard]] static WinFdReg &instance() noexcept;

    ~WinFdReg() noexcept = default;

    WinFdReg(const WinFdReg &) = delete;
    WinFdReg &operator=(const WinFdReg &) = delete;
    WinFdReg(WinFdReg &&) = delete;
    WinFdReg &operator=(WinFdReg &&) = delete;

    [[nodiscard]] int allocate(SOCKET socket) noexcept;
    [[nodiscard]] SOCKET lookup(int token) const noexcept;
    void release(int token) noexcept;

private:
    WinFdReg() = default;

    mutable std::shared_mutex m_mutex;
    std::vector<SOCKET>       m_table;
    std::vector<int>          m_freeList;
};

} // namespace job::threads