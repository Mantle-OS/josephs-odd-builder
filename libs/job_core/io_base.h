#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <unistd.h>

namespace job::core {

class IODevice {

public:
    using ReadCallback = std::function<void(const char *, size_t)>;

    virtual ~IODevice() = default;

    [[nodiscard]] virtual bool openDevice() = 0;
    virtual void closeDevice() = 0;

    // FIXME add a new read and write that is apart of our schema that we design
    [[nodiscard]] virtual ssize_t read(char *buffer, size_t maxlen) = 0;
    [[nodiscard]] virtual ssize_t write(const char *data, size_t len) = 0;

    ssize_t write(const std::string &data)
    {
        return write(data.data(), data.size());
    }

    [[nodiscard]] virtual bool isOpen() const = 0;
    [[nodiscard]] virtual int fd() const = 0;

    virtual void setNonBlocking(bool enabled) = 0;

    // Async callback registration
    virtual void setReadCallback(ReadCallback cb) = 0;

protected:
    [[nodiscard]] ssize_t readToString(std::string &output, size_t maxlen)
    {
        output.resize(maxlen);
        ssize_t bytesRead = read(output.data(), maxlen);
        if (bytesRead < 0) {
            output.clear();
            return -1;
        }
        output.resize(bytesRead);
        return bytesRead;
    }

    [[nodiscard]] ssize_t readToString(std::vector<uint8_t> &output, size_t maxlen)
    {
        output.resize(maxlen);
        ssize_t bytesRead = read(reinterpret_cast<char*>(output.data()), maxlen);
        if (bytesRead < 0) {
            output.clear();
            return -1;
        }
        output.resize(bytesRead);
        return bytesRead;
    }

};

} // namespace job::core
// CHECKPOINT: v1
