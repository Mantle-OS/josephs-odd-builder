#include "job_gguf_writer.h"

#include <cstring>
#include <stdexcept>
#include <utility>
#include <filesystem>

#include <gguf.h>

namespace job::ggml {

namespace {

[[nodiscard]] std::string pathToUtf8(const std::filesystem::path &filePath)
{
#if defined(__cpp_lib_char8_t)
    const std::u8string utf8Path = filePath.u8string();
    return {
        reinterpret_cast<const char *>(utf8Path.data()),
        utf8Path.size()
    };
#else
    return filePath.u8string();
#endif
}

} // namespace

JobGgufWriter::JobGgufWriter(JobGgufContext *context) :
    m_context{context}
{
    if (!m_context) {
        throw std::invalid_argument{
            "JobGgufWriter requires a valid JobGgufContext"
        };
    }
}

bool JobGgufWriter::isValid() const noexcept
{
    return m_context && m_context->isValid();
}

bool JobGgufWriter::write(const std::filesystem::path &filePath, bool metadataOnly)
{
    clearError();

    if (!isValid()) {
        setError("JobGgufWriter does not have a valid source context");

        return false;
    }

    if (filePath.empty()) {
        setError("JobGgufWriter requires a non-empty output path");

        return false;
    }

    const std::string nativePath = pathToUtf8(filePath);

    if (nativePath.empty()) {
        setError("Failed to convert the GGUF output path");

        return false;
    }

    const bool success = gguf_write_to_file(m_context->context(), nativePath.c_str(), metadataOnly);

    if (!success) {
        setError("Failed to write the GGUF context to: " + filePath.string());
        return false;
    }

    clearError();
    return true;
}

bool JobGgufWriter::write(std::FILE *file, bool metadataOnly)
{
    clearError();

    if (!isValid()) {
        setError("JobGgufWriter does not have a valid source context");

        return false;
    }

    if (!file) {
        setError("JobGgufWriter requires a valid FILE pointer");

        return false;
    }

    const bool success = gguf_write_to_file_ptr(m_context->context(), file, metadataOnly);

    if (!success) {
        setError("Failed to write the GGUF context to the supplied FILE");

        return false;
    }

    clearError();
    return true;
}

std::size_t JobGgufWriter::metadataSize() const noexcept
{
    if (!isValid())
        return 0;

    return gguf_get_meta_size(m_context->context());
}

std::vector<std::byte> JobGgufWriter::metadata() const
{
    if (!isValid()) {
        throw std::runtime_error{
            "Cannot serialize metadata from an invalid JobGgufContext"
        };
    }

    const std::size_t size = metadataSize();

    std::vector<std::byte> ret(size);

    if (size > 0)
        gguf_get_meta_data( m_context->context(), ret.data());

    return ret;
}

bool JobGgufWriter::writeMetadata(void *destination, std::size_t destinationSize)
{
    clearError();

    if (!isValid()) {
        setError("JobGgufWriter does not have a valid source context");

        return false;
    }

    const std::size_t requiredSize = metadataSize();

    if (requiredSize == 0) {
        setError("The GGUF context does not expose serializable metadata");

        return false;
    }

    if (!destination) {
        setError("JobGgufWriter requires a valid metadata destination");

        return false;
    }

    if (destinationSize < requiredSize) {
        setError("The metadata destination is smaller than the required GGUF metadata size");

        return false;
    }

    gguf_get_meta_data(m_context->context(), destination);

    clearError();
    return true;
}

bool JobGgufWriter::writeMetadata(std::span<std::byte> destination)
{
    if (destination.empty()) {
        clearError();

        setError("JobGgufWriter cannot write metadata into an empty byte span");

        return false;
    }

    return writeMetadata(destination.data(), destination.size_bytes());
}

const std::string &JobGgufWriter::errorString() const noexcept
{
    return m_errorString;
}

bool JobGgufWriter::hasError() const noexcept
{
    return !m_errorString.empty();
}

void JobGgufWriter::clearError() noexcept
{
    m_errorString.clear();
}

void JobGgufWriter::setError(const std::string &errorString)
{
    m_errorString = errorString;
}

void JobGgufWriter::setError(std::string &&errorString)
{
    m_errorString = std::move(errorString);
}

} // namespace job::ggml