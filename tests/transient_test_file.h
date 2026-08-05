#pragma once

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

class TransientTestFile {
public:
    explicit TransientTestFile(std::string path) :
        m_path(std::move(path))
    {
        std::ofstream stream(m_path, std::ios::binary);
    }

    TransientTestFile(std::string path, std::size_t size, char pattern) :
        m_path(std::move(path))
    {
        std::ofstream stream(m_path, std::ios::binary);

        if (size > 0) {
            std::vector<char> buffer(size, pattern);
            stream.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        }
    }
    TransientTestFile(std::string path, const std::vector<std::byte> &data) :
        m_path(std::move(path))
    {
        std::ofstream stream(m_path, std::ios::binary);

        if (!stream) {
            throw std::runtime_error{
                "Failed to create transient test file"
            };
        }

        if (!data.empty()) {
            stream.write(
                reinterpret_cast<const char *>(data.data()),
                static_cast<std::streamsize>(data.size())
                );
        }
    }

    ~TransientTestFile()
    {
        std::remove(m_path.c_str());
    }

    TransientTestFile(const TransientTestFile &) = delete;
    TransientTestFile &operator=(const TransientTestFile &) = delete;
    TransientTestFile(TransientTestFile &&) = delete;
    TransientTestFile &operator=(TransientTestFile &&) = delete;

    [[nodiscard]] const std::string &path() const noexcept
    {
        return m_path;
    }

private:
    std::string m_path;
};