#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>

#include <job_mmap.h>

#include "packed.h"

class MappedFile
{
public:
    MappedFile(const std::string &path, std::size_t size) :
        m_map(path)
    {
        if (!m_map.openDevice())
            throw std::runtime_error("Failed to open mapped file");

        if (m_map.fileSize() < size) {
            if (!m_map.grow(size))
                throw std::runtime_error("Failed to grow mapped file");
        }

        if (m_map.fileSize() != size)
            throw std::runtime_error("Mapped file size does not match requested size");

        if (!m_map.addr())
            throw std::runtime_error("Mapped file has no mapped address");
    }

    ~MappedFile() = default;

    MappedFile(const MappedFile &) = delete;
    MappedFile &operator=(const MappedFile &) = delete;
    MappedFile(MappedFile &&) noexcept = default;
    MappedFile &operator=(MappedFile &&) noexcept = default;

    [[nodiscard]] const Packed *asPacked(std::size_t offset = 0) const noexcept
    {
        auto *ptr = const_cast<char *>(static_cast<const char *>(m_map.addr()) + offset);
        return std::start_lifetime_as<Packed>(ptr);
    }

    [[nodiscard]] Packed *asPackedMut(std::size_t offset = 0) noexcept
    {
        return std::start_lifetime_as<Packed>(static_cast<char *>(m_map.addr()) + offset);
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return m_map.mappedSize();
    }

    [[nodiscard]] job::io::JobMmap &map() noexcept
    {
        return m_map;
    }

    [[nodiscard]] const job::io::JobMmap &map() const noexcept
    {
        return m_map;
    }

private:
    job::io::JobMmap m_map;
};