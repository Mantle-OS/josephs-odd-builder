#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "io_base.h"

namespace job::io {

class JobFile final : public IODevice
{
public:
    using Ptr  = std::shared_ptr<JobFile>;
    using WPtr = std::weak_ptr<JobFile>;
    using UPtr = std::unique_ptr<JobFile>;

    enum class Access : std::uint8_t
    {
        ReadOnly = 0,
        WriteOnly,
        ReadWrite
    };

    enum class OpenMode : std::uint8_t
    {
        OpenExisting = 0,
        Create,
        Truncate,
        Append
    };

    enum class Handle : std::uint8_t
    {
        None = 0,
        FileDescriptor,
        FilePointer
    };

    enum class Seek : std::uint8_t
    {
        Begin = 0,
        Current,
        End
    };

    //////////////////////////////////////////////////////////
    // Construction
    //////////////////////////////////////////////////////////

    JobFile() = default;

    explicit JobFile(std::filesystem::path path,
                     Access access = Access::ReadOnly,
                     OpenMode openMode = OpenMode::OpenExisting);

    JobFile(int fd, bool owned = false)
        pre(fd >= 0);

    JobFile(FILE *fp, bool owned = false)
        pre(fp != nullptr);

    ~JobFile() override;

    JobFile(const JobFile &) = delete;
    JobFile &operator=(const JobFile &) = delete;
    JobFile(JobFile &&other) noexcept;
    JobFile &operator=(JobFile &&other) noexcept;

    //////////////////////////////////////////////////////////
    // Factories
    //////////////////////////////////////////////////////////

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobFile>();
    }

    [[nodiscard]] static Ptr createShared(std::filesystem::path path,
                                          Access access = Access::ReadOnly,
                                          OpenMode openMode = OpenMode::OpenExisting)
    {
        return std::make_shared<JobFile>(std::move(path), access, openMode);
    }

    [[nodiscard]] static Ptr createShared(int fd, bool owned = false)
        pre(fd >= 0)
    {
        return std::make_shared<JobFile>(fd, owned);
    }

    [[nodiscard]] static Ptr createShared(FILE *fp, bool owned = false)
        pre(fp != nullptr)
    {
        return std::make_shared<JobFile>(fp, owned);
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobFile>();
    }

    [[nodiscard]] static UPtr createUniq(std::filesystem::path path,
                                         Access access = Access::ReadOnly,
                                         OpenMode openMode = OpenMode::OpenExisting)
    {
        return std::make_unique<JobFile>(std::move(path), access, openMode);
    }

    [[nodiscard]] static UPtr createUniq(int fd, bool owned = false)
        pre(fd >= 0)
    {
        return std::make_unique<JobFile>(fd, owned);
    }

    [[nodiscard]] static UPtr createUniq(FILE *fp, bool owned = false)
        pre(fp != nullptr)
    {
        return std::make_unique<JobFile>(fp, owned);
    }

    //////////////////////////////////////////////////////////
    // IODevice
    //////////////////////////////////////////////////////////

    [[nodiscard]] bool openDevice() override;
    void closeDevice() override;

    [[nodiscard]] ssize_t read(char *buffer, std::size_t maxlen) override;
    [[nodiscard]] ssize_t write(const char *data, std::size_t len) override;

    [[nodiscard]] bool isOpen() const override;
    [[nodiscard]] int fd() const override;

    void setNonBlocking(bool enabled) override;
    void setReadCallback(ReadCallback cb) override;

    [[nodiscard]] bool flush() override;

    [[nodiscard]] IOPermissions permissions() const override;
    void setPermissions(IOPermissions perms) override;

    //////////////////////////////////////////////////////////
    // File identity
    //////////////////////////////////////////////////////////

    [[nodiscard]] const std::filesystem::path &path() const noexcept;
    [[nodiscard]] std::string pathString() const;

    [[nodiscard]] Access access() const noexcept;
    [[nodiscard]] OpenMode openMode() const noexcept;
    [[nodiscard]] Handle handle() const noexcept;

    [[nodiscard]] bool owned() const noexcept;
    [[nodiscard]] bool hasPath() const noexcept;

    [[nodiscard]] FILE *fp() noexcept;
    [[nodiscard]] const FILE *fp() const noexcept;

    //////////////////////////////////////////////////////////
    // File positioning / size
    //////////////////////////////////////////////////////////

    [[nodiscard]] std::size_t size() const;
    [[nodiscard]] std::size_t tell() const;

    [[nodiscard]] bool seek(std::int64_t offset, Seek whence);

    //////////////////////////////////////////////////////////
    // Bulk IO
    //////////////////////////////////////////////////////////

    [[nodiscard]] ssize_t readAll(std::vector<std::uint8_t> &output);
    [[nodiscard]] ssize_t readAll(std::string &output);

    //////////////////////////////////////////////////////////
    // Typed IO
    //////////////////////////////////////////////////////////

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    [[nodiscard]] bool readValue(T &value)
    {
        return read(reinterpret_cast<char *>(&value), sizeof(T)) == static_cast<ssize_t>(sizeof(T));
    }

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    [[nodiscard]] bool writeValue(const T &value)
    {
        return write(reinterpret_cast<const char *>(&value), sizeof(T)) == static_cast<ssize_t>(sizeof(T));
    }

private:
    //////////////////////////////////////////////////////////
    // Open / close
    //////////////////////////////////////////////////////////

    [[nodiscard]] bool openPath();

    [[nodiscard]] bool initFd(int fd, bool owned)
        pre(fd >= 0);

    [[nodiscard]] bool initFp(FILE *fp, bool owned)
        pre(fp != nullptr);

    void closeFd() noexcept;
    void closeFp() noexcept;

    //////////////////////////////////////////////////////////
    // POSIX helpers
    //////////////////////////////////////////////////////////

    [[nodiscard]] int openFlags() const noexcept;
    [[nodiscard]] bool updatePermissions() noexcept;

    //////////////////////////////////////////////////////////
    // Shared cleanup / move
    //////////////////////////////////////////////////////////

    void reset() noexcept;
    void moveFrom(JobFile &&other) noexcept;

    std::filesystem::path m_path;

    int m_fd{-1};
    FILE *m_fp{nullptr};

    Access m_access{Access::ReadOnly};
    OpenMode m_openMode{OpenMode::OpenExisting};
    Handle m_handle{Handle::None};

    bool m_owned{false};
    bool m_nonBlocking{false};

    ReadCallback m_readCallback;
};

} // namespace job::io