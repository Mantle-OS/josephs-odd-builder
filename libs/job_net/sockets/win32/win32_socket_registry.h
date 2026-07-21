#pragma once

#include <winsock2.h>
#include <vector>
#include <shared_mutex>
#include "jobnet_export.h"
namespace job::net {

class JOBNET_EXPORT Win32SocketRegistry {
public:
    Win32SocketRegistry() = default;
    ~Win32SocketRegistry() noexcept = default;

    // Strict non-copyable/non-movable resource rules
    Win32SocketRegistry(const Win32SocketRegistry&) = delete;
    Win32SocketRegistry& operator=(const Win32SocketRegistry&) = delete;
    Win32SocketRegistry(Win32SocketRegistry&&) = delete;
    Win32SocketRegistry& operator=(Win32SocketRegistry&&) = delete;

    [[nodiscard]] int allocate(SOCKET socket) noexcept {
        if (socket == INVALID_SOCKET) [[unlikely]] {
            return -1;
        }

        try {
            std::unique_lock lock(m_mutex);

            // Recycle slots if possible to keep descriptor indexing low and sequential
            for (size_t i = 0; i < m_table.size(); ++i) {
                if (m_table[i] == INVALID_SOCKET) {
                    m_table[i] = socket;
                    return static_cast<int>(i);
                }
            }

            m_table.push_back(socket);
            return static_cast<int>(m_table.size() - 1);
        } catch (...) {
            // Guarantee noexcept boundary security during vector allocation failures
            return -1;
        }
    }

    [[nodiscard]] SOCKET lookup(int token) const noexcept {
        std::shared_lock lock(m_mutex);
        if (token < 0 || token >= static_cast<int>(m_table.size())) [[unlikely]] {
            return INVALID_SOCKET;
        }
        return m_table[token];
    }

    void release(int token) noexcept {
        std::unique_lock lock(m_mutex);
        if (token >= 0 && token < static_cast<int>(m_table.size())) [[likely]] {
            m_table[token] = INVALID_SOCKET;
        }
    }

private:
    mutable std::shared_mutex m_mutex;
    std::vector<SOCKET> m_table; // Low-overhead sequential offset lookup table
};

} // namespace job::net
