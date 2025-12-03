#pragma once

#include <string>
#include <sys/mman.h>

#include <io_base.h>
#include <job_permissions.h>

#include <job_semaphore.h>
#include <job_shared_memory_header.h>

namespace job::io {

enum class SharedMemoryErrors : uint8_t {
    None = 0,
    Attach,
    Size,
    Exists,
    Thread,
    Perm,           // Permissions
    OOR,            // Out of resources
    Key,
    Truncate,
    Header,
    Unknown = 255
};

// Look the commas always go down lol. I didn't intend for that to happen Its a cone of states...
enum class SharedMemoryState : uint8_t {
    Unconnected = 0,
    Connecting,
    Connected,
    Closing,
    Closed,
    Bound,
    Error
};

enum class SharedMemoryMode : uint8_t {
    Read,
    Write // Read + Write access
};

class JobSharedMemory final : public core::IODevice {
public:
    using Ptr = std::shared_ptr<JobSharedMemory>;

    JobSharedMemory();
    ~JobSharedMemory() override;

    void setKey(const std::string &key);
    [[nodiscard]] std::string getKey() const;

    void setSize(size_t size);
    [[nodiscard]] size_t size() const;

    void setMode(SharedMemoryMode mode);
    [[nodiscard]] SharedMemoryMode getMode() const;

    [[nodiscard]] bool openDevice() override;
    void closeDevice() override;

    [[nodiscard]] ssize_t read(char *buffer, size_t maxlen) override;
    [[nodiscard]] ssize_t write(const char *data, size_t len) override;

    [[nodiscard]] bool isOpen() const override;
    [[nodiscard]] int fd() const override;

    void setNonBlocking(bool enabled) override;
    void setReadCallback(ReadCallback cb) override;
    void setPermissions(core::IOPermissions perms) override;

    [[nodiscard]] bool attach();
    [[nodiscard]] bool detach();
    [[nodiscard]] bool isAttached() const;

    [[nodiscard]] SharedMemoryErrors getLastError() const;
    [[nodiscard]] SharedMemoryState getState() const;

    // Highway to the DANGER ZONE ! zero copy later.
    [[nodiscard]] void *data();
    [[nodiscard]] const void *data() const;

    // Header access for diagnostics
    [[nodiscard]] const threads::JobSharedMemoryHeader *header() const;

    // Utils for ring buffer stats
    [[nodiscard]] size_t availableToRead();
    [[nodiscard]] size_t availableToWrite();

private:
    std::string                     m_key;
    size_t                          m_requested_size{0};
    int                             m_shm_fd{-1};
    void                            *m_mapped_ptr{MAP_FAILED};
    threads::JobSharedMemoryHeader  *m_header{nullptr};
    uint8_t                         *m_data_ptr{nullptr};
    bool                            m_nonBlocking{false};
    SharedMemoryMode                m_mode{SharedMemoryMode::Read};
    SharedMemoryState               m_state{SharedMemoryState::Unconnected};
    SharedMemoryErrors              m_last_error{SharedMemoryErrors::None};
    threads::JobSem                 m_sem;

    void cleanup();
};

} // namespace job::io
