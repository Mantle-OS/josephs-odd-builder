#include "job_shared_memory.h"
#include "job_logger.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <algorithm>
#include <new>

namespace job::io {

using namespace job::threads;

JobSharedMemory::JobSharedMemory() = default;

JobSharedMemory::~JobSharedMemory()
{
    if (isAttached())
        if(!detach())
            JOB_LOG_WARN("[JobSharedMemory] Object cleanup failed to detach this is not the greatest thing");
}

void JobSharedMemory::setKey(const std::string &key)
{
    if (m_state != SharedMemoryState::Unconnected) {
        JOB_LOG_WARN("[JobSharedMemory] Cannot set key while connected");
        return;
    }
    m_key = key;
}

std::string JobSharedMemory::getKey() const
{
    return m_key;
}

void JobSharedMemory::setSize(size_t size)
{
    if (m_state != SharedMemoryState::Unconnected) {
        JOB_LOG_WARN("[JobSharedMemory] Cannot set size while connected");
        return;
    }
    m_requested_size = size;
}

size_t JobSharedMemory::size() const
{
    if (isAttached() && m_header)
        return m_header->totalSize;

    return m_requested_size;
}

void JobSharedMemory::setMode(SharedMemoryMode mode)
{
    if (m_state != SharedMemoryState::Unconnected) {
        JOB_LOG_WARN("[JobSharedMemory] Cannot set mode while connected");
        return;
    }
    m_mode = mode;
}

SharedMemoryMode JobSharedMemory::getMode() const
{
    return m_mode;
}

void JobSharedMemory::setNonBlocking(bool enabled)
{
    m_nonBlocking = enabled;
}



bool JobSharedMemory::openDevice()
{
    return attach();
}
void JobSharedMemory::closeDevice()
{
    if(!detach())
        JOB_LOG_WARN("[JobSharedMemory] Failed to detach on close of the shared memory .... ligit life choices");
}

bool JobSharedMemory::isOpen() const
{
    return isAttached();
}

int JobSharedMemory::fd() const
{
    return m_shm_fd;
}

void JobSharedMemory::setReadCallback(ReadCallback)
{
    JOB_LOG_WARN("[JobSharedMemory] Read callbacks not supported");
}

void JobSharedMemory::setPermissions(core::IOPermissions perms)
{
    // well ... you're are already connected. what premissions do you have ? live
    if (isAttached() && m_shm_fd != -1){
        if (::fchmod(m_shm_fd, core::toMode(perms)) != 0) {
            JOB_LOG_ERROR("[JobSharedMemory] fchmod failed: {}", std::strerror(errno));
            m_last_error = SharedMemoryErrors::Perm;
        }else{
            // callback the permissions police
            IODevice::setPermissions(perms);
        }
    }
}

// !! PRAY !!
bool JobSharedMemory::attach()
{
    if (isAttached()){
        m_last_error = SharedMemoryErrors::Exists;
        return true;
    }

    if (m_key.empty() || m_key[0] != '/') {
        JOB_LOG_ERROR("[JobSharedMemory] Key must start with '/'");
        m_last_error = SharedMemoryErrors::Key;
        return false;
    }

    if (m_requested_size == 0 && m_mode == SharedMemoryMode::Write) {
        JOB_LOG_ERROR("[JobSharedMemory] Attach size cannot be 0, m_mode can not be write");
        m_last_error = SharedMemoryErrors::Size;
        return false;
    }

    m_state = SharedMemoryState::Connecting;

    size_t headerSize = sizeof(JobSharedMemoryHeader);

    // ensure alignment logic if needed, but struct alignas(64) "handles" the start
    size_t totalMappingSize = headerSize + m_requested_size;

    int oflags = 0;
    if (m_mode == SharedMemoryMode::Write) {
        oflags = O_RDWR | O_CREAT;
    } else {
        oflags = O_RDWR;
    }

    int mmap_prot = PROT_READ | PROT_WRITE;

    mode_t mode = job::core::toMode(permissions());

    // open sesame SHM
    m_shm_fd = ::shm_open(m_key.c_str(), oflags, mode);
    if (m_shm_fd == -1) {
        JOB_LOG_ERROR("[JobSharedMemory] shm_open failed: {}", std::strerror(errno));
        m_last_error = SharedMemoryErrors::Attach;
        m_state = SharedMemoryState::Error;
        return false;
    }

    // I am 6 foot tall but only if I read and write
    // sizing (only writer truncates)
    if (m_mode == SharedMemoryMode::Write) {
        // set the physical file size to Header + Data
        if (::ftruncate(m_shm_fd, static_cast<off_t>(totalMappingSize)) == -1) {
            JOB_LOG_ERROR("[JobSharedMemory] ftruncate failed");
            ::close(m_shm_fd);
            m_shm_fd = -1;
            m_last_error = SharedMemoryErrors::Truncate;
            m_state = SharedMemoryState::Error;
            return false;
        }
    } else {
        // reader actual size
        struct stat sb{};
        if (::fstat(m_shm_fd, &sb) != -1)
            totalMappingSize = static_cast<size_t>(sb.st_size);
    }

    // talk to the cartographer they have the maps (try MAP_HUGETLB 1st)
    m_mapped_ptr = ::mmap(nullptr, totalMappingSize, mmap_prot, MAP_SHARED | MAP_HUGETLB, m_shm_fd, 0);
    if (m_mapped_ptr == MAP_FAILED) {
        // Fallback: Standard 4KB Pages
        // This handles cases where the OS hasn't allocated huge pages
        m_mapped_ptr = ::mmap(nullptr, totalMappingSize, mmap_prot, MAP_SHARED, m_shm_fd, 0);
    }

    if (m_mapped_ptr == MAP_FAILED) {
        JOB_LOG_ERROR("[JobSharedMemory] mmap failed {}", std::strerror(errno));
        ::close(m_shm_fd);
        m_shm_fd = -1;
        m_state = SharedMemoryState::Error;
        return false;
    }

    // here a couple of pointers :>)
    m_header = static_cast<JobSharedMemoryHeader*>(m_mapped_ptr);
    m_data_ptr = static_cast<uint8_t*>(m_mapped_ptr) + headerSize;

    // initialize header first time creation. if writer, check magic. If 0, assume its just created.
    if (m_mode == SharedMemoryMode::Write) {
        // is it zero? (fresh pages are zeroed by ?OS?)
        if (m_header->magic != JobSharedMemoryHeader::kSmMagic) {
            // PLACEMENT NEW to initialize std::atomic and other fields properly
            // 1st time using "new" as not a keyword
            new(m_header)JobSharedMemoryHeader();
            JobSharedMemoryHeader::create(*m_header, 1, static_cast<uint32_t>(m_requested_size));
        }
    } else {
        // validate (reader side)
        if (!m_header->validSharedMemMagicAndSize()) {
            JOB_LOG_ERROR("[JobSharedMemory] Invalid Header Magic/Version");
            cleanup();
            m_last_error = SharedMemoryErrors::Header;
            return false;
        }
    }


    //sem is now a signal, not a mutex. | append "_sem" to the key because why not
    std::string semName = m_key + "_sig";
    m_sem.setName(semName);

    // ring buffer signal, usually post when we write. let's open it with 0. ?????
    JobSemFlags semFlags = (m_mode == SharedMemoryMode::Write) ?
                               threads::JobSemFlags::Create | threads::JobSemFlags::UnlinkOnDestroy :
                               JobSemFlags::None;

    // value 0: waiters block until post occurs hopefully
    auto oret = m_sem.open(semFlags, mode, 0);
    if (oret != JobSemRet::OK) {
        JOB_LOG_ERROR("[JobSharedMemory] Failed to open semaphore '{}': {}", semName, threads::semiRetToString(oret));
        cleanup();
        return false;
    }

    m_state = SharedMemoryState::Connected;
    return true;
}

bool JobSharedMemory::detach()
{
    if (!isAttached())
        return true;
    m_state = SharedMemoryState::Closing;
    cleanup();
    m_state = SharedMemoryState::Closed;
    return true;
}

void JobSharedMemory::cleanup() {
    size_t mapSize = sizeof(JobSharedMemoryHeader) + (m_header ? m_header->totalSize : m_requested_size);

    if (m_mapped_ptr != MAP_FAILED && m_mapped_ptr != nullptr)
        ::munmap(m_mapped_ptr, mapSize);

    if (m_shm_fd != -1)
        ::close(m_shm_fd);

    auto cret =  m_sem.close();
    if(cret != threads::JobSemRet::OK){
        JOB_LOG_WARN("[JobSharedMemory] This ius BAD could not close the shared memory in cleanup: {}",
                     threads::semiRetToString(cret));
    }

    m_mapped_ptr = MAP_FAILED;
    m_header = nullptr;
    m_data_ptr = nullptr;
    m_shm_fd = -1;
}

bool JobSharedMemory::isAttached() const
{
    return m_state == SharedMemoryState::Connected && m_header != nullptr;
}


size_t JobSharedMemory::availableToRead()
{
    if (!isAttached()){
        JOB_LOG_WARN("[JobSharedMemory] Could not get availableToRead from memory -> not attached");
        m_last_error = SharedMemoryErrors::Attach;
        return 0;
    }
    size_t w = m_header->writePos.load(std::memory_order_acquire);
    size_t r = m_header->readPos.load(std::memory_order_relaxed);
    return w - r;
}

size_t JobSharedMemory::availableToWrite()
{
    if (!isAttached()){
        JOB_LOG_WARN("[JobSharedMemory] Could not get availableToWrite -> not attached");
        m_last_error = SharedMemoryErrors::Attach;
        return 0;
    }

    size_t w = m_header->writePos.load(std::memory_order_relaxed);
    size_t r = m_header->readPos.load(std::memory_order_acquire);
    size_t cap = m_header->totalSize;
    return cap - (w - r);
}

ssize_t JobSharedMemory::read(char *buffer, size_t maxlen) {
    if (!isAttached()){
        JOB_LOG_WARN("[JobSharedMemory] Could not read from memory not attached");
        m_last_error = SharedMemoryErrors::Attach;
        return -1;
    }

    // wait for data
    size_t avail = availableToRead();
    while (avail == 0) {
        if (m_nonBlocking) {
            errno = EAGAIN;
            return -1;
        }
        // Block semaphore until signaled
         auto rret = m_sem.wait();
        if (rret != JobSemRet::OK) {
             JOB_LOG_WARN("[JobSharedMemory] Could not read from memory JobSem wait function failed, {}",
                         threads::semiRetToString(rret));
             m_last_error = SharedMemoryErrors::Thread;
            return -1;
        }
        avail = availableToRead();
    }

    // how much to read
    size_t toRead = std::min(maxlen, avail);
    size_t currentReadPos = m_header->readPos.load(std::memory_order_relaxed);
    size_t cap = m_header->totalSize;

    // perform copy (handle wrap around)
    size_t offset = currentReadPos % cap;
    // size_t endOffset = (currentReadPos + toRead) % cap;

    if (offset + toRead <= cap) {
        // contiguous copy
        std::memcpy(buffer, m_data_ptr + offset, toRead);
    } else {
        // split copy
        size_t firstChunk = cap - offset;
        std::memcpy(buffer, m_data_ptr + offset, firstChunk);
        std::memcpy(buffer + firstChunk, m_data_ptr, toRead - firstChunk);
    }

    // update reader pos
    m_header->readPos.store(currentReadPos + toRead, std::memory_order_release);

    return static_cast<ssize_t>(toRead);
}

ssize_t JobSharedMemory::write(const char *data, size_t len)
{
    if (!isAttached()){
        JOB_LOG_WARN("[JobSharedMemory] Could not write to memory not attached or no data");
        m_last_error = SharedMemoryErrors::Attach;
        return -1;
    }

    if (m_mode != SharedMemoryMode::Write){
        JOB_LOG_WARN("[JobSharedMemory] Could not write to memory the mode is not in write mode");
        m_last_error = SharedMemoryErrors::Perm;
        return -1;
    }

    // check space
    // Shit If full, we currently fail.
    // ?????? Ideally I might want to block-wait for space, but usually writers define the pace ??????????????
    size_t freeSpace = availableToWrite();
    if (len > freeSpace) {
        // "Could/should" wait here if we implemented a "space available" semaphore
        errno = ENOSPC;
        m_last_error = SharedMemoryErrors::OOR;
        return 0;
    }

    size_t currentWritePos = m_header->writePos.load(std::memory_order_relaxed);
    size_t cap = m_header->totalSize;

    // perform copy (handle wrap-around)
    size_t offset = currentWritePos % cap;

    // remember what I said about praying
    if (offset + len <= cap) {
        std::memcpy(m_data_ptr + offset, data, len);
    } else {
        size_t firstChunk = cap - offset;
        std::memcpy(m_data_ptr + offset, data, firstChunk);
        std::memcpy(m_data_ptr, data + firstChunk, len - firstChunk);
    }

    // update write pos
    m_header->writePos.store(currentWritePos + len, std::memory_order_release);

    // yell at the reader
    if(m_sem.post() != threads::JobSemRet::OK)
        JOB_LOG_WARN("[JobSharedMemory] Could not post(write) to the shared memory");

    return static_cast<ssize_t>(len);
}

void *JobSharedMemory::data()
{
    return isAttached() ? m_mapped_ptr : nullptr;
}

const void *JobSharedMemory::data() const
{
    return (isAttached()) ? m_mapped_ptr : nullptr;
}

const JobSharedMemoryHeader *JobSharedMemory::header() const
{
    return m_header;
}

SharedMemoryErrors JobSharedMemory::getLastError() const
{
    return m_last_error;
}
SharedMemoryState JobSharedMemory::getState() const
{
    return m_state;
}

} // namespace job::io
