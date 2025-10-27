#pragma once

#include <fstream>
#include <mutex>
#include <string>
#include <iostream>

#include <io_base.h>

// #include <filesystem> // todo use filesystem later on

namespace job::io {

enum class FileMode {
    RegularFile,
    StdIn,
    StdOut,
    StdErr
};

class FileIO : public core::IODevice
{
public:
    explicit FileIO(const std::string &path,
           FileMode mode = FileMode::RegularFile,
           bool writeMode = false);
    ~FileIO();

    [[nodiscard]] bool openDevice() override;
    void closeDevice() override;
    [[nodiscard]] bool flush();


    ssize_t read(char *buffer, size_t size) override;
    ssize_t write(const char *data, size_t size) override;

    [[nodiscard]] bool isOpen() const override;

    [[nodiscard]] int fd() const override;

    void setNonBlocking(bool enabled) override;
    void setReadCallback(ReadCallback cb) override;

    std::string path() const;

private:
    std::string m_filePath;
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
