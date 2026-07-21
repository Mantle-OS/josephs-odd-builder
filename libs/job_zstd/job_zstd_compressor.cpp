#include "job_zstd_compressor.h"

#include <filesystem>
#include <fstream>
#include <vector>
#include <limits>
#include <algorithm>

#include "job_zstd_io.h"
#include "job_zstd_entry.h"
#include "job_zstd_wire.h"

namespace job::zstd {

bool JobZstdCompressor::execute()
{
    if (input().empty() || output().empty()) {
        setErrorString("Input or output pathways are unconfigured.");
        return false;
    }

    std::error_code existsEc;
    bool const inputExists = std::filesystem::exists(input(), existsEc);

    if (!inputExists) {
        setErrorString("Source input layout path does not exist.");
        return false;
    }

    std::filesystem::path const dstInfo   = output();
    std::filesystem::path const parentDir = dstInfo.parent_path();

    if (!parentDir.empty()) {
        std::error_code parentExistsEc;
        bool const parentExists = std::filesystem::exists(parentDir, parentExistsEc);

        if (!parentExists) {
            std::error_code mkEc;
            if (!std::filesystem::create_directories(parentDir, mkEc)) {
                setErrorString("Failed to generate directory tree layout for destination container.");
                return false;
            }
        }
    }

    std::error_code isDirEc;
    bool const inputIsDir = std::filesystem::is_directory(input(), isDirEc);

    if (isDirEc) {
        setErrorString("Failed to determine whether input path is a directory: " + input());
        return false;
    }

    if (inputIsDir)
        return compressFolder();

    std::error_code regEc;
    bool const isRegular = std::filesystem::is_regular_file(input(), regEc);

    if (regEc || !isRegular) {
        setErrorString("Refusing to compress a non-regular file (FIFO, device, socket, etc.): " + input());
        return false;
    }

    return compressFile();
}

bool JobZstdCompressor::compressFolder()
{
    std::ofstream dst(output(), std::ios::binary | std::ios::trunc);
    if (!dst) {
        setErrorString("Failed to open destination archive for writing: " + output());
        return false;
    }

    job::zstd::JobZstdIO zstd(dst.rdbuf());
    if (!zstd.setCompressionLevel(compressionLevel())) {
        setErrorString("Compression level rejected: Was the stream already open?");
        return false;
    }

    if (!zstd.open(job::zstd::JobZstdIO::Mode::WriteOnly)) {
        setErrorString(zstd.errorString());
        return false;
    }

    std::vector<job::zstd::JobPendingEntry> entries;
    std::string walkError;
    std::filesystem::path const root(input());

    if (!job::zstd::collectEntries(root, root, preserveEmptyDirectories(), preserveSymlinks(), entries, walkError)) {
        setErrorString(walkError);
        static_cast<void>(zstd.close());
        return false;
    }

    if (!recursiveDirectories()) {
        if (!job::zstd::flattenEntries(entries, walkError)) {
            setErrorString(walkError);
            static_cast<void>(zstd.close());
            return false;
        }
    }

    std::uint64_t totalBytes = 0;
    for (const auto &e : entries) {
        if (e.kind != job::zstd::JobZstdEntryKind::File)
            continue;

        std::error_code sizeEc;
        totalBytes += std::filesystem::file_size(e.sourcePath, sizeEc);
    }

    setTotal(static_cast<int>(std::min<std::uint64_t>(totalBytes, static_cast<std::uint64_t>(std::numeric_limits<int>::max()))));
    setCurrent(0);

    std::ostream zstdOut(&zstd);

    job::zstd::utils::writeString(zstdOut, JobZstdOptions::magicDirString());
    job::zstd::utils::writeU64(zstdOut, static_cast<std::uint64_t>(entries.size()));

    char buffer[65536];
    std::uint64_t done = 0;

    for (const auto &e : entries) {
        job::zstd::utils::writeString(zstdOut, job::zstd::entryMagicString(e.kind));
        job::zstd::utils::writeString(zstdOut, e.relativePath.generic_string());

        if (!zstdOut) {
            setErrorString("Stream serialization write interruption occurred while writing entry header.");
            static_cast<void>(zstd.close());
            return false;
        }

        switch (e.kind) {
        case job::zstd::JobZstdEntryKind::Directory:
        case job::zstd::JobZstdEntryKind::EmptyDirectory:
            break;

        case job::zstd::JobZstdEntryKind::Symlink:
            job::zstd::utils::writeString(zstdOut, e.symlinkTarget);
            break;

        case job::zstd::JobZstdEntryKind::File: {
            std::error_code sizeEc;
            std::uint64_t const fileSize = std::filesystem::file_size(e.sourcePath, sizeEc);
            if (sizeEc) {
                setErrorString("Failed to stat file during archival: " + e.sourcePath.string());
                static_cast<void>(zstd.close());
                return false;
            }

            job::zstd::utils::writeU64(zstdOut, fileSize);

            std::ifstream src(e.sourcePath, std::ios::binary);
            if (!src) {
                setErrorString("Failed to open source file for reading: " + e.sourcePath.string());
                static_cast<void>(zstd.close());
                return false;
            }

            std::uint64_t remaining = fileSize;
            while (remaining > 0) {
                std::streamsize const toRead = static_cast<std::streamsize>(std::min<std::uint64_t>(remaining, sizeof(buffer)));

                src.read(buffer, toRead);
                std::streamsize const got = src.gcount();

                if (got <= 0) {
                    setErrorString("Source read error or unexpected EOF: " + e.sourcePath.string());
                    static_cast<void>(zstd.close());
                    return false;
                }

                zstdOut.write(buffer, got);
                if (!zstdOut) {
                    setErrorString("Stream serialization write interruption occurred.");
                    static_cast<void>(zstd.close());
                    return false;
                }

                remaining -= static_cast<std::uint64_t>(got);
                done      += static_cast<std::uint64_t>(got);
                setCurrent(static_cast<int>(std::min<std::uint64_t>(done, static_cast<std::uint64_t>(std::numeric_limits<int>::max()))));
            }
            break;
        }
        }
    }

    if (!zstd.close()) {
        if (errorString().empty())
            setErrorString(zstd.errorString());
        return false;
    }

    notifyFinished();
    return true;
}

bool JobZstdCompressor::compressFile()
{
    std::ifstream src(input(), std::ios::binary);
    if (!src) {
        setErrorString("Failed to open source file for reading: " + input());
        return false;
    }

    std::ofstream dst(output(), std::ios::binary | std::ios::trunc);
    if (!dst) {
        setErrorString("Failed to open destination file for writing: " + output());
        return false;
    }

    job::zstd::JobZstdIO zstd(dst.rdbuf());
    if (!zstd.setCompressionLevel(compressionLevel())) {
        setErrorString("Compression level rejected -- was the stream already open?");
        return false;
    }

    if (!zstd.open(job::zstd::JobZstdIO::Mode::WriteOnly)) {
        setErrorString(zstd.errorString());
        return false;
    }

    std::ostream zstdOut(&zstd);

    job::zstd::utils::writeString(zstdOut, JobZstdOptions::magicFileString());

    std::error_code sizeEc;
    std::uint64_t const srcSize = std::filesystem::file_size(input(), sizeEc);
    setTotal(static_cast<int>(std::min<std::uint64_t>(srcSize, static_cast<std::uint64_t>(std::numeric_limits<int>::max()))));
    setCurrent(0);

    char buffer[65536];
    std::uint64_t done = 0;

    while (src) {
        src.read(buffer, sizeof(buffer));
        std::streamsize const got = src.gcount();
        if (got <= 0)
            break;

        zstdOut.write(buffer, got);
        if (!zstdOut) {
            setErrorString(zstd.errorString());
            static_cast<void>(zstd.close());
            return false;
        }

        done += static_cast<std::uint64_t>(got);
        setCurrent(static_cast<int>(std::min<std::uint64_t>(done, static_cast<std::uint64_t>(std::numeric_limits<int>::max()))));
    }

    if (!zstd.close()) {
        setErrorString(zstd.errorString());
        return false;
    }

    notifyFinished();
    return true;
}

} // namespace job::zstd