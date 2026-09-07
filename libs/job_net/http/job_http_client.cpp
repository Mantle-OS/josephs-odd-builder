#include "job_http_client.h"

#include <utility>

#include <job_logger.h>

namespace job::net {

JobHttpClient::JobHttpClient(threads::JobIoAsyncThread::Ptr loop,
                             JobResolver::Ptr resolver,
                             JobSslContext::Ptr sslContext) :
    m_loop(std::move(loop)),
    m_resolver(std::move(resolver)),
    m_sslContext(std::move(sslContext))
{
}

JobHttpClient::~JobHttpClient()
{
    detachTransportCallbacks();
    closeTransport();
}

bool JobHttpClient::send(const JobHttpRequest &request)
{
    if (isBusy())
        return false;

    m_request = request;
    return beginRequest();
}

bool JobHttpClient::send(JobHttpRequest &&request)
{
    if (isBusy())
        return false;

    m_request = std::move(request);
    return beginRequest();
}

void JobHttpClient::cancel()
{
    if (!isBusy())
        return;

    detachTransportCallbacks();
    closeTransport();

    m_state = State::Idle;
    m_transport = Transport::None;

    m_requestData.clear();
    m_requestBytesSent = 0;

    m_parser.reset();
    m_lastError.clear();
}

void JobHttpClient::reset()
{
    detachTransportCallbacks();
    closeTransport();

    m_request.clear();
    m_parser.reset();

    m_state = State::Idle;
    m_transport = Transport::None;

    m_requestData.clear();
    m_requestBytesSent = 0;

    m_lastError.clear();
}

JobHttpClient::State JobHttpClient::state() const noexcept
{
    return m_state;
}

JobHttpClient::Transport JobHttpClient::transport() const noexcept
{
    return m_transport;
}

bool JobHttpClient::isBusy() const noexcept
{
    return m_state == State::Connecting ||
           m_state == State::Sending ||
           m_state == State::Receiving;
}

bool JobHttpClient::isComplete() const noexcept
{
    return m_state == State::Complete;
}

bool JobHttpClient::hasError() const noexcept
{
    return m_state == State::Error;
}

const std::string &JobHttpClient::lastError() const noexcept
{
    return m_lastError;
}

const JobHttpRequest &JobHttpClient::request() const noexcept
{
    return m_request;
}

const JobHttpResponse &JobHttpClient::response() const noexcept
{
    return m_parser.response();
}

void JobHttpClient::setBodyMode(JobHttpParser::BodyMode mode) noexcept
{
    m_parser.setBodyMode(mode);
}

JobHttpParser::BodyMode JobHttpClient::bodyMode() const noexcept
{
    return m_parser.bodyMode();
}

void JobHttpClient::setBodyCallback(JobHttpParser::BodyCallback callback)
{
    m_parser.setBodyCallback(std::move(callback));
}

void JobHttpClient::clearBodyCallback()
{
    m_parser.clearBodyCallback();
}

void JobHttpClient::setResolver(JobResolver::Ptr resolver)
{
    m_resolver = std::move(resolver);

    if (m_tcpClient)
        m_tcpClient->setResolver(m_resolver);

    if (m_sslClient)
        m_sslClient->setResolver(m_resolver);
}

JobResolver::Ptr JobHttpClient::resolver() const noexcept
{
    return m_resolver;
}

void JobHttpClient::setSslContext(JobSslContext::Ptr context)
{
    m_sslContext = std::move(context);

    if (m_sslClient)
        m_sslClient->setContext(m_sslContext);
}

JobSslContext::Ptr JobHttpClient::sslContext() const noexcept
{
    return m_sslContext;
}

bool JobHttpClient::beginRequest()
{
    m_lastError.clear();
    m_requestData.clear();
    m_requestBytesSent = 0;
    m_transport = Transport::None;

    m_parser.reset();
    m_parser.setRequestMethod(m_request.method());

    if (!m_request.isValid()) {
        fail("Invalid HTTP request");
        return false;
    }

    const JobUrl::Scheme scheme = m_request.url().scheme();

    if (scheme != JobUrl::Scheme::Http &&
        scheme != JobUrl::Scheme::Https) {
        fail("Unsupported HTTP URL scheme");
        return false;
    }

    if (!m_resolver) {
        fail("HTTP client requires a resolver");
        return false;
    }

    if (scheme == JobUrl::Scheme::Https &&
        !m_sslContext) {
        fail("HTTPS request requires an SSL context");
        return false;
    }

    if (!buildRequestData())
        return false;

    if (!createTransport())
        return false;

    m_state = State::Connecting;

    if (!connectTransport())
        return false;

    return true;
}

bool JobHttpClient::createTransport()
{
    detachTransportCallbacks();
    closeTransport();

    switch (m_request.url().scheme()) {
    case JobUrl::Scheme::Http:
        m_transport = Transport::Tcp;
        m_tcpClient = std::make_shared<TcpClient>(m_loop, m_resolver);
        setupTcpCallbacks();
        return true;

    case JobUrl::Scheme::Https:
        m_transport = Transport::Ssl;
        m_sslClient = std::make_shared<SslClient>(m_loop, m_sslContext, m_resolver);
        setupSslCallbacks();
        return true;

    default:
        fail("Unsupported HTTP transport");
        return false;
    }
}

bool JobHttpClient::connectTransport()
{
    switch (m_transport) {
    case Transport::Tcp:
        if (m_tcpClient && m_tcpClient->connectToHost(m_request.url()))
            return true;

        if (m_tcpClient)
            fail(m_tcpClient->lastErrorString());
        else
            fail("TCP transport unavailable");

        return false;

    case Transport::Ssl:
        if (m_sslClient && m_sslClient->connectToHost(m_request.url()))
            return true;

        if (m_sslClient)
            fail(m_sslClient->lastErrorString());
        else
            fail("SSL transport unavailable");

        return false;

    case Transport::None:
        break;
    }

    fail("No HTTP transport selected");
    return false;
}

bool JobHttpClient::buildRequestData()
{
    const JobUrl &url = m_request.url();
    const JobHttpHeader &headers = m_request.headers();

    const std::string_view method = m_request.methodString();

    if (method.empty()) {
        fail("HTTP request method is empty");
        return false;
    }

    std::string target = url.encodedPath();

    if (!url.query().empty()) {
        target.push_back('?');
        target += url.query();
    }

    m_requestData.clear();

    std::size_t reserveSize = method.size() +
                              1 +
                              target.size() +
                              sizeof(" HTTP/1.1\r\n") - 1 +
                              m_request.bodySize();

    for (const auto &field : headers)
        reserveSize += field.displayName.size() + 2 + field.value.size() + 2;

    if (!headers.contains("host"))
        reserveSize += url.host().size() + 32;

    m_requestData.reserve(reserveSize);

    m_requestData.append(method);
    m_requestData.push_back(' ');
    m_requestData.append(target);
    m_requestData.append(" HTTP/1.1\r\n");

    if (!headers.contains("host")) {
        m_requestData.append("Host: ");
        m_requestData.append(url.host());

        const uint16_t port = url.port();

        const bool defaultPort =
            (url.scheme() == JobUrl::Scheme::Http && (port == 0 || port == 80)) ||
            (url.scheme() == JobUrl::Scheme::Https && (port == 0 || port == 443));

        if (!defaultPort && port != 0) {
            m_requestData.push_back(':');
            m_requestData.append(std::to_string(port));
        }

        m_requestData.append("\r\n");
    }

    for (const auto &field : headers) {
        m_requestData.append(field.displayName);
        m_requestData.append(": ");
        m_requestData.append(field.value);
        m_requestData.append("\r\n");
    }

    m_requestData.append("\r\n");

    if (m_request.hasBody()) {
        const std::span<const std::byte> body = m_request.bodyView();

        m_requestData.append(
            reinterpret_cast<const char *>(body.data()),
            body.size());
    }

    return true;
}

bool JobHttpClient::sendPendingData()
{
    if (m_state != State::Sending)
        return false;

    while (m_requestBytesSent < m_requestData.size()) {
        const std::size_t remaining =
            m_requestData.size() - m_requestBytesSent;

        const void *data =
            m_requestData.data() + m_requestBytesSent;

        const NetIoResult result =
            sendTransport(data, remaining);

        switch (result.status) {
        case NetIoStatus::Ok:
            if (result.bytes == 0) {
                fail("HTTP transport reported a zero-byte successful write");
                return false;
            }

            if (result.bytes > remaining) {
                fail("HTTP transport reported an invalid write count");
                return false;
            }

            m_requestBytesSent += result.bytes;
            break;

        case NetIoStatus::WouldBlock:
            if (!setWriteInterest(true)) {
                fail("Failed to arm HTTP writable notification");
                return false;
            }

            return true;

        case NetIoStatus::Closed:
            fail("HTTP transport closed while sending request");
            return false;

        case NetIoStatus::Error:
            switch (m_transport) {
            case Transport::Tcp:
                fail(m_tcpClient ? m_tcpClient->lastErrorString() :
                         "TCP write failed");
                break;

            case Transport::Ssl:
                fail(m_sslClient ? m_sslClient->lastErrorString() :
                         "SSL write failed");
                break;

            case Transport::None:
                fail("HTTP write failed without an active transport");
                break;
            }

            return false;
        }
    }

    if (!setWriteInterest(false)) {
        fail("Failed to clear HTTP writable notification");
        return false;
    }

    m_state = State::Receiving;
    return true;
}

bool JobHttpClient::setWriteInterest(bool enable)
{
    switch (m_transport) {
    case Transport::Tcp:
        return m_tcpClient &&
               m_tcpClient->setWriteInterest(enable);

    case Transport::Ssl:
        return m_sslClient &&
               m_sslClient->setWriteInterest(enable);

    case Transport::None:
        return false;
    }

    return false;
}

NetIoResult JobHttpClient::sendTransport(const void *data, std::size_t size)
{
    switch (m_transport) {
    case Transport::Tcp:
        if (m_tcpClient)
            return m_tcpClient->send(data, size);

        break;

    case Transport::Ssl:
        if (m_sslClient)
            return m_sslClient->send(data, size);

        break;

    case Transport::None:
        break;
    }

    return NetIoResult::error();
}

void JobHttpClient::setupTcpCallbacks()
{
    if (!m_tcpClient)
        return;

    m_tcpClient->onConnect = [this]() {
        handleConnected();
    };

    m_tcpClient->onMessage = [this](const char *data, std::size_t size) {
        handleMessage(data, size);
    };

    m_tcpClient->onWritable = [this]() {
        handleWritable();
    };

    m_tcpClient->onDisconnect = [this]() {
        handleDisconnected();
    };

    m_tcpClient->onError = [this](int) {
        handleTransportError(
            m_tcpClient ? m_tcpClient->lastErrorString() :
                "TCP transport error");
    };
}

void JobHttpClient::setupSslCallbacks()
{
    if (!m_sslClient)
        return;

    /*
     * SslClient::onConnect is application-ready and fires after TLS encryption
     * completes, so it maps directly to JobHttpClient::handleConnected().
     */
    m_sslClient->onConnect = [this]() {
        handleConnected();
    };

    m_sslClient->onMessage = [this](const char *data, std::size_t size) {
        handleMessage(data, size);
    };

    m_sslClient->onWritable = [this]() {
        handleWritable();
    };

    m_sslClient->onDisconnect = [this]() {
        handleDisconnected();
    };

    m_sslClient->onSocketError = [this](int) {
        handleTransportError(
            m_sslClient ? m_sslClient->lastErrorString() :
                "SSL transport socket error");
    };

    m_sslClient->onSslError =
        [this](JobSslError::SslErrNo error,
               const std::string &message) {
            if (!JobSslError::isFatalSslError(error))
                return;

            handleTransportError(message);
        };
}

void JobHttpClient::detachTransportCallbacks() noexcept
{
    if (m_tcpClient) {
        m_tcpClient->onConnect = nullptr;
        m_tcpClient->onMessage = nullptr;
        m_tcpClient->onWritable = nullptr;
        m_tcpClient->onDisconnect = nullptr;
        m_tcpClient->onError = nullptr;
    }

    if (m_sslClient) {
        m_sslClient->onConnect = nullptr;
        m_sslClient->onEncrypted = nullptr;
        m_sslClient->onMessage = nullptr;
        m_sslClient->onWritable = nullptr;
        m_sslClient->onDisconnect = nullptr;
        m_sslClient->onSocketError = nullptr;
        m_sslClient->onSslError = nullptr;
    }
}

void JobHttpClient::closeTransport() noexcept
{
    if (m_tcpClient)
        m_tcpClient->disconnect();

    if (m_sslClient)
        m_sslClient->disconnect();

    m_tcpClient.reset();
    m_sslClient.reset();

    m_transport = Transport::None;
}

void JobHttpClient::handleConnected()
{
    if (m_state != State::Connecting)
        return;

    m_state = State::Sending;

    if (!sendPendingData())
        return;
}

void JobHttpClient::handleWritable()
{
    if (m_state != State::Sending)
        return;

    if (!setWriteInterest(false)) {
        fail("Failed to clear HTTP writable notification");
        return;
    }

    static_cast<void>(sendPendingData());
}

void JobHttpClient::handleMessage(const char *data, std::size_t size)
{
    if (m_state != State::Receiving)
        return;

    if (!data || size == 0)
        return;

    std::span<const std::byte> input{
        reinterpret_cast<const std::byte *>(data),
        size
    };

    const std::size_t bodyBefore =
        m_parser.bodyBytesReceived();

    const JobHttpParser::ParseResult result =
        m_parser.parse(input);

    const std::size_t bodyAfter =
        m_parser.bodyBytesReceived();

    if (onProgress && bodyAfter != bodyBefore) {
        onProgress(
            bodyAfter,
            m_parser.hasContentLength() ?
                m_parser.contentLength() :
                0,
            m_parser.hasContentLength());
    }

    switch (result) {
    case JobHttpParser::ParseResult::NeedMoreData:
        return;

    case JobHttpParser::ParseResult::Complete:
        if (!input.empty()) {
            JOB_LOG_WARN(
                "[JobHttpClient] {} unconsumed bytes remain after HTTP response",
                input.size());
        }

        completeResponse();
        return;

    case JobHttpParser::ParseResult::Error:
        fail(m_parser.lastError());
        return;
    }
}

void JobHttpClient::handleDisconnected()
{
    if (m_state == State::Idle ||
        m_state == State::Complete ||
        m_state == State::Error)
        return;

    if (m_state == State::Receiving) {
        const JobHttpParser::ParseResult result =
            m_parser.notifyEof();

        if (result == JobHttpParser::ParseResult::Complete) {
            completeResponse();
            return;
        }

        if (result == JobHttpParser::ParseResult::Error) {
            fail(m_parser.lastError());
            return;
        }
    }

    fail("HTTP transport disconnected before response completed");
}

void JobHttpClient::handleTransportError(std::string_view error)
{
    if (m_state == State::Complete ||
        m_state == State::Error)
        return;

    if (error.empty())
        error = "HTTP transport error";

    fail(error);
}

void JobHttpClient::completeResponse()
{
    if (m_state == State::Complete)
        return;

    m_state = State::Complete;

    static_cast<void>(setWriteInterest(false));

    detachTransportCallbacks();

    /*
     * First implementation deliberately closes after each response.
     * Connection reuse can be layered in later without changing parser or
     * partial-write semantics.
     */
    closeTransport();

    m_requestData.clear();
    m_requestBytesSent = 0;

    if (onResponse)
        onResponse(m_parser.response());
}

void JobHttpClient::fail(std::string_view error)
{
    if (m_state == State::Error)
        return;

    m_lastError.assign(error);
    m_state = State::Error;

    static_cast<void>(setWriteInterest(false));

    detachTransportCallbacks();
    closeTransport();

    m_requestData.clear();
    m_requestBytesSent = 0;

    if (onError)
        onError(m_lastError);
}

} // namespace job::net