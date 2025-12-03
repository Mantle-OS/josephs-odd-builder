#pragma once

#include <fstream>
#include <mutex>
#include <string>
#include <iostream>
#include <filesystem>

#include <io_base.h>
#include <job_logger.h>


namespace job::io {

enum class FileMode {
    RegularFile,
    StdIn,
    StdOut,
    StdErr
};

enum class OpenType{
    Truncate,
    Append,
    ReadOnly
};

class FileIO : public core::IODevice
{
public:
    explicit FileIO(const std::filesystem::path &path, FileMode mode = FileMode::RegularFile, bool writeMode = false);
    ~FileIO();

    [[nodiscard]] bool openDevice() override;
    void closeDevice() override;
    bool flush() override;

    ssize_t read(char *buffer, size_t size) override;
    ssize_t write(const char *data, size_t size) override;

    [[nodiscard]] bool isOpen() const override;
    [[nodiscard]] int fd() const override;

    void setNonBlocking(bool enabled) override;
    void setReadCallback(ReadCallback cb) override;

    [[nodiscard]] std::filesystem::path  path() const;
    [[nodiscard]] std::string pathString() const;

    void setPath(const std::filesystem::path &path,  OpenType openType = OpenType::ReadOnly) noexcept;
    void setPath( const std::string &path, OpenType openType = OpenType::ReadOnly) noexcept;

    [[nodiscard]] std::string readAll() noexcept;
    [[nodiscard]] bool readAll(std::vector<uint8_t>& out_buf) noexcept;

private:
    std::filesystem::path m_filePath;
    FileMode m_mode = FileMode::RegularFile;

    bool m_writeMode = false;
    bool m_open = false;

    std::ifstream m_input;
    std::ofstream m_output;

    std::ostream *m_stdOut = nullptr;
    std::istream *m_stdIn = nullptr;

    ReadCallback m_readCallback = nullptr;

    mutable std::mutex m_ioMutex;

};

} // job::io
// CHECKPOINT: v1.0
