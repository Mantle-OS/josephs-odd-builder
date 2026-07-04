#pragma once
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

class TransientTestFile {
public:
    TransientTestFile(const std::string &path, std::size_t size, char pattern) :
        m_path(path)
    {
        std::ofstream stream(m_path, std::ios::binary);
        if (size > 0) {
            std::vector<char> buffer(size, pattern);
            stream.write(buffer.data(), size);
        }
    }

    ~TransientTestFile()
    {
        std::remove(m_path.c_str());
    }

    [[nodiscard]] std::string path() const noexcept { return m_path; }

private:
    std::string m_path;
};

