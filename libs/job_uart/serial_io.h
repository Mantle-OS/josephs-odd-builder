#pragma once

#include <io_base.h>
#include "serial_settings.h"

#include <fcntl.h>

#include <memory>
#include <filesystem>
#include <fstream>
#include <string>
#include <atomic>
#include <mutex>

#include <job_io_async_thread.h>
#include <job_logger.h>

namespace job::uart {

class SerialIO : public job::core::IODevice, public SerialSettings
{
public:
    enum class Error {
        None = 0,
        OpenError,
        ReadError,
        WriteError,
        PermissionError,
        NotFound,
        Timeout,
        LoopError,
        Unknown
    };

    enum class State {
        Disconnected = 0,
        Connected,
        Busy
    };

    explicit SerialIO(std::shared_ptr<threads::JobIoAsyncThread> loop);
    ~SerialIO();

    bool openDevice() override;
    void closeDevice() override;
    [[nodiscard]] ssize_t read(char *buffer, size_t maxlen) override;
    [[nodiscard]] ssize_t write(const char *data, size_t len) override;
    [[nodiscard]] int fd() const override;
    [[nodiscard]] bool isOpen() const override;
    void setNonBlocking(bool enabled) override;
    void setReadCallback(ReadCallback cb) override;

    [[nodiscard]] bool write(const std::string &data);
    [[nodiscard]] bool writeRawFile(const std::filesystem::path &filePath);

    [[nodiscard]] State state() const;
    [[nodiscard]] Error error() const;
    [[nodiscard]] const std::string &errorString() const;

    void setLocalEcho(bool val);
    [[nodiscard]] bool localEcho() const;

    void setPortName(const std::string &val);
    [[nodiscard]] const std::string &portName() const;

    void setIsRecording(bool val);
    [[nodiscard]] bool isRecording() const;

    void setLogFile(const std::filesystem::path &val);
    [[nodiscard]] const std::filesystem::path &logFile() const;

    void setCustomDevice(bool val);
    [[nodiscard]] bool customDevice() const;

    void setIsBusy(bool val);
    [[nodiscard]] bool isBusy() const;

    void setDeviceStatus(const std::string &val);
    [[nodiscard]] const std::string &deviceStatus() const;

    void setDescription(const std::string &val);
    [[nodiscard]] const std::string &description() const;

    void setManufacturer(const std::string &val);
    [[nodiscard]] const std::string &manufacturer() const;

    void setSerialNumber(const std::string &val);
    [[nodiscard]] const std::string &serialNumber() const;

    void setLocation(const std::string &val);
    [[nodiscard]] const std::string &location() const;

    void setVendorId(uint16_t val);
    [[nodiscard]] uint16_t vendorId() const;

    void setProductId(uint16_t val);
    [[nodiscard]] uint16_t productId() const;

    void setUploadPercent(int val);
    [[nodiscard]] int uploadPercent() const;

    [[nodiscard]] bool setRecording(bool enabled, const std::filesystem::path &filePath);

private:
    void onEvents(uint32_t events);

    bool startRecording(const std::filesystem::path &filePath);
    void stopRecordingInternal();

    void updateState(State newState);
    void updateError(Error newError);
    static std::string_view errorToString(Error err); // <-- Made static

    std::atomic<bool> m_isOpen{false};
    std::atomic<bool> m_isRecording{false};
    std::atomic<bool> m_isBusy{false};

    std::atomic<State> m_state{State::Disconnected};
    std::atomic<Error> m_error{Error::None};
    std::string m_errorString{"No Error"};

    int m_fd{-1};

    std::shared_ptr<threads::JobIoAsyncThread> m_loop;
    mutable std::mutex m_cbMutex;

    std::ofstream m_logStream;
    std::filesystem::path m_logFile;

    bool m_localEcho{false};
    std::string m_portName{"Unknown"};
    bool m_customDevice{false};
    std::string m_deviceStatus{"Unknown"};

    std::string m_description{"Unknown"};
    std::string m_manufacturer{"Unknown"};
    std::string m_serialNumber{"Unknown"};
    std::string m_location{"Unknown"};

    uint16_t m_vendorId{0};
    uint16_t m_productId{0};
    int m_uploadPercent{0};

    ReadCallback  m_readCallback;
};

} // job::uart
// CHECKPOINT: v2.0
