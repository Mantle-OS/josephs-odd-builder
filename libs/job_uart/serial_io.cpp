#include "serial_io.h"

#include <cstring>

#include <unistd.h>
#include <termios.h>
#include <poll.h>

namespace job::uart {

SerialIO::SerialIO(std::shared_ptr<threads::JobIoAsyncThread> loop) :
    m_loop{loop}
{
}

SerialIO::~SerialIO()
{
    closeDevice();
}

bool SerialIO::openDevice()
{
    closeDevice();

    if (m_portName.empty() || !std::filesystem::exists(m_portName)) {
        updateError(Error::NotFound);
        return false;
    }

    if (access(m_portName.c_str(), R_OK | W_OK) != 0) {
        updateError(Error::PermissionError);
        return false;
    }

    m_fd = ::open(m_portName.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (m_fd < 0) {
        updateError(Error::OpenError);
        return false;
    }

    struct termios tty {};
    if (tcgetattr(m_fd, &tty) != 0) {
        updateError(Error::OpenError);
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    cfsetospeed(&tty, toSpeed(baudRate));
    cfsetispeed(&tty, toSpeed(baudRate));
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= toDataBits(dataBits);
    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_cflag |= toParity(parity, tty);
    tty.c_cflag |= toStopBits(stopBits);
    toFlowControl(flowControl, tty);

    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_oflag &= ~OPOST;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(m_fd, TCSANOW, &tty) != 0) {
        updateError(Error::OpenError);
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    try {
        m_loop = std::make_shared<threads::JobIoAsyncThread>();
    } catch (const std::exception& e) {
        JOB_LOG_ERROR("[SerialIO] Failed to create JobIoAsyncThread: {}", e.what());
        updateError(Error::LoopError);
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    if (!m_loop->registerFD(m_fd, EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET, [this](uint32_t e) { onEvents(e); })) {
        JOB_LOG_ERROR("[SerialIO] Failed to register FD with event loop!");
        updateError(Error::LoopError);
        m_loop->stop();
        m_loop.reset();
        ::close(m_fd);
        m_fd = -1;
        return false;
    }
    m_loop->start();

    m_isOpen = true;
    updateState(State::Connected);
    return true;
}

void SerialIO::closeDevice()
{
    if (m_fd != -1 && m_loop) {
        m_loop->unregisterFD(m_fd);
    }

    if (m_loop) {
        m_loop->stop();
        m_loop.reset();
    }

    if (m_isRecording)
        stopRecordingInternal();

    if (m_fd != -1) {
        ::close(m_fd);
        m_fd = -1;
    }

    m_isOpen = false;
    updateState(State::Disconnected);
}

ssize_t SerialIO::read(char *buffer, size_t maxlen)
{
    if (!m_isOpen || m_fd == -1)
        return -1;

    ssize_t n = ::read(m_fd, buffer, maxlen);
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        return 0; // Not an error, just no data
    if (n < 0)
        updateError(Error::ReadError);

    return n;
}

ssize_t SerialIO::write(const char *data, size_t len)
{
    if (!m_isOpen || m_fd == -1)
        return -1;

    ssize_t n = ::write(m_fd, data, len);
    if (n < 0)
        updateError(Error::WriteError);

    if (m_localEcho) {
        std::scoped_lock lock(m_cbMutex);
        if (m_readCallback)
            m_readCallback(data, n > 0 ? n : 0); // Echo back what was written
    }

    return n;
}

int SerialIO::fd() const
{
    return m_fd;
}

bool SerialIO::isOpen() const
{
    return m_isOpen;
}

void SerialIO::setNonBlocking(bool enabled)
{
    int flags = fcntl(m_fd, F_GETFL, 0);
    if (flags == -1)
        return;
    fcntl(m_fd, F_SETFL, enabled ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK));
}

void SerialIO::setReadCallback(ReadCallback cb)
{
    std::lock_guard<std::mutex> lock(m_cbMutex);
    m_readCallback = std::move(cb);
}

bool SerialIO::write(const std::string &data)
{
    ssize_t written = write(data.c_str(), data.size());
    return written == static_cast<ssize_t>(data.size());
}

bool SerialIO::writeRawFile(const std::filesystem::path &filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        updateError(Error::OpenError);
        return false;
    }

    constexpr size_t chunkSize = 4096;
    std::vector<char> buffer(chunkSize);
    size_t totalSent = 0;
    file.seekg(0, std::ios::end);
    size_t totalSize = file.tellg();
    file.seekg(0);

    m_isBusy = true;
    while (file.read(buffer.data(), buffer.size()) || file.gcount() > 0) {
        ssize_t bytesWritten = write(buffer.data(), file.gcount());
        if (bytesWritten <= 0) {
            updateError(Error::WriteError);
            m_isBusy = false;
            return false;
        }
        totalSent += bytesWritten;
        setUploadPercent((100 * totalSent) / totalSize);
    }
    m_isBusy = false;
    setUploadPercent(0);
    return totalSent == totalSize;
}

void SerialIO::onEvents(uint32_t events)
{
    if (events & (POLLERR | POLLNVAL)) {
        updateError(Error::ReadError);
        closeDevice(); // Fatal error
        return;
    }

    if (events & POLLHUP) {
        closeDevice();
        return;
    }

    if (events & POLLIN) {
        char buf[1024];
        while (true) {
            ssize_t n = ::read(m_fd, buf, sizeof(buf));

            if (n > 0) {
                std::lock_guard<std::mutex> lock(m_cbMutex);

                if (m_isRecording && m_logStream.is_open())
                    m_logStream.write(buf, n);

                if (m_readCallback)
                    m_readCallback(buf, n);
            } else if (n == 0) {
                closeDevice();
                break; // Exit loop
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // Exit loop
            } else if (errno != EINTR) {
                updateError(Error::ReadError);
                closeDevice();
                break; // Exit loop
            }
        }
    }
}

SerialIO::State SerialIO::state() const
{
    return m_state.load();
}
SerialIO::Error SerialIO::error() const
{
    return m_error.load();
}
const std::string &SerialIO::errorString() const
{
    return m_errorString;
}

void SerialIO::setLocalEcho(bool val)
{
    m_localEcho = val;
}

bool SerialIO::localEcho() const
{
    return m_localEcho;
}

void SerialIO::setPortName(const std::string &val)
{
    m_portName = val;
}

const std::string &SerialIO::portName() const
{
    return m_portName;
}


void SerialIO::setIsRecording(bool val)
{
    m_isRecording = val;
}

bool SerialIO::isRecording() const
{
    return m_isRecording;
}

void SerialIO::setLogFile(const std::filesystem::path &val)
{
    m_logFile = val;
}

const std::filesystem::path &SerialIO::logFile() const
{
    return m_logFile;
}

void SerialIO::setCustomDevice(bool val)
{
    m_customDevice = val;
}

bool SerialIO::customDevice() const
{
    return m_customDevice;
}

void SerialIO::setIsBusy(bool val)
{
    m_isBusy = val;
}

bool SerialIO::isBusy() const
{
    return m_isBusy;
}

void SerialIO::setDeviceStatus(const std::string &val)
{
    m_deviceStatus = val;
}

const std::string &SerialIO::deviceStatus() const
{
    return m_deviceStatus;
}

void SerialIO::setDescription(const std::string &val)
{
    m_description = val;
}

const std::string &SerialIO::description() const
{
    return m_description;
}

void SerialIO::setManufacturer(const std::string &val)
{
    m_manufacturer = val;
}

const std::string &SerialIO::manufacturer() const
{
    return m_manufacturer;
}

void SerialIO::setSerialNumber(const std::string &val)
{
    m_serialNumber = val;
}

const std::string &SerialIO::serialNumber() const
{
    return m_serialNumber;
}

void SerialIO::setLocation(const std::string &val)
{
    m_location = val;
}

const std::string &SerialIO::location() const
{
    return m_location;
}

void SerialIO::setVendorId(uint16_t val)
{
    m_vendorId = val;
}

uint16_t SerialIO::vendorId() const
{
    return m_vendorId;
}

void SerialIO::setProductId(uint16_t val)
{
    m_productId = val;
}

uint16_t SerialIO::productId() const
{
    return m_productId;
}

void SerialIO::setUploadPercent(int val)
{
    m_uploadPercent = val;
}


int SerialIO::uploadPercent() const
{
    return m_uploadPercent;
}

bool SerialIO::setRecording(bool enabled, const std::filesystem::path &filePath)
{
    std::lock_guard lock(m_cbMutex); // Protects logStream
    if (enabled) {
        m_logFile = filePath;
        m_logStream.open(m_logFile, std::ios::binary | std::ios::trunc);
        m_isRecording = m_logStream.is_open();
        return m_isRecording;
    } else {
        stopRecordingInternal();
        return true;
    }
}

void SerialIO::updateState(State newState)
{
    m_state.store(newState);
}

void SerialIO::updateError(Error newError)
{
    m_error.store(newError);
    m_errorString = std::string(errorToString(newError));
    if (newError != Error::None) {
        JOB_LOG_ERROR("[SerialIO] Error set: {}", m_errorString);
    }
}

std::string_view SerialIO::errorToString(Error err)
{
    switch (err) {
    case Error::None: return "No Error";
    case Error::OpenError: return "Failed to open device";
    case Error::ReadError: return "Failed to read from device";
    case Error::WriteError: return "Failed to write to device";
    case Error::PermissionError: return "Permission denied";
    case Error::NotFound: return "Device not found";
    case Error::Timeout: return "Operation timed out";
    case Error::LoopError: return "Event loop error";
    case Error::Unknown: return "Unknown error";
    default: return "Unknown error";
    }
}

bool SerialIO::startRecording(const std::filesystem::path &filePath)
{
    std::lock_guard lock(m_cbMutex); // Protects logStream
    m_logFile = filePath;
    m_logStream.open(m_logFile, std::ios::binary | std::ios::trunc);
    m_isRecording = m_logStream.is_open();
    return m_isRecording;
}

void SerialIO::stopRecordingInternal()
{
    // Assumes m_cbMutex is already held or not needed
    if (m_logStream.is_open()) {
        m_logStream.flush();
        m_logStream.close();
    }
    m_isRecording = false;
}

} // job::uart
// CHECKPOINT: v2.0
