#include "job_gguf_reader.h"

#include <exception>
#include <stdexcept>
#include <utility>

#include <ggml-cpp.h>

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

JobGgufReader::JobGgufReader(JobGgufContext *context) :
    m_context{context}
{
    if (!m_context) {
        throw std::invalid_argument{
            "JobGgufReader requires a valid JobGgufContext"
        };
    }
}

bool JobGgufReader::isValid() const noexcept
{
    return m_context != nullptr;
}

bool JobGgufReader::read(const std::filesystem::path &filePath, const JobGgufInitParams &initParams)
{
    clearError();

    if (!isValid()) {
        setError("JobGgufReader does not have a valid destination context");
        return false;
    }

    if (filePath.empty()) {
        setError("JobGgufReader requires a non-empty file path");
        return false;
    }

    std::error_code errorCode;

    const bool exists = std::filesystem::exists(filePath, errorCode);
    if (errorCode) {
        setError("Failed to inspect the GGUF file path: " + errorCode.message());
        return false;
    }

    if (!exists) {
        setError("GGUF file does not exist: " + filePath.string());
        return false;
    }

    const bool regularFile = std::filesystem::is_regular_file(filePath, errorCode);

    if (errorCode) {
        setError("Failed to inspect the GGUF file type: " + errorCode.message());
        return false;
    }

    if (!regularFile) {
        setError("GGUF path does not reference a regular file: " + filePath.string());
        return false;
    }

    const std::string nativePath = pathToUtf8(filePath);
    if (nativePath.empty()) {
        setError("Failed to convert the GGUF file path");
        return false;
    }

    struct ggml_context *nativeGgmlContext = nullptr;
    const struct gguf_init_params nativeParams = initParams.params( initParams.createContext() ? &nativeGgmlContext : nullptr );

    struct gguf_context *nativeGgufContext = gguf_init_from_file(nativePath.c_str(), nativeParams);
    return finishRead(nativeGgufContext, nativeGgmlContext, initParams);
}

bool JobGgufReader::read(std::FILE *file, const JobGgufInitParams &initParams)
{
    clearError();

    if (!isValid()) {
        setError("JobGgufReader does not have a valid destination context");
        return false;
    }

    if (!file) {
        setError("JobGgufReader requires a valid FILE pointer");

        return false;
    }

    struct ggml_context *nativeGgmlContext = nullptr;
    const struct gguf_init_params nativeParams = initParams.params(initParams.createContext() ? &nativeGgmlContext : nullptr);
    struct gguf_context *nativeGgufContext = gguf_init_from_file_ptr(file, nativeParams);

    return finishRead(nativeGgufContext, nativeGgmlContext, initParams);
}

bool JobGgufReader::read(const void *data, std::size_t size, const JobGgufInitParams &initParams)
{
    clearError();

    if (!isValid()) {
        setError("JobGgufReader does not have a valid destination context");
        return false;
    }

    if (!data) {
        setError("JobGgufReader requires a valid input buffer");

        return false;
    }

    if (size == 0) {
        setError("JobGgufReader cannot read an empty input buffer");

        return false;
    }

    struct ggml_context *nativeGgmlContext = nullptr;
    const struct gguf_init_params nativeParams = initParams.params(
        initParams.createContext() ? &nativeGgmlContext : nullptr
        );

    struct gguf_context *nativeGgufContext = gguf_init_from_buffer(data, size, nativeParams);

    return finishRead(nativeGgufContext, nativeGgmlContext, initParams);
}

bool JobGgufReader::read(std::span<const std::byte> data, const JobGgufInitParams &initParams)
{
    if (data.empty()) {
        clearError();

        setError("JobGgufReader cannot read an empty byte span");

        return false;
    }

    return read(data.data(), data.size_bytes(), initParams);
}

bool JobGgufReader::read(ReadCallback callback, std::size_t maxChunkRead, std::uint64_t maxExpectedSize, const JobGgufInitParams &initParams)
{
    clearError();

    if (!isValid()) {
        setError("JobGgufReader does not have a valid destination context");

        return false;
    }

    if (!callback) {
        setError("JobGgufReader requires a valid read callback");

        return false;
    }

    if (maxExpectedSize == 0) {
        setError("JobGgufReader requires a positive maximum expected size");

        return false;
    }

    CallbackState callbackState{
        &callback,
        &m_errorString
    };

    struct ggml_context *nativeGgmlContext = nullptr;
    const struct gguf_init_params nativeParams = initParams.params(
        initParams.createContext() ? &nativeGgmlContext : nullptr
        );

    struct gguf_context *nativeGgufContext = gguf_init_from_callback(
        &JobGgufReader::callbackTrampoline,
        &callbackState,
        maxChunkRead,
        maxExpectedSize,
        nativeParams
        );

    if (!nativeGgufContext &&
        m_errorString.empty()) {
        setError("Failed to read GGUF data through the supplied callback");
    }

    return finishRead(nativeGgufContext, nativeGgmlContext, initParams);
}

const std::string &JobGgufReader::errorString() const noexcept
{
    return m_errorString;
}

bool JobGgufReader::hasError() const noexcept
{
    return !m_errorString.empty();
}

void JobGgufReader::clearError() noexcept
{
    m_errorString.clear();
}

std::size_t JobGgufReader::callbackTrampoline(void *userData, void *output, std::uint64_t offset, std::size_t length) noexcept
{
    if (!userData)
        return 0;

    auto *state = static_cast<CallbackState *>(userData);
    if (!state->callback || !(*state->callback)) {

        if (state->errorString)
            *state->errorString = "GGUF callback state does not contain a valid callback";

        return 0;
    }

    if (!output && length > 0) {
        if (state->errorString)
            *state->errorString = "GGUF callback received a null output buffer";

        return 0;
    }

    if (length == 0)
        return 0;

    try {
        const std::size_t bytesRead = (*state->callback)(
            output,
            offset,
            length
            );

        if (bytesRead > length) {
            if (state->errorString)
                *state->errorString = "GGUF callback returned more bytes than requested";

            return 0;
        }

        return bytesRead;
    } catch (const std::exception &exception) {
        if (state->errorString) {
            *state->errorString = std::string{"GGUF callback threw an exception: "} + exception.what();
        }

        return 0;
    } catch (...) {
        if (state->errorString) {
            *state->errorString = "GGUF callback threw an unknown exception";
        }
        return 0;
    }
}

bool JobGgufReader::finishRead(struct gguf_context *nativeGgufContext, struct ggml_context *nativeGgmlContext, const JobGgufInitParams &initParams)
{
    /*
     * Hold any native GGML context in RAII storage until ownership can be
     * committed to the JobGgml-owned output destination.
     */
    ggml_context_ptr ownedGgmlContext{ nativeGgmlContext };
    if (!nativeGgufContext) {
        if (m_errorString.empty()) {
            setError("Upstream GGUF parsing failed");
        }

        return false;
    }

    /*
     * Keep ownership local until every requested result can be committed.
     * This preserves the previously loaded JobGgufContext when a later part
     * of the operation fails.
     */
    struct NativeGgufContextGuard
    {
        struct gguf_context *context{nullptr};
        ~NativeGgufContextGuard()
        {
            if (context)
                gguf_free(context);
        }

        [[nodiscard]] struct gguf_context *release() noexcept
        {
            struct gguf_context *ret = context;
            context = nullptr;
            return ret;
        }
    };

    NativeGgufContextGuard ownedGgufContext
        {
            nativeGgufContext
        };

    JobGgmlContext::UPtr wrappedGgmlContext;

    if (initParams.createContext()) {
        if (!ownedGgmlContext) {
            setError("GGUF was requested to create a GGML context, but none was returned");

            return false;
        }

        JobGgmlContext::UPtr *contextOutput = initParams.contextOutput();
        if (!contextOutput) {
            setError("GGUF created a GGML context, but JobGgufInitParams has no JobGgmlContext output destination");

            return false;
        }

        try {
            wrappedGgmlContext = JobGgmlContext::createUniq(std::move(ownedGgmlContext));
        } catch (const std::exception &exception) {
            setError(std::string{"Failed to wrap the GGML context created by GGUF: "} + exception.what());

            return false;
        } catch (...) {
            setError("Failed to wrap the GGML context created by GGUF");

            return false;
        }
    } else if (ownedGgmlContext) {
        /*
         * Upstream should not create a context when no output slot was
         * supplied. Treat an unexpected result as an ownership error rather
         * than silently discarding it.
         */
        setError("GGUF returned an unexpected GGML context");

        return false;
    }

    /*
     * Commit only after all validation and wrapper construction succeeded.
     *
     * JobGguf owns m_context, while JobGgufReader merely replaces the native
     * gguf_context held by that stable wrapper.
     */
    m_context->reset(ownedGgufContext.release());

    if (wrappedGgmlContext) {
        JobGgmlContext::UPtr *contextOutput = initParams.contextOutput();

        *contextOutput = std::move(wrappedGgmlContext);
    }

    clearError();
    return true;
}

void JobGgufReader::setError(const std::string &errorString)
{
    m_errorString = errorString;
}

void JobGgufReader::setError(std::string &&errorString)
{
    m_errorString = std::move(errorString);
}

} // namespace job::ggml