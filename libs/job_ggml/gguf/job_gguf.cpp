#include "job_gguf.h"

#include <stdexcept>
#include <utility>

#include <gguf.h>

namespace job::ggml {

JobGguf::JobGguf(JobGgmlContext::UPtr *contextOutput) :
    m_initParams{std::make_unique<JobGgufInitParams>(true, contextOutput)},
    m_context{JobGgufContext::createUniq()},
    m_reader{JobGgufReader::createUniq(m_context.get())},
    m_writer{JobGgufWriter::createUniq(m_context.get())}
{
    if (!isValid()) {
        throw std::runtime_error{
            "Failed to construct the JobGguf subsystem"
        };
    }
}

bool JobGguf::isValid() const noexcept
{
    return m_initParams &&
           m_context &&
           m_context->isValid() &&
           m_reader &&
           m_reader->isValid() &&
           m_writer &&
           m_writer->isValid();
}

bool JobGguf::hasContent() const noexcept
{
    return m_context &&
           (m_context->keyValueCount() > 0 ||
            m_context->tensorCount() > 0);
}

JobGgufInitParams *JobGguf::initParams() noexcept
{
    return m_initParams.get();
}

const JobGgufInitParams *JobGguf::initParams() const noexcept
{
    return m_initParams.get();
}

JobGgufContext *JobGguf::context() noexcept
{
    return m_context.get();
}

const JobGgufContext *JobGguf::context() const noexcept
{
    return m_context.get();
}

JobGgufReader *JobGguf::reader() noexcept
{
    return m_reader.get();
}

const JobGgufReader *JobGguf::reader() const noexcept
{
    return m_reader.get();
}

JobGgufWriter *JobGguf::writer() noexcept
{
    return m_writer.get();
}

const JobGgufWriter *JobGguf::writer() const noexcept
{
    return m_writer.get();
}

void JobGguf::setContextOutput(JobGgmlContext::UPtr *contextOutput) noexcept
{
    if (!m_initParams)
        return;

    m_initParams->setContextOutput(contextOutput);
}

JobGgmlContext::UPtr *JobGguf::contextOutput() const noexcept
{
    return m_initParams ? m_initParams->contextOutput() : nullptr;
}

// Start Reading
bool JobGguf::open(const std::filesystem::path &filePath)
{
    clearError();

    if (!m_reader || !m_initParams) {
        setError("JobGguf does not have a valid reader or initialization parameters");

        return false;
    }

    if (!m_reader->read(filePath, *m_initParams)) {
        adoptReaderError();
        return false;
    }

    clearError();
    return true;
}

bool JobGguf::open(std::FILE *file)
{
    clearError();

    if (!m_reader || !m_initParams) {
        setError("JobGguf does not have a valid reader or initialization parameters");

        return false;
    }

    if (!m_reader->read(
            file,
            *m_initParams
            )) {
        adoptReaderError();
        return false;
    }

    clearError();
    return true;
}

bool JobGguf::open(const void *data, std::size_t size)
{
    clearError();

    if (!m_reader || !m_initParams) {
        setError("JobGguf does not have a valid reader or initialization parameters");

        return false;
    }

    if (!m_reader->read(data, size, *m_initParams)) {
        adoptReaderError();
        return false;
    }

    clearError();
    return true;
}

bool JobGguf::open(std::span<const std::byte> data)
{
    clearError();

    if (!m_reader || !m_initParams) {
        setError("JobGguf does not have a valid reader or initialization parameters");

        return false;
    }

    if (!m_reader->read(data, *m_initParams)) {
        adoptReaderError();
        return false;
    }

    clearError();
    return true;
}

bool JobGguf::open(JobGgufReader::ReadCallback callback, std::size_t maxChunkRead, std::uint64_t maxExpectedSize)
{
    clearError();

    if (!m_reader || !m_initParams) {
        setError("JobGguf does not have a valid reader or initialization parameters");

        return false;
    }

    if (!m_reader->read(
            std::move(callback),
            maxChunkRead,
            maxExpectedSize,
            *m_initParams
            )) {
        adoptReaderError();
        return false;
    }

    clearError();
    return true;
}


// Writing
bool JobGguf::save(const std::filesystem::path &filePath, bool metadataOnly)
{
    clearError();

    if (!m_writer) {
        setError("JobGguf does not have a valid writer");
        return false;
    }

    if (!m_writer->write(filePath, metadataOnly)) {
        adoptWriterError();
        return false;
    }

    clearError();
    return true;
}

bool JobGguf::save(std::FILE *file, bool metadataOnly)
{
    clearError();

    if (!m_writer) {
        setError("JobGguf does not have a valid writer");

        return false;
    }

    if (!m_writer->write(file, metadataOnly)) {
        adoptWriterError();
        return false;
    }

    clearError();
    return true;
}

std::size_t JobGguf::metadataSize() const noexcept
{
    return m_writer ? m_writer->metadataSize() : 0;
}

std::vector<std::byte> JobGguf::metadata() const
{
    if (!m_writer) {
        throw std::runtime_error{
            "JobGguf does not have a valid writer"
        };
    }

    return m_writer->metadata();
}

bool JobGguf::writeMetadata(void *destination, std::size_t destinationSize)
{
    clearError();

    if (!m_writer) {
        setError("JobGguf does not have a valid writer");

        return false;
    }

    if (!m_writer->writeMetadata(destination, destinationSize)) {
        adoptWriterError();
        return false;
    }

    clearError();
    return true;
}

bool JobGguf::writeMetadata(std::span<std::byte> destination)
{
    clearError();

    if (!m_writer) {
        setError("JobGguf does not have a valid writer");

        return false;
    }

    if (!m_writer->writeMetadata(destination)) {
        adoptWriterError();
        return false;
    }

    clearError();
    return true;
}


// Common context inspection
std::uint32_t JobGguf::version() const noexcept
{
    return m_context ? m_context->version() : 0;
}

std::size_t JobGguf::alignment() const noexcept
{
    return m_context ? m_context->alignment() : 0;
}

std::size_t JobGguf::dataOffset() const noexcept
{
    return m_context ? m_context->dataOffset() : 0;
}

std::int64_t JobGguf::keyValueCount() const noexcept
{
    return m_context ? m_context->keyValueCount() : 0;
}

std::int64_t JobGguf::tensorCount() const noexcept
{
    return m_context ? m_context->tensorCount() : 0;
}

bool JobGguf::hasKey(const std::string &key) const noexcept
{
    return m_context && m_context->hasKey(key);
}

bool JobGguf::hasTensor(const std::string &name) const noexcept
{
    return m_context && m_context->hasTensor(name);
}

JobGgufKv::UPtr JobGguf::keyValue(const std::string &key) const
{
    if (!m_context) {
        throw std::runtime_error{
            "JobGguf does not have a valid context"
        };
    }

    return m_context->keyValue(key);
}

JobGgufKv::UPtr JobGguf::keyValue(std::int64_t index) const
{
    if (!m_context) {
        throw std::runtime_error{
            "JobGguf does not have a valid context"
        };
    }

    return m_context->keyValue(index);
}

std::vector<JobGgufKv::UPtr> JobGguf::keyValues() const
{
    if (!m_context) {
        throw std::runtime_error{
            "JobGguf does not have a valid context"
        };
    }

    return m_context->keyValues();
}

// Common context mutation
void JobGguf::setKeyValue(
    const JobGgufKv &keyValue
    )
{
    if (!m_context) {
        throw std::runtime_error{
            "JobGguf does not have a valid context"
        };
    }

    m_context->setKeyValue(keyValue);
}

void JobGguf::setKeyValues(const JobGgufContext &source)
{
    if (!m_context) {
        throw std::runtime_error{
            "JobGguf does not have a valid context"
        };
    }

    m_context->setKeyValues(source);
}

std::int64_t JobGguf::removeKey(const std::string &key)
{
    if (!m_context) {
        throw std::runtime_error{
            "JobGguf does not have a valid context"
        };
    }

    return m_context->removeKey(key);
}

void JobGguf::addTensor(const JobGgmlTensor &tensor)
{
    if (!m_context) {
        throw std::runtime_error{
            "JobGguf does not have a valid context"
        };
    }

    m_context->addTensor(tensor);
}

void JobGguf::setTensorType(const std::string &name, JobGgmlType type)
{
    if (!m_context) {
        throw std::runtime_error{
            "JobGguf does not have a valid context"
        };
    }

    m_context->setTensorType(name, type);
}

void JobGguf::setTensorData(const std::string &name, const void *data)
{
    if (!m_context) {
        throw std::runtime_error{
            "JobGguf does not have a valid context"
        };
    }

    m_context->setTensorData(name, data);
}

void JobGguf::setTensorData(const std::string &name, const std::vector<std::byte> &data)
{
    if (!m_context) {
        throw std::runtime_error{
            "JobGguf does not have a valid context"
        };
    }

    m_context->setTensorData(name, data);
}

void JobGguf::reset()
{
    if (!m_context) {
        throw std::runtime_error{
            "JobGguf does not have a valid context wrapper"
        };
    }

    struct gguf_context *nativeContext = gguf_init_empty();

    if (!nativeContext) {
        throw std::runtime_error{
            "Failed to create an empty native GGUF context"
        };
    }

    /*
     * Preserve the JobGgufContext wrapper address borrowed by m_reader and
     * m_writer. Only replace the native context that it owns.
     */
    m_context->reset(nativeContext);

    if (m_reader)
        m_reader->clearError();

    if (m_writer)
        m_writer->clearError();

    clearError();
}

const std::string &JobGguf::errorString() const noexcept
{
    return m_errorString;
}

bool JobGguf::hasError() const noexcept
{
    return !m_errorString.empty();
}

void JobGguf::clearError() noexcept
{
    m_errorString.clear();

    if (m_reader)
        m_reader->clearError();

    if (m_writer)
        m_writer->clearError();
}

void JobGguf::adoptReaderError()
{
    if (!m_reader) {
        setError("JobGguf reader is unavailable");

        return;
    }

    if (m_reader->hasError())
        setError(m_reader->errorString());
    else
        setError("JobGgufReader failed without an error description");

}

void JobGguf::adoptWriterError()
{
    if (!m_writer) {
        setError( "JobGguf writer is unavailable" );
        return;
    }

    if (m_writer->hasError())
        setError( m_writer->errorString() );
    else
        setError( "JobGgufWriter failed without an error description" );
}

void JobGguf::setError(const std::string &errorString)
{
    m_errorString = errorString;
}

void JobGguf::setError(std::string &&errorString)
{
    m_errorString = std::move(errorString);
}

} // namespace job::ggml