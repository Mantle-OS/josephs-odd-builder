#pragma once

#include <concepts>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include <io_base.h>

namespace job::io {

class MockIO : public IODevice
{
public:
    using Ptr  = std::shared_ptr<MockIO>;
    using WPtr = std::weak_ptr<MockIO>;
    using UPtr = std::unique_ptr<MockIO>;

    MockIO();
    ~MockIO() override = default;

    MockIO(const MockIO &) = delete;
    MockIO &operator=(const MockIO &) = delete;
    MockIO(MockIO &&) = delete;
    MockIO &operator=(MockIO &&) = delete;

    template <typename... Args>
        requires std::constructible_from<MockIO, Args...>
    [[nodiscard]] static Ptr createShared(Args &&...args)
    {
        return std::make_shared<MockIO>(std::forward<Args>(args)...);
    }

    template <typename... Args>
        requires std::constructible_from<MockIO, Args...>
    [[nodiscard]] static UPtr createUniq(Args &&...args)
    {
        return std::make_unique<MockIO>(std::forward<Args>(args)...);
    }

    void setReadBuffer(const std::string &data);
    void appendReadBuffer(const std::string &data);
    void clearReadBuffer();

    [[nodiscard]] std::string takeWriteBuffer();
    [[nodiscard]] std::string peekWriteBuffer() const;

    [[nodiscard]] bool openDevice() override;
    void closeDevice() override;
    [[nodiscard]] bool isOpen() const override;

    [[nodiscard]] ssize_t read(char *buffer, std::size_t maxlen) override;
    [[nodiscard]] ssize_t write(const char *data, std::size_t len) override;

    [[nodiscard]] int fd() const override
    {
        return -1;
    }

    void setNonBlocking(bool enabled) override;
    void setReadCallback(ReadCallback cb) override;

    void emitRead(const std::string &data);

    [[nodiscard]] bool readable() const;
    [[nodiscard]] bool writable() const;
    [[nodiscard]] bool nonBlocking() const;

private:
    bool m_isOpen{false};
    bool m_nonBlocking{false};

    std::deque<char> m_readBuffer;
    std::string m_writeBuffer;

    mutable std::mutex m_mutex;
    ReadCallback m_readCallback;
};

} // namespace job::io