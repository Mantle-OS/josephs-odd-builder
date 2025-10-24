#include "serial_io.h"

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>
#include <iostream>
#include <poll.h>

namespace job::io{

SerialIO::SerialIO()
{
}

SerialIO::~SerialIO()
{
    closeDevice();
}

bool SerialIO::openDevice()
{
    closeDevice();

    if (!std::filesystem::exists(m_portName)) {
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

    if (tcsetattr(m_fd, TCSANOW, &tty) != 0) {
        updateError(Error::OpenError);
        return false;
    }

    m_isOpen = true;
    updateState(State::Connected);
    return true;
}

void SerialIO::closeDevice()
{
    stopReading();

    if (m_isRecording)
        stopRecordingInternal();

    if (m_fd != -1)
        ::close(m_fd);

    m_fd = -1;
    m_isOpen = false;

    updateState(State::Disconnected);
}

ssize_t SerialIO::read(char *buffer, size_t maxlen)
{
    if (!m_isOpen || m_fd == -1)
        return -1;
    return ::read(m_fd, buffer, maxlen);
}

ssize_t SerialIO::write(const char *data, size_t len)
{
    if (!m_isOpen || m_fd == -1)
        return -1;
    return ::write(m_fd, data, len);
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
    std::lock_guard<std::mutex> lock(m_readMutex); // FIXME
    m_readCallback = std::move(cb);
}

bool SerialIO::write(const std::string &data)
{
    return write(data.c_str(), data.size()) == static_cast<ssize_t>(data.size());
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

void SerialIO::startReading()
{
    if (!m_isOpen || m_readerThread)
        return;

    job::threads::JobThreadOptions opts = job::threads::JobThreadOptions::normal();
    opts.name = "Serial Reader";

    m_readerThread = std::make_shared<job::threads::JobThread>(opts);
    m_readerThread->setRunFunction([this](std::stop_token token) { this->readerLoop(token); });

    auto ret = m_readerThread->start();
    if (ret != job::threads::JobThread::StartResult::Started)
        updateError(Error::Unknown);
}

void SerialIO::stopReading()
{
    if (!m_readerThread)
        return;

    m_readerThread->requestStop();
    m_readerThread->join();
    m_readerThread.reset();
}

void SerialIO::readerLoop(std::stop_token token)
{
    char buf[1024];
    struct pollfd pfd { m_fd, POLLIN, 0 };

    while (!token.stop_requested()) {
        int ret = ::poll(&pfd, 1, 250);
        if (ret <= 0)
            continue;

        if (pfd.revents & POLLIN) {
            ssize_t n = ::read(m_fd, buf, sizeof(buf));
            if (n > 0) {
                std::lock_guard<std::mutex> lock(m_readMutex);

                if (m_isRecording && m_logStream.is_open())
                    m_logStream.write(buf, n);

                if (m_readCallback)
                    m_readCallback(buf, n);
            } else if (n < 0 && errno != EAGAIN && errno != EINTR) {
                updateError(Error::ReadError);
                break;
            }
        }
    }
}



SerialIO::State SerialIO::state() const
{
    return m_state;
}
SerialIO::Error SerialIO::error() const
{
    return m_error;
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
    std::lock_guard lock(m_readMutex);
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
    m_state = newState;
}

void SerialIO::updateError(Error newError)
{
    m_error = newError;
    switch (newError) {
        case Error::None:
            m_errorString = "No Error";
            break;
        case Error::OpenError:
            m_errorString = "Failed to open device";
            break;
        case Error::ReadError:
            m_errorString = "Failed to read from device";
            break;
        case Error::WriteError:
            m_errorString = "Failed to write to device";
            break;
        case Error::PermissionError:
            m_errorString = "Permission denied";
            break;
        case Error::NotFound:
            m_errorString = "Device not found";
            break;
        case Error::Timeout:
            m_errorString = "Operation timed out";
            break;
        case Error::Unknown:
            m_errorString = "Unknown error";
            break;
    }
}

bool SerialIO::startRecording(const std::filesystem::path &filePath)
{
    m_logFile = filePath;
    m_logStream.open(m_logFile, std::ios::binary | std::ios::trunc);
    m_isRecording = m_logStream.is_open();
    return m_isRecording;
}

void SerialIO::stopRecordingInternal()
{
    if (m_logStream.is_open()) {
        m_logStream.flush();
        m_logStream.close();
    }
    m_isRecording = false;
}

}
