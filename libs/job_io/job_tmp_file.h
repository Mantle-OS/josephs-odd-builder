#pragma once

#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "job_file.h"

namespace job::io {

class JobTmpFile final
{
public:
    using Ptr  = std::shared_ptr<JobTmpFile>;
    using WPtr = std::weak_ptr<JobTmpFile>;
    using UPtr = std::unique_ptr<JobTmpFile>;

    explicit JobTmpFile(std::filesystem::path path) :
        m_path(std::move(path)),
        m_file(m_path, JobFile::Access::ReadWrite, JobFile::OpenMode::Truncate)
    {
        open();
    }

    JobTmpFile(std::filesystem::path path, std::size_t size, std::byte pattern) :
        m_path(std::move(path)),
        m_file(m_path, JobFile::Access::ReadWrite, JobFile::OpenMode::Truncate)
    {
        open();
        writePattern(size, pattern);
        rewind();
    }

    JobTmpFile(std::filesystem::path path, std::size_t size, char pattern) :
        JobTmpFile(std::move(path), size, static_cast<std::byte>(pattern))
    {
    }

    JobTmpFile(std::filesystem::path path, const std::vector<std::byte> &data) :
        m_path(std::move(path)),
        m_file(m_path, JobFile::Access::ReadWrite, JobFile::OpenMode::Truncate)
    {
        open();

        if (!data.empty())
            writeBytes(data.data(), data.size());

        rewind();
    }

    JobTmpFile(std::filesystem::path path, std::string_view data) :
        m_path(std::move(path)),
        m_file(m_path, JobFile::Access::ReadWrite, JobFile::OpenMode::Truncate)
    {
        open();

        if (!data.empty())
            writeBytes(data.data(), data.size());

        rewind();
    }

    ~JobTmpFile()
    {
        cleanup();
    }

    JobTmpFile(const JobTmpFile &) = delete;
    JobTmpFile &operator=(const JobTmpFile &) = delete;

    JobTmpFile(JobTmpFile &&other) noexcept :
        m_path(std::move(other.m_path)),
        m_file(std::move(other.m_file)),
        m_removeOnDestroy(other.m_removeOnDestroy)
    {
        other.m_path.clear();
        other.m_removeOnDestroy = false;
    }

    JobTmpFile &operator=(JobTmpFile &&other) noexcept
    {
        if (this != &other) {
            cleanup();

            m_path = std::move(other.m_path);
            m_file = std::move(other.m_file);
            m_removeOnDestroy = other.m_removeOnDestroy;

            other.m_path.clear();
            other.m_removeOnDestroy = false;
        }

        return *this;
    }

    [[nodiscard]] static Ptr createShared(std::filesystem::path path)
    {
        return std::make_shared<JobTmpFile>(std::move(path));
    }

    [[nodiscard]] static Ptr createShared(std::filesystem::path path, std::size_t size, std::byte pattern)
    {
        return std::make_shared<JobTmpFile>(std::move(path), size, pattern);
    }

    [[nodiscard]] static Ptr createShared(std::filesystem::path path, std::size_t size, char pattern)
    {
        return std::make_shared<JobTmpFile>(std::move(path), size, pattern);
    }

    [[nodiscard]] static Ptr createShared(std::filesystem::path path, const std::vector<std::byte> &data)
    {
        return std::make_shared<JobTmpFile>(std::move(path), data);
    }

    [[nodiscard]] static Ptr createShared(std::filesystem::path path, std::string_view data)
    {
        return std::make_shared<JobTmpFile>(std::move(path), data);
    }

    [[nodiscard]] static UPtr createUniq(std::filesystem::path path)
    {
        return std::make_unique<JobTmpFile>(std::move(path));
    }

    [[nodiscard]] static UPtr createUniq(std::filesystem::path path, std::size_t size, std::byte pattern)
    {
        return std::make_unique<JobTmpFile>(std::move(path), size, pattern);
    }

    [[nodiscard]] static UPtr createUniq(std::filesystem::path path, std::size_t size, char pattern)
    {
        return std::make_unique<JobTmpFile>(std::move(path), size, pattern);
    }

    [[nodiscard]] static UPtr createUniq(std::filesystem::path path, const std::vector<std::byte> &data)
    {
        return std::make_unique<JobTmpFile>(std::move(path), data);
    }

    [[nodiscard]] static UPtr createUniq(std::filesystem::path path, std::string_view data)
    {
        return std::make_unique<JobTmpFile>(std::move(path), data);
    }

    [[nodiscard]] const std::filesystem::path &path() const noexcept
    {
        return m_path;
    }

    [[nodiscard]] std::string pathString() const
    {
        return m_path.string();
    }

    [[nodiscard]] JobFile &file() noexcept
    {
        return m_file;
    }

    [[nodiscard]] const JobFile &file() const noexcept
    {
        return m_file;
    }

    [[nodiscard]] bool exists() const
    {
        return !m_path.empty() && std::filesystem::exists(m_path);
    }

    [[nodiscard]] bool removeOnDestroy() const noexcept
    {
        return m_removeOnDestroy;
    }

    void setRemoveOnDestroy(bool enabled) noexcept
    {
        m_removeOnDestroy = enabled;
    }

    void rewind()
    {
        if (!m_file.seek(0, JobFile::Seek::Begin))
            throw std::runtime_error("Failed to rewind temporary file");
    }

private:
    void open()
    {
        if (!m_file.openDevice())
            throw std::runtime_error("Failed to create temporary file: " + m_path.string());
    }

    void writeBytes(const void *data, std::size_t size)
    {
        const char *bytes = static_cast<const char *>(data);
        std::size_t written = 0;

        while (written < size) {
            const ssize_t result = m_file.write(bytes + written, size - written);

            if (result < 0) {
                if (errno == EINTR)
                    continue;

                throw std::runtime_error("Failed to write temporary file: " + m_path.string());
            }

            if (result == 0)
                throw std::runtime_error("Failed to write temporary file: " + m_path.string());

            written += static_cast<std::size_t>(result);
        }
    }

    void writePattern(std::size_t size, std::byte pattern)
    {
        constexpr std::size_t kChunkSize = 64 * 1024;

        std::array<std::byte, kChunkSize> buffer{};
        buffer.fill(pattern);

        while (size > 0) {
            const std::size_t count = size < buffer.size() ? size : buffer.size();
            writeBytes(buffer.data(), count);
            size -= count;
        }
    }

    void cleanup() noexcept
    {
        m_file.closeDevice();

        if (!m_removeOnDestroy || m_path.empty())
            return;

        std::error_code error;
        std::filesystem::remove(m_path, error);

        m_path.clear();
    }

    std::filesystem::path m_path;
    JobFile m_file;
    bool m_removeOnDestroy{true};
};

} // namespace job::io