#include "job_zstd_decompressor_crypto.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>

#include <sodium/crypto_secretbox.h>

#include "job_zstd_wire.h"
#include "job_zstd_entry.h"
namespace job::zstd {

const job::crypto::JobSecureMem &JobZstdDecompressorCrypto::decryptionKey() const noexcept
{
    return m_decryptionKey;
}

void JobZstdDecompressorCrypto::setDecryptionKey(const job::crypto::JobSecureMem &key)
{
    m_decryptionKey = key;
}

bool JobZstdDecompressorCrypto::hasKeys() const noexcept
{
    return m_decryptionKey.size() == crypto_secretbox_KEYBYTES;
}

bool JobZstdDecompressorCrypto::execute()
{
    if (!hasKeys()) {
        setErrorString("Cryptographic pipeline missing a valid decryption key -- call setDecryptionKey() first.");
        return false;
    }

    if (input().empty() || output().empty()) {
        setErrorString("Input or output pathways are completely unconfigured.");
        return false;
    }

    std::error_code existsEc;
    bool const inputExists = std::filesystem::exists(input(), existsEc);

    std::error_code isDirEc;
    bool const inputIsDir = std::filesystem::is_directory(input(), isDirEc);

    if (!inputExists || inputIsDir) {
        setErrorString("Source input archive file does not exist or is a directory path.");
        return false;
    }

    std::string tag;
    {
        std::ifstream probe(input(), std::ios::binary);
        if (!probe) {
            setErrorString("Failed to open source archive for reading: " + input());
            return false;
        }

        JobZstdDecryptingTransport decTransport(probe.rdbuf(), decryptionKey());
        JobZstdIO zstdProbe(&decTransport);

        if (!zstdProbe.open(JobZstdIO::Mode::ReadOnly)) {
            setErrorString(zstdProbe.errorString());
            return false;
        }

        std::istream zstdIn(&zstdProbe);
        bool const gotMagic = job::zstd::utils::readString(zstdIn, tag);
        static_cast<void>(zstdProbe.close());

        if (!gotMagic) {
            setErrorString(JobZstdDecompressorCrypto::bestErrorMessage(zstdProbe, decTransport, "Archive is empty or missing its header tag."));
            return false;
        }
    }

    auto const kindOpt = entryKindFromMagicString(tag);
    if (!kindOpt) {
        setErrorString("Unrecognized archive header tag: " + tag);
        return false;
    }

    switch (*kindOpt) {
    case JobZstdEntryKind::File:
        return decompressFile();
    case JobZstdEntryKind::Directory:
        return decompressFolder();
    case JobZstdEntryKind::EmptyDirectory:
        return decompressEmptyDirectoryArchive();
    case JobZstdEntryKind::Symlink:
        return decompressSymlinkArchive();
    }

    setErrorString("Unreachable archive header state.");
    return false;
}

// Well this was even more fun lol .....
bool JobZstdDecompressorCrypto::decompressFolder()
{
    if (!hasKeys()) {
        setErrorString("Cryptographic pipeline missing a valid decryption key.");
        return false;
    }

    std::ifstream src(input(), std::ios::binary);
    if (!src) {
        setErrorString("Failed to open source archive for reading: " + input());
        return false;
    }

    JobZstdDecryptingTransport decTransport(src.rdbuf(), decryptionKey());
    JobZstdIO zstd(&decTransport);

    if (!zstd.open(JobZstdIO::Mode::ReadOnly)) {
        setErrorString(zstd.errorString());
        return false;
    }

    std::istream zstdIn(&zstd);

    std::string tag;
    if (!job::zstd::utils::readString(zstdIn, tag) || tag != JobZstdOptions::magicDirString()) {
        setErrorString(JobZstdDecompressorCrypto::bestErrorMessage(zstd, decTransport, "Archive header did not match the expected directory format tag."));
        static_cast<void>(zstd.close());
        return false;
    }

    std::uint64_t totalEntries = 0;
    if (!job::zstd::utils::readU64(zstdIn, totalEntries)) {
        setErrorString(JobZstdDecompressorCrypto::bestErrorMessage(zstd, decTransport, "Archive header is truncated -- missing entry count."));
        static_cast<void>(zstd.close());
        return false;
    }

    std::filesystem::path const baseDir(output());
    std::error_code baseExistsEc;
    bool const baseExists = std::filesystem::exists(baseDir, baseExistsEc);

    if (!baseExists) {
        std::error_code mkBaseEc;
        if (!std::filesystem::create_directories(baseDir, mkBaseEc)) {
            setErrorString("Failed to allocate extraction target folder destination: " + output());
            static_cast<void>(zstd.close());
            return false;
        }
    }

    setCurrent(0);
    setTotal(0);

    char buffer[65536];
    std::uint64_t bytesWrittenSoFar = 0;

    for (std::uint64_t i = 0; i < totalEntries; ++i) {
        std::string entryTag;
        std::string relPathStr;

        if (!job::zstd::utils::readString(zstdIn, entryTag) || !job::zstd::utils::readString(zstdIn, relPathStr)) {
            setErrorString(JobZstdDecompressorCrypto::bestErrorMessage(zstd, decTransport, "Archive entry stream ended unexpectedly while reading entry header."));
            static_cast<void>(zstd.close());
            return false;
        }

        auto const kindOpt = entryKindFromMagicString(entryTag);
        if (!kindOpt) {
            setErrorString("Archive entry has an unrecognized type tag: " + entryTag);
            static_cast<void>(zstd.close());
            return false;
        }

        auto const safePath = safeJoin(baseDir, std::filesystem::path(relPathStr));
        if (!safePath) {
            setErrorString("Archive entry path escapes the extraction directory -- refusing to extract: " + relPathStr);
            static_cast<void>(zstd.close());
            return false;
        }

        std::string symlinkCheckError;
        if (!verifyNoSymlinkComponents(*safePath, symlinkCheckError)) {
            setErrorString(symlinkCheckError);
            static_cast<void>(zstd.close());
            return false;
        }

        switch (*kindOpt) {
        case JobZstdEntryKind::Directory:
        case JobZstdEntryKind::EmptyDirectory: {
            std::error_code dirExistsEc;
            bool const dirExists = std::filesystem::exists(*safePath, dirExistsEc);

            if (!dirExists) {
                std::error_code mkEc;
                if (!std::filesystem::create_directories(*safePath, mkEc)) {
                    setErrorString("Failed to recreate directory entry: " + relPathStr);
                    static_cast<void>(zstd.close());
                    return false;
                }
            }
            break;
        }

        case JobZstdEntryKind::Symlink: {
            std::string targetStr;
            if (!job::zstd::utils::readString(zstdIn, targetStr)) {
                setErrorString(JobZstdDecompressorCrypto::bestErrorMessage(zstd, decTransport, "Archive entry stream ended unexpectedly while reading symlink target."));
                static_cast<void>(zstd.close());
                return false;
            }

            if (!preserveSymlinks()) {
                setErrorString("Archive contains a symlink entry and preserveSymlinks() is false -- refusing extraction: " + relPathStr);
                static_cast<void>(zstd.close());
                return false;
            }

            std::filesystem::path const parent = safePath->parent_path();

            if (!parent.empty()) {
                std::error_code parentExistsEc;
                bool const parentExists = std::filesystem::exists(parent, parentExistsEc);

                if (!parentExists) {
                    std::error_code mkParentEc;
                    if (!std::filesystem::create_directories(parent, mkParentEc)) {
                        setErrorString("Failed to compose parent directory for symlink entry: " + relPathStr);
                        static_cast<void>(zstd.close());
                        return false;
                    }
                }
            }

            std::error_code linkEc;
            std::filesystem::create_symlink(targetStr, *safePath, linkEc);
            if (linkEc) {
                setErrorString("Failed to recreate symlink entry: " + relPathStr + " -> " + targetStr);
                static_cast<void>(zstd.close());
                return false;
            }
            break;
        }

        case JobZstdEntryKind::File: {
            std::uint64_t fileLength = 0;
            if (!job::zstd::utils::readU64(zstdIn, fileLength)) {
                setErrorString(JobZstdDecompressorCrypto::bestErrorMessage(zstd, decTransport, "Archive entry stream ended unexpectedly while reading file length."));
                static_cast<void>(zstd.close());
                return false;
            }

            std::filesystem::path const parent = safePath->parent_path();

            if (!parent.empty()) {
                std::error_code parentExistsEc;
                bool const parentExists = std::filesystem::exists(parent, parentExistsEc);

                if (!parentExists) {
                    std::error_code mkParentEc;
                    if (!std::filesystem::create_directories(parent, mkParentEc)) {
                        setErrorString("Failed to compose parent directory for file entry: " + relPathStr);
                        static_cast<void>(zstd.close());
                        return false;
                    }
                }
            }

            std::ofstream targetFile(*safePath, std::ios::binary | std::ios::trunc);
            if (!targetFile) {
                setErrorString("Failed to open extraction target for writing: " + safePath->string());
                static_cast<void>(zstd.close());
                return false;
            }

            std::uint64_t remaining = fileLength;
            while (remaining > 0) {
                std::streamsize const toRead = static_cast<std::streamsize>(std::min<std::uint64_t>(remaining, sizeof(buffer)));

                zstdIn.read(buffer, toRead);
                std::streamsize const got = zstdIn.gcount();

                if (got <= 0) {
                    setErrorString(JobZstdDecompressorCrypto::bestErrorMessage(zstd, decTransport, "Archive format payload stream broken or unexpected EOF reached: " + relPathStr));
                    static_cast<void>(zstd.close());
                    return false;
                }

                targetFile.write(buffer, got);
                if (!targetFile) {
                    setErrorString("Extraction write failure: " + safePath->string());
                    static_cast<void>(zstd.close());
                    return false;
                }

                remaining         -= static_cast<std::uint64_t>(got);
                bytesWrittenSoFar += static_cast<std::uint64_t>(got);
                setCurrent(static_cast<int>(std::min<std::uint64_t>(bytesWrittenSoFar, static_cast<std::uint64_t>(std::numeric_limits<int>::max()))));
            }
            break;
        }
        }
    }

    if (!zstd.close()) {
        setErrorString(JobZstdDecompressorCrypto::bestErrorMessage(zstd, decTransport, zstd.errorString()));
        return false;
    }

    notifyFinished();
    return true;
}

bool JobZstdDecompressorCrypto::decompressFile()
{
    if (!hasKeys()) {
        setErrorString("Cryptographic pipeline missing a valid decryption key.");
        return false;
    }

    std::ifstream src(input(), std::ios::binary);
    if (!src) {
        setErrorString("Failed to open source archive for reading: " + input());
        return false;
    }

    JobZstdDecryptingTransport decTransport(src.rdbuf(), decryptionKey());
    JobZstdIO zstd(&decTransport);

    if (!zstd.open(JobZstdIO::Mode::ReadOnly)) {
        setErrorString(zstd.errorString());
        return false;
    }

    std::istream zstdIn(&zstd);

    std::string tag;
    if (!job::zstd::utils::readString(zstdIn, tag) || tag != JobZstdOptions::magicFileString()) {
        setErrorString(JobZstdDecompressorCrypto::bestErrorMessage(zstd, decTransport, "Archive header did not match the expected flat-file format tag."));
        static_cast<void>(zstd.close());
        return false;
    }

    std::ofstream dst(output(), std::ios::binary | std::ios::trunc);
    if (!dst) {
        setErrorString("Failed to open destination file for writing: " + output());
        static_cast<void>(zstd.close());
        return false;
    }

    std::error_code sizeEc;
    std::uint64_t const encryptedSize = std::filesystem::file_size(input(), sizeEc);
    setTotal(static_cast<int>(std::min<std::uint64_t>(encryptedSize, static_cast<std::uint64_t>(std::numeric_limits<int>::max()))));
    setCurrent(0);

    char buffer[65536];

    while (!zstd.atEnd()) {
        zstdIn.read(buffer, sizeof(buffer));
        std::streamsize const got = zstdIn.gcount();

        if (got <= 0)
            break;

        dst.write(buffer, got);
        if (!dst) {
            setErrorString("Extraction write failure: " + output());
            static_cast<void>(zstd.close());
            return false;
        }

        setCurrent(static_cast<int>(std::min<std::uint64_t>(static_cast<std::uint64_t>(src.tellg()), static_cast<std::uint64_t>(std::numeric_limits<int>::max()))));
    }

    if (zstd.wasTruncated() || zstd.hadDecodeError() || decTransport.wasTruncated() || decTransport.hadAuthenticationError()) {
        setErrorString(JobZstdDecompressorCrypto::bestErrorMessage(zstd, decTransport, zstd.errorString()));
        static_cast<void>(zstd.close());
        return false;
    }

    if (!zstd.close()) {
        setErrorString(JobZstdDecompressorCrypto::bestErrorMessage(zstd, decTransport, zstd.errorString()));
        return false;
    }

    notifyFinished();
    return true;
}

bool JobZstdDecompressorCrypto::decompressSymlinkArchive()
{
    if (!hasKeys()) {
        setErrorString("Cryptographic pipeline missing a valid decryption key.");
        return false;
    }

    std::ifstream src(input(), std::ios::binary);
    if (!src) {
        setErrorString("Failed to open source archive for reading: " + input());
        return false;
    }

    JobZstdDecryptingTransport decTransport(src.rdbuf(), decryptionKey());
    JobZstdIO zstd(&decTransport);

    if (!zstd.open(JobZstdIO::Mode::ReadOnly)) {
        setErrorString(zstd.errorString());
        return false;
    }

    std::istream zstdIn(&zstd);

    std::string tag;
    std::string targetStr;

    if (!job::zstd::utils::readString(zstdIn, tag) || tag != JobZstdOptions::magicLinkString() || !job::zstd::utils::readString(zstdIn, targetStr)) {
        setErrorString(JobZstdDecompressorCrypto::bestErrorMessage(zstd, decTransport, "Archive header did not match the expected symlink format tag."));
        static_cast<void>(zstd.close());
        return false;
    }

    if (!preserveSymlinks()) {
        setErrorString("Archive is a symlink and preserveSymlinks() is false: refusing extraction.");
        static_cast<void>(zstd.close());
        return false;
    }

    std::string symlinkCheckError;
    if (!verifyNoSymlinkComponents(output(), symlinkCheckError)) {
        setErrorString(symlinkCheckError);
        static_cast<void>(zstd.close());
        return false;
    }

    std::filesystem::path const parent = std::filesystem::path(output()).parent_path();

    if (!parent.empty()) {
        std::error_code parentExistsEc;
        bool const parentExists = std::filesystem::exists(parent, parentExistsEc);

        if (!parentExists) {
            std::error_code mkParentEc;
            if (!std::filesystem::create_directories(parent, mkParentEc)) {
                setErrorString("Failed to compose parent directory for symlink archive target: " + output());
                static_cast<void>(zstd.close());
                return false;
            }
        }
    }

    std::error_code linkEc;
    std::filesystem::create_symlink(targetStr, output(), linkEc);
    if (linkEc) {
        setErrorString("Failed to recreate symlink archive: " + output() + " -> " + targetStr);
        static_cast<void>(zstd.close());
        return false;
    }

    static_cast<void>(zstd.close());
    notifyFinished();
    return true;
}

std::string JobZstdDecompressorCrypto::bestErrorMessage(const JobZstdIO &zstd, const JobZstdDecryptingTransport &decTransport, const std::string &fallback)
{
    if (decTransport.hadAuthenticationError())
        return decTransport.errorString();

    if (decTransport.wasTruncated())
        return decTransport.errorString();

    if (!zstd.errorString().empty())
        return zstd.errorString();

    return fallback;
}

} // namespace job::zstd