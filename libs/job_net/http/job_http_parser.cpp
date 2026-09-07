#include "job_http_parser.h"

#include <algorithm>
#include <charconv>
#include <limits>
#include <utility>

namespace job::net {

JobHttpParser::ParseResult JobHttpParser::parse(std::span<const std::byte> &data)
{
    const std::size_t initialSize = data.size();

    const auto result = [this, &data]() -> ParseResult {
        for (;;) {
            if (m_state == State::Error)
                return ParseResult::Error;

            if (m_state == State::Complete)
                return ParseResult::Complete;

            switch (m_state) {
            case State::StatusLine: {
                std::string_view line;
                if (!consumeLine(data, line))
                    return m_state == State::Error ? ParseResult::Error : ParseResult::NeedMoreData;

                if (!parseStatusLine(line))
                    return ParseResult::Error;

                m_state = State::Headers;
                break;
            }

            case State::Headers: {
                std::string_view line;
                if (!consumeLine(data, line))
                    return m_state == State::Error ? ParseResult::Error : ParseResult::NeedMoreData;

                if (!line.empty()) {
                    if (!parseHeaderLine(line))
                        return ParseResult::Error;
                    break;
                }

                if (!parseHeadersComplete())
                    return ParseResult::Error;

                break;
            }

            case State::Body:
                if (m_bodyFraming == BodyFraming::ContentLength) {
                    if (m_bodyBytesReceived >= m_contentLength) {
                        m_state = State::Complete;
                        break;
                    }

                    if (data.empty())
                        return ParseResult::NeedMoreData;

                    const std::size_t remaining = m_contentLength - m_bodyBytesReceived;
                    const std::size_t take = std::min(remaining, data.size());
                    auto body = data.first(take);

                    if (!consumeBody(body))
                        return ParseResult::Error;

                    data = data.subspan(take);

                    if (m_bodyBytesReceived == m_contentLength)
                        m_state = State::Complete;

                    break;
                }

                if (m_bodyFraming == BodyFraming::CloseDelimited) {
                    if (data.empty())
                        return ParseResult::NeedMoreData;

                    auto body = data;
                    if (!consumeBody(body))
                        return ParseResult::Error;

                    data = data.subspan(body.size());
                    return ParseResult::NeedMoreData;
                }

                setError("HTTP parser entered body state without body framing");
                return ParseResult::Error;

            case State::ChunkSize: {
                std::string_view line;
                if (!consumeLine(data, line))
                    return m_state == State::Error ? ParseResult::Error : ParseResult::NeedMoreData;

                if (!parseChunkSize(line))
                    return ParseResult::Error;

                if (m_chunkBytesRemaining == 0)
                    m_state = State::Trailers;
                else
                    m_state = State::ChunkData;

                break;
            }

            case State::ChunkData: {
                if (m_chunkBytesRemaining == 0) {
                    m_state = State::ChunkDataEnd;
                    break;
                }

                if (data.empty())
                    return ParseResult::NeedMoreData;

                const std::size_t take = std::min(m_chunkBytesRemaining, data.size());
                auto body = data.first(take);

                if (!consumeBody(body))
                    return ParseResult::Error;

                data = data.subspan(take);
                m_chunkBytesRemaining -= take;

                if (m_chunkBytesRemaining == 0)
                    m_state = State::ChunkDataEnd;

                break;
            }

            case State::ChunkDataEnd: {
                std::string_view line;
                if (!consumeLine(data, line))
                    return m_state == State::Error ? ParseResult::Error : ParseResult::NeedMoreData;

                if (!line.empty()) {
                    setError("Invalid HTTP chunk terminator");
                    return ParseResult::Error;
                }

                m_state = State::ChunkSize;
                break;
            }

            case State::Trailers: {
                std::string_view line;
                if (!consumeLine(data, line))
                    return m_state == State::Error ? ParseResult::Error : ParseResult::NeedMoreData;

                if (line.empty()) {
                    m_state = State::Complete;
                    break;
                }

                if (!parseTrailerLine(line))
                    return ParseResult::Error;

                break;
            }

            case State::Complete:
                return ParseResult::Complete;

            case State::Error:
                return ParseResult::Error;
            }
        }
    }();

    m_bytesReceived += initialSize - data.size();
    return result;
}

JobHttpParser::ParseResult JobHttpParser::parse(std::string_view &data)
{
    auto bytes = std::span<const std::byte>{
        reinterpret_cast<const std::byte *>(data.data()),
        data.size()
    };

    const std::size_t initialSize = bytes.size();
    const ParseResult result = parse(bytes);
    data.remove_prefix(initialSize - bytes.size());
    return result;
}

JobHttpParser::ParseResult JobHttpParser::notifyEof()
{
    if (m_state == State::Error)
        return ParseResult::Error;

    if (m_state == State::Complete)
        return ParseResult::Complete;

    if (m_bodyFraming == BodyFraming::CloseDelimited && m_state == State::Body) {
        m_state = State::Complete;
        return ParseResult::Complete;
    }

    if (m_bodyFraming == BodyFraming::ContentLength) {
        setError("Unexpected EOF before Content-Length body completed");
        return ParseResult::Error;
    }

    if (m_bodyFraming == BodyFraming::Chunked) {
        setError("Unexpected EOF before chunked body completed");
        return ParseResult::Error;
    }

    setError("Unexpected EOF while parsing HTTP response");
    return ParseResult::Error;
}

JobHttpParser::State JobHttpParser::state() const noexcept
{
    return m_state;
}

JobHttpParser::BodyFraming JobHttpParser::bodyFraming() const noexcept
{
    return m_bodyFraming;
}

bool JobHttpParser::isComplete() const noexcept
{
    return m_state == State::Complete;
}

bool JobHttpParser::hasError() const noexcept
{
    return m_state == State::Error;
}

const std::string &JobHttpParser::lastError() const noexcept
{
    return m_lastError;
}

const JobHttpResponse &JobHttpParser::response() const noexcept
{
    return m_response;
}

JobHttpResponse &JobHttpParser::response() noexcept
{
    return m_response;
}

std::size_t JobHttpParser::bytesReceived() const noexcept
{
    return m_bytesReceived;
}

std::size_t JobHttpParser::bodyBytesReceived() const noexcept
{
    return m_bodyBytesReceived;
}

bool JobHttpParser::hasContentLength() const noexcept
{
    return m_hasContentLength;
}

std::size_t JobHttpParser::contentLength() const noexcept
{
    return m_contentLength;
}

void JobHttpParser::setRequestMethod(HttpMethod method) noexcept
{
    m_requestMethod = method;
}

HttpMethod JobHttpParser::requestMethod() const noexcept
{
    return m_requestMethod;
}

void JobHttpParser::setBodyMode(BodyMode mode) noexcept
{
    m_bodyMode = mode;
}

JobHttpParser::BodyMode JobHttpParser::bodyMode() const noexcept
{
    return m_bodyMode;
}

void JobHttpParser::setBodyCallback(BodyCallback callback)
{
    m_bodyCallback = std::move(callback);
}

void JobHttpParser::clearBodyCallback()
{
    m_bodyCallback = {};
}

void JobHttpParser::reset()
{
    m_state = State::StatusLine;
    m_bodyFraming = BodyFraming::None;
    m_response.clear();

    m_lineSize = 0;

    m_bytesReceived = 0;
    m_bodyBytesReceived = 0;

    m_contentLength = 0;
    m_chunkBytesRemaining = 0;

    m_httpMajor = 0;
    m_httpMinor = 0;

    m_hasContentLength = false;

    m_lastError.clear();
}

bool JobHttpParser::parseStatusLine(std::string_view line)
{
    constexpr std::string_view prefix = "HTTP/";

    if (!line.starts_with(prefix)) {
        setError("Invalid HTTP status line");
        return false;
    }

    line.remove_prefix(prefix.size());

    const std::size_t dot = line.find('.');
    if (dot == std::string_view::npos || dot == 0) {
        setError("Invalid HTTP version");
        return false;
    }

    const std::size_t firstSpace = line.find(' ');
    if (firstSpace == std::string_view::npos || dot >= firstSpace) {
        setError("Invalid HTTP status line");
        return false;
    }

    const std::string_view majorText = line.substr(0, dot);
    const std::string_view minorText = line.substr(dot + 1, firstSpace - dot - 1);

    unsigned int major = 0;
    unsigned int minor = 0;

    {
        const auto result = std::from_chars(majorText.data(), majorText.data() + majorText.size(), major);
        if (result.ec != std::errc{} || result.ptr != majorText.data() + majorText.size() ||
            major > std::numeric_limits<std::uint8_t>::max()) {
            setError("Invalid HTTP major version");
            return false;
        }
    }

    {
        const auto result = std::from_chars(minorText.data(), minorText.data() + minorText.size(), minor);
        if (result.ec != std::errc{} || result.ptr != minorText.data() + minorText.size() ||
            minor > std::numeric_limits<std::uint8_t>::max()) {
            setError("Invalid HTTP minor version");
            return false;
        }
    }

    if (major != 1 || minor > 1) {
        setError("Unsupported HTTP version");
        return false;
    }

    m_httpMajor = static_cast<std::uint8_t>(major);
    m_httpMinor = static_cast<std::uint8_t>(minor);

    line.remove_prefix(firstSpace + 1);

    if (line.size() < 3) {
        setError("Invalid HTTP status code");
        return false;
    }

    const std::string_view codeText = line.substr(0, 3);

    if (codeText[0] < '0' || codeText[0] > '9' ||
        codeText[1] < '0' || codeText[1] > '9' ||
        codeText[2] < '0' || codeText[2] > '9') {
        setError("Invalid HTTP status code");
        return false;
    }

    unsigned int code = 0;
    const auto codeResult = std::from_chars(codeText.data(), codeText.data() + codeText.size(), code);

    if (codeResult.ec != std::errc{} || codeResult.ptr != codeText.data() + codeText.size() ||
        code < 100 || code > 999) {
        setError("Invalid HTTP status code");
        return false;
    }

    std::string_view reasonPhrase;

    if (line.size() > 3) {
        if (line[3] != ' ') {
            setError("Invalid HTTP status line");
            return false;
        }

        reasonPhrase = line.substr(4);

        for (char c : reasonPhrase) {
            const unsigned char uc = static_cast<unsigned char>(c);
            if ((uc < 0x20 && c != '\t') || uc == 0x7f) {
                setError("Invalid HTTP reason phrase");
                return false;
            }
        }
    }

    m_response.setCode(static_cast<std::uint16_t>(code));
    m_response.setReasonPhrase(reasonPhrase);
    return true;
}

bool JobHttpParser::isForbiddenTrailer(std::string_view name) noexcept
{
    // RFC 9110 6.5.1 prohibits trailer fields that are necessary for message
    // framing, routing, authentication, request modification, response control or content processing.
    // It states the rule as categories rather than a list, so the names below are
    // JOB's own conservative reading of it, not a table copied from the specification.
    static constexpr std::string_view kForbidden[] = {
        "transfer-encoding",
        "content-length",
        "host",
        "trailer",
        "te",
        "connection",
        "keep-alive",
        "upgrade",
        "proxy-connection",
        "expect",
        "content-encoding",
        "content-type",
        "content-range",
        "authorization",
        "proxy-authorization",
        "www-authenticate",
        "proxy-authenticate",
        "set-cookie",
        "cookie",
        "cache-control",
        "location",
    };

    for (const std::string_view forbidden : kForbidden) {
        if (JobHttpHeader::equalsIgnoreCase(name, forbidden))
            return true;
    }

    return false;
}

bool JobHttpParser::parseFieldLine(std::string_view line, JobHttpHeader &target, std::size_t maxFields)
{
    if (line.empty()) {
        setError("Invalid empty HTTP header line");
        return false;
    }

    if (line.front() == ' ' || line.front() == '\t') {
        setError("Obsolete folded HTTP header fields are not supported");
        return false;
    }

    const std::size_t colon = line.find(':');
    if (colon == std::string_view::npos || colon == 0) {
        setError("Invalid HTTP header field");
        return false;
    }

    const std::string_view name = line.substr(0, colon);
    const std::string_view value = line.substr(colon + 1);

    for (char c : value) {
        const unsigned char uc = static_cast<unsigned char>(c);
        if ((uc < 0x20 && c != '\t') || uc == 0x7f) {
            setError("Invalid control character in HTTP header field");
            return false;
        }
    }

    if (target.size() >= maxFields) {
        setError("Too many HTTP header fields");
        return false;
    }

    if (!target.append(name, value)) {
        setError("Unable to append HTTP header field");
        return false;
    }

    return true;
}

bool JobHttpParser::parseHeaderLine(std::string_view line)
{
    return parseFieldLine(line, m_response.headers(), kMaxHeaderFields);
}

bool JobHttpParser::parseTrailerLine(std::string_view line)
{
    const std::size_t colon = line.find(':');
    if (colon != std::string_view::npos && colon != 0 && isForbiddenTrailer(line.substr(0, colon))) {
        setError("Forbidden field in HTTP trailer section");
        return false;
    }

    // Trailers land in their own section: they arrive after the body and must not be mistaken for the initial field section by anything downstream.
    return parseFieldLine(line, m_response.trailers(), kMaxTrailerFields);
}

bool JobHttpParser::parseHeadersComplete()
{
    m_hasContentLength = m_response.headers().contains("content-length");

    if (m_hasContentLength && !parseContentLength())
        return false;

    const bool hasTransferEncoding = m_response.headers().contains("transfer-encoding");
    const std::uint16_t code = m_response.code();

    // RFC 9112 6.3 decides framing in a fixed order. The cases below are bodyless (or opaque) by message semantics and take precedence over any
    // Transfer-Encoding or Content-Length the sender supplied. So they are resolved before those fields are consulted for framing.
    if (code >= 100 && code < 200) {
        if (code == 101) {
            m_bodyFraming = BodyFraming::Tunnel;
            m_state = State::Complete;
            return true;
        }

        resetMessage();
        return true;
    }

    if (m_requestMethod == HttpMethod::Head) {
        m_bodyFraming = BodyFraming::None;
        m_state = State::Complete;
        return true;
    }

    if (m_requestMethod == HttpMethod::Connect && code >= 200 && code < 300) {
        m_bodyFraming = BodyFraming::Tunnel;
        m_state = State::Complete;
        return true;
    }

    if (code == 204 || code == 304) {
        m_bodyFraming = BodyFraming::None;
        m_state = State::Complete;
        return true;
    }

    if (hasTransferEncoding && !parseTransferEncoding())
        return false;

    // RFC 9112 6.1: Transfer-Encoding overrides Content-Length, but a sender
    // that supplies both is either broken or attempting response smuggling... jerk
    // an intermediary that picks the other field sees a different message boundary than we do.
    // Reject rather than silently pick a winner. This is only reachable once the fields are load-bearing for framing.
    if (hasTransferEncoding && m_hasContentLength) {
        setError("Both Transfer-Encoding and Content-Length present");
        return false;
    }

    if (hasTransferEncoding) {
        if (m_bodyFraming == BodyFraming::Chunked)
            m_state = State::ChunkSize;
        else
            m_state = State::Body;

        return true;
    }

    if (m_hasContentLength) {
        m_bodyFraming = BodyFraming::ContentLength;

        if (m_contentLength == 0)
            m_state = State::Complete;
        else
            m_state = State::Body;

        return true;
    }

    m_bodyFraming = BodyFraming::CloseDelimited;
    m_state = State::Body;
    return true;
}

bool JobHttpParser::parseContentLength()
{
    const std::vector<std::string_view> members = m_response.headers().listMembers("content-length");

    if (members.empty()) {
        setError("Empty Content-Length field");
        return false;
    }

    std::size_t parsedLength = 0;
    bool haveLength = false;

    for (std::string_view member : members) {
        if (member.empty()) {
            setError("Invalid empty Content-Length value");
            return false;
        }

        std::size_t value = 0;
        const auto result = std::from_chars(member.data(), member.data() + member.size(), value, 10);

        if (result.ec != std::errc{} || result.ptr != member.data() + member.size()) {
            setError("Invalid Content-Length value");
            return false;
        }

        if (!haveLength) {
            parsedLength = value;
            haveLength = true;
            continue;
        }

        if (value != parsedLength) {
            setError("Conflicting Content-Length values");
            return false;
        }
    }

    m_contentLength = parsedLength;
    return true;
}

bool JobHttpParser::parseTransferEncoding()
{
    const std::vector<std::string_view> members = m_response.headers().listMembers("transfer-encoding");

    if (members.empty()) {
        setError("Empty Transfer-Encoding field");
        return false;
    }

    const auto trimOws = [](std::string_view value) noexcept {
        while (!value.empty() && (value.front() == ' ' || value.front() == '\t'))
            value.remove_prefix(1);

        while (!value.empty() && (value.back() == ' ' || value.back() == '\t'))
            value.remove_suffix(1);

        return value;
    };

    bool sawChunked = false;

    for (std::size_t i = 0; i < members.size(); ++i) {
        std::string_view member = trimOws(members[i]);

        if (member.empty()) {
            setError("Invalid empty Transfer-Encoding member");
            return false;
        }

        const std::size_t semicolon = member.find(';');
        std::string_view coding = semicolon == std::string_view::npos ? member : member.substr(0, semicolon);
        coding = trimOws(coding);

        if (coding.empty()) {
            setError("Invalid Transfer-Encoding member");
            return false;
        }

        if (!JobHttpHeader::isValidFieldName(coding)) {
            setError("Invalid Transfer-Encoding coding");
            return false;
        }

        if (JobHttpHeader::equalsIgnoreCase(coding, "chunked")) {
            if (sawChunked) {
                setError("Duplicate chunked Transfer-Encoding");
                return false;
            }

            sawChunked = true;

            if (i + 1 != members.size()) {
                setError("Chunked Transfer-Encoding must be final");
                return false;
            }

            if (semicolon != std::string_view::npos) {
                setError("Chunked Transfer-Encoding cannot have parameters");
                return false;
            }
        }
    }

    m_bodyFraming = sawChunked ? BodyFraming::Chunked : BodyFraming::CloseDelimited;
    return true;
}

bool JobHttpParser::parseChunkSize(std::string_view line)
{
    const std::size_t semicolon = line.find(';');
    const std::string_view sizeText = semicolon == std::string_view::npos ? line : line.substr(0, semicolon);

    if (sizeText.empty()) {
        setError("Invalid empty HTTP chunk size");
        return false;
    }

    std::size_t chunkSize = 0;
    const auto result = std::from_chars(sizeText.data(), sizeText.data() + sizeText.size(), chunkSize, 16);

    if (result.ec != std::errc{} || result.ptr != sizeText.data() + sizeText.size()) {
        setError("Invalid HTTP chunk size");
        return false;
    }

    m_chunkBytesRemaining = chunkSize;
    return true;
}

bool JobHttpParser::consumeBody(std::span<const std::byte> &data)
{
    if (data.empty())
        return true;

    switch (m_bodyMode) {
    case BodyMode::Store:
        try {
            m_response.appendBody(data); // added
        } catch (...) {
            setError("Unable to store HTTP response body");
            return false;
        }
        break;

    case BodyMode::Stream:
        if (!m_bodyCallback) {
            setError("HTTP body stream mode requires a callback");
            return false;
        }

        try {
            m_bodyCallback(data);
        } catch (...) {
            setError("HTTP body callback threw an exception");
            return false;
        }
        break;

    case BodyMode::Discard:
        break;
    }

    m_bodyBytesReceived += data.size();
    return true;
}

bool JobHttpParser::consumeLine(std::span<const std::byte> &data, std::string_view &line)
{
    line = {};

    if (m_lineSize == 0) {
        for (std::size_t i = 0; i + 1 < data.size(); ++i) {
            const char current = static_cast<char>(data[i]);
            const char next = static_cast<char>(data[i + 1]);

            if (current != '\r' || next != '\n')
                continue;

            if (i + 2 > kLineBufferSize) {
                setError("HTTP line exceeded maximum allowed size");
                return false;
            }

            line = std::string_view{
                reinterpret_cast<const char *>(data.data()),
                i
            };

            data = data.subspan(i + 2);
            return true;
        }

        if (data.empty())
            return false;
    }

    while (!data.empty()) {
        if (m_lineSize >= kLineBufferSize) {
            setError("HTTP line exceeded maximum allowed size");
            return false;
        }

        const std::byte value = data.front();
        m_lineBuffer[m_lineSize++] = value;
        data = data.subspan(1);

        if (m_lineSize >= 2 &&
            m_lineBuffer[m_lineSize - 2] == std::byte{'\r'} &&
            m_lineBuffer[m_lineSize - 1] == std::byte{'\n'}) {
            line = std::string_view{
                reinterpret_cast<const char *>(m_lineBuffer.data()),
                m_lineSize - 2
            };

            m_lineSize = 0;
            return true;
        }

        if (m_lineSize == kLineBufferSize) {
            setError("HTTP line exceeded maximum allowed size");
            return false;
        }
    }

    return false;
}

void JobHttpParser::resetMessage()
{
    m_state = State::StatusLine;
    m_bodyFraming = BodyFraming::None;

    m_response.clear();

    m_lineSize = 0;
    m_bodyBytesReceived = 0;

    m_contentLength = 0;
    m_chunkBytesRemaining = 0;

    m_httpMajor = 0;
    m_httpMinor = 0;

    m_hasContentLength = false;
}

void JobHttpParser::setError(std::string_view error)
{
    m_lastError.assign(error);
    m_state = State::Error;
}

} // namespace job::net