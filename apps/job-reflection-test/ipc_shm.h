#pragma once

#include <cstddef>
#include <memory>
#include <new>
#include <string>

#include <job_shared_memory.h>
#include <job_shared_memory_header.h>

#include "packed.h"

class SharedPacked
{
public:
    SharedPacked() = default;
    ~SharedPacked() = default;

    SharedPacked(const SharedPacked &) = delete;
    SharedPacked &operator=(const SharedPacked &) = delete;
    SharedPacked(SharedPacked &&) = delete;
    SharedPacked &operator=(SharedPacked &&) = delete;

    [[nodiscard]] bool open(const std::string &key)
    {
        m_shm.setKey(key);
        m_shm.setSize(sizeof(Packed));
        m_shm.setMode(job::io::SharedMemoryMode::Write);

        return m_shm.openDevice();
    }

    void close()
    {
        m_shm.closeDevice();
    }

    [[nodiscard]] Packed *asPackedMut() noexcept
    {
        void *data = m_shm.data();
        if (!data)
            return nullptr;

        auto *payload = static_cast<std::byte *>(data) + sizeof(job::io::JobSharedMemoryHeader);
        return std::start_lifetime_as<Packed>(payload);
    }

    [[nodiscard]] const Packed *asPacked() const noexcept
    {
        const void *data = m_shm.data();
        if (!data)
            return nullptr;

        const auto *payload = static_cast<const std::byte *>(data) + sizeof(job::io::JobSharedMemoryHeader);
        return reinterpret_cast<const Packed *>(payload);
    }

private:
    job::io::JobSharedMemory m_shm;
};