#include "win_fd_reg.h"

#include <limits>
#include <mutex>

namespace job::threads {

WinFdReg &WinFdReg::instance() noexcept
{
    static WinFdReg registry;
    return registry;
}

int WinFdReg::allocate(SOCKET socket) noexcept
{
    if (socket == INVALID_SOCKET) [[unlikely]]
        return -1;

    try {
        std::unique_lock lock(m_mutex);
        if (!m_freeList.empty()) {
            int const token = m_freeList.back();
            m_freeList.pop_back();
            m_table[static_cast<std::size_t>(token)] = socket;
            return token;
        }

        if (m_table.size() >= static_cast<std::size_t>(std::numeric_limits<int>::max())) [[unlikely]]
            return -1;

        m_table.push_back(socket);
        return static_cast<int>(m_table.size() - 1);
    } catch (...) {
        return -1; // Keep noexcept guarantee
    }
}

SOCKET WinFdReg::lookup(int token) const noexcept
{
    if (token < 0) [[unlikely]]
        return INVALID_SOCKET;

    try {
        std::shared_lock lock(m_mutex);

        auto const index = static_cast<std::size_t>(token);
        if (index >= m_table.size()) [[unlikely]]
            return INVALID_SOCKET;

        return m_table[index];
    } catch (...) {
        return INVALID_SOCKET; // Keep noexcept guarantee
    }
}

void WinFdReg::release(int token) noexcept
{
    if (token < 0) [[unlikely]]
        return;

    try {
        std::unique_lock lock(m_mutex);
        auto const index = static_cast<std::size_t>(token);

        if (index < m_table.size() && m_table[index] != INVALID_SOCKET) [[likely]] {
            m_table[index] = INVALID_SOCKET;
            m_freeList.push_back(token); // Add index to free list in O(1) time
        }
    } catch (...) {
        // Keep noexcept guarantee
    }
}

} // namespace job::threads