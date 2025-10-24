#pragma once

#include "io_base.h"

#include <string>
#include <deque>
#include <mutex>

namespace job::io {

class MockIO : public IODevice {
public:
    MockIO();
    ~MockIO() override = default;

    void setReadBuffer(const std::string &data);
    void appendReadBuffer(const std::string &data);
    void clearReadBuffer();

    std::string takeWriteBuffer();
    std::string peekWriteBuffer() const;

    bool openDevice() override;
    void closeDevice() override;
    bool isOpen() const override;

    ssize_t read(char *buffer, size_t maxlen) override;
    ssize_t write(const char *data, size_t len) override;

    int fd() const override { return -1; }

    void setNonBlocking(bool) override {}
    void setReadCallback(ReadCallback) override {}

    bool readable() const;
    bool writable() const;

private:
    bool m_isOpen = false;

    std::deque<char> m_readBuffer;
    std::string m_writeBuffer;

    mutable std::mutex m_mutex;
};

} // namespace job::io
