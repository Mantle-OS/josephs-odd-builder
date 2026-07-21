#pragma once

#include <winsock2.h>

#include <vector>
#include <shared_mutex>

namespace job::net {
class Win32SocketRegistry {
public:
    Win32SocketRegistry() = default;
    ~Win32SocketRegistry() noexcept = default;

    Win32SocketRegistry(const Win32SocketRegistry&) = delete;
    Win32SocketRegistry& operator=(const Win32SocketRegistry&) = delete;
    Win32SocketRegistry(Win32SocketRegistry&&) = delete;
    Win32SocketRegistry& operator=(Win32SocketRegistry&&) = delete;

    [[nodiscard]] int allocate(SOCKET socket) noexcept
    {
        if (socket == INVALID_SOCKET) [[unlikely]]
            return -1;
        try {
            std::unique_lock lock(m_mutex);
            for (size_t i = 0; i < m_table.size(); ++i) {
                if (m_table[i] == INVALID_SOCKET) {
                    m_table[i] = socket;
                    return static_cast<int>(i);
                }
            }

            m_table.push_back(socket);
            return static_cast<int>(m_table.size() - 1);
        } catch (...) {
            return -1;
        }
    }

    [[nodiscard]] SOCKET lookup(int token) const noexcept
    {
        std::shared_lock lock(m_mutex);
        if (token < 0 || token >= static_cast<int>(m_table.size())) [[unlikely]]
            return INVALID_SOCKET;
        return m_table[token];
    }

    void release(int token) noexcept
    {
        std::unique_lock lock(m_mutex);
        if (token >= 0 && token < static_cast<int>(m_table.size())) [[likely]]
            m_table[token] = INVALID_SOCKET;
    }

private:
    mutable std::shared_mutex m_mutex;
    std::vector<SOCKET> m_table;
};

} // namespace job::net
