#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>

#include "job_http_method.h"
#include "job_http_response.h"
#include "jobnet_export.h"

namespace job::net {

class JOBNET_EXPORT JobHttpParser
{
public:
    using Ptr          = std::shared_ptr<JobHttpParser>;
    using WPtr         = std::weak_ptr<JobHttpParser>;
    using UPtr         = std::unique_ptr<JobHttpParser>;

    static constexpr std::size_t kLineBufferSize   = 8192;
    static constexpr std::size_t kMaxHeaderFields  = 128;
    static constexpr std::size_t kMaxTrailerFields = 32;

    using LineBuffer   = std::array<std::byte, kLineBufferSize>;
    using BodyCallback = std::function<void(std::span<const std::byte>)>;

    enum class State : std::uint8_t
    {
        StatusLine = 0,
        Headers,
        Body,
        ChunkSize,
        ChunkData,
        ChunkDataEnd,
        Trailers,
        Complete,
        Error
    };

    enum class ParseResult : std::uint8_t
    {
        NeedMoreData = 0,
        Complete,
        Error
    };

    enum class BodyMode : std::uint8_t
    {
        Store = 0,
        Stream,
        Discard
    };

    enum class BodyFraming : std::uint8_t
    {
        None = 0,
        ContentLength,
        Chunked,
        CloseDelimited,
        Tunnel
    };

    JobHttpParser() = default;
    ~JobHttpParser() = default;

    JobHttpParser(const JobHttpParser &) = delete;
    JobHttpParser &operator=(const JobHttpParser &) = delete;
    JobHttpParser(JobHttpParser &&) noexcept = default;
    JobHttpParser &operator=(JobHttpParser &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobHttpParser>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobHttpParser>();
    }

    [[nodiscard]] ParseResult parse(std::span<const std::byte> &data);
    [[nodiscard]] ParseResult parse(std::string_view &data);
    [[nodiscard]] ParseResult notifyEof();

    [[nodiscard]] State state() const noexcept;
    [[nodiscard]] BodyFraming bodyFraming() const noexcept;

    [[nodiscard]] bool isComplete() const noexcept;
    [[nodiscard]] bool hasError() const noexcept;

    [[nodiscard]] const std::string &lastError() const noexcept;

    [[nodiscard]] const JobHttpResponse &response() const noexcept;
    [[nodiscard]] JobHttpResponse &response() noexcept;

    [[nodiscard]] std::size_t bytesReceived() const noexcept;
    [[nodiscard]] std::size_t bodyBytesReceived() const noexcept;

    [[nodiscard]] bool hasContentLength() const noexcept;
    [[nodiscard]] std::size_t contentLength() const noexcept;

    void setRequestMethod(HttpMethod method) noexcept;
    [[nodiscard]] HttpMethod requestMethod() const noexcept;

    void setBodyMode(BodyMode mode) noexcept;
    [[nodiscard]] BodyMode bodyMode() const noexcept;

    void setBodyCallback(BodyCallback callback);
    void clearBodyCallback();

    void reset();

private:
    [[nodiscard]] bool parseStatusLine(std::string_view line);
    [[nodiscard]] bool parseFieldLine(std::string_view line, JobHttpHeader &target, std::size_t maxFields);
    [[nodiscard]] bool parseHeaderLine(std::string_view line);
    [[nodiscard]] bool parseTrailerLine(std::string_view line);
    [[nodiscard]] bool parseHeadersComplete();

    [[nodiscard]] static bool isForbiddenTrailer(std::string_view name) noexcept;

    [[nodiscard]] bool parseContentLength();
    [[nodiscard]] bool parseTransferEncoding();

    [[nodiscard]] bool parseChunkSize(std::string_view line);

    [[nodiscard]] bool consumeBody(std::span<const std::byte> &data);
    [[nodiscard]] bool consumeLine(std::span<const std::byte> &data, std::string_view &line);

    void resetMessage();
    void setError(std::string_view error);

private:
    State           m_state{State::StatusLine};
    BodyMode        m_bodyMode{BodyMode::Store};
    BodyFraming     m_bodyFraming{BodyFraming::None};
    HttpMethod      m_requestMethod{HttpMethod::Get};
    JobHttpResponse m_response;

    LineBuffer      m_lineBuffer{};
    std::size_t     m_lineSize{0};

    std::size_t     m_bytesReceived{0};
    std::size_t     m_bodyBytesReceived{0};

    std::size_t     m_contentLength{0};
    std::size_t     m_chunkBytesRemaining{0};

    std::uint8_t    m_httpMajor{0};
    std::uint8_t    m_httpMinor{0};

    bool            m_hasContentLength{false};

    BodyCallback    m_bodyCallback;
    std::string     m_lastError;
};

} // namespace job::net