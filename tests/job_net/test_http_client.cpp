#include "test_loop.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

#include <ctx/job_fifo_ctx.h>
#include <http/job_http_client.h>
#include <servers/tcp_server.h>

using namespace job::net;
using namespace std::chrono_literals;

static bool waitHttpUntil(const std::function<bool()> &predicate,
                          std::chrono::milliseconds timeout = 2000ms)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (!predicate()) {
        if (std::chrono::steady_clock::now() >= deadline)
            return false;

        std::this_thread::sleep_for(1ms);
    }

    return true;
}

TEST_CASE("JobHttpClient performs a local HTTP GET", "[job_net][http_client][usage][get]")
{
    TestLoop loop;

    JobFifoCtx resolverCtx(2);
    auto resolver = JobResolver::create(loop.loop, resolverCtx.pool);

    REQUIRE(resolver);

    auto server = std::make_shared<TcpServer>(loop.loop);

    std::atomic<bool> serverReceivedRequest{false};
    std::atomic<bool> responseReceived{false};
    std::atomic<bool> clientError{false};

    std::string requestData;

    server->onClientMessage = [&](TcpClient::Ptr client, const char *data, size_t size) {
        requestData.append(data, size);

        if (requestData.find("\r\n\r\n") == std::string::npos)
            return;

        serverReceivedRequest.store(true);

        static const std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 5\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n"
            "Hello";

        const NetIoResult result = client->send(response);

        REQUIRE(result.status == NetIoStatus::Ok);
        REQUIRE(result.bytes == response.size());
    };

    REQUIRE(server->start("127.0.0.1", 0));
    REQUIRE(server->port() > 0);

    auto client = JobHttpClient::createShared(loop.loop, resolver, nullptr);

    REQUIRE(client);

    client->onResponse = [&](const JobHttpResponse &response) {
        REQUIRE(response.code() == 200);
        REQUIRE(response.reasonPhrase() == "OK");
        REQUIRE(response.headers().value("content-type") == "text/plain");
        REQUIRE(response.bodySize() == 5);

        const auto body = response.bodyView();
        const std::string text(reinterpret_cast<const char *>(body.data()), body.size());

        REQUIRE(text == "Hello");

        responseReceived.store(true);
    };

    client->onError = [&](std::string_view) {
        clientError.store(true);
    };

    JobUrl url("http://127.0.0.1:" + std::to_string(server->port()) + "/hello");
    JobHttpRequest request(url);

    REQUIRE(client->send(request));

    REQUIRE(waitHttpUntil([&]() {
        return responseReceived.load() || clientError.load();
    }));

    REQUIRE(serverReceivedRequest.load());
    REQUIRE(responseReceived.load());
    REQUIRE_FALSE(clientError.load());

    REQUIRE(client->state() == JobHttpClient::State::Complete);
    REQUIRE(client->isComplete());
    REQUIRE_FALSE(client->hasError());

    REQUIRE(requestData.starts_with("GET /hello HTTP/1.1\r\n"));

    server->stop();
}

TEST_CASE("JobHttpClient serializes path query host and custom headers", "[job_net][http_client][usage][request]")
{
    TestLoop loop;

    JobFifoCtx resolverCtx(2);
    auto resolver = JobResolver::create(loop.loop, resolverCtx.pool);

    auto server = std::make_shared<TcpServer>(loop.loop);

    std::atomic<bool> complete{false};
    std::atomic<bool> failed{false};

    std::string requestData;

    server->onClientMessage = [&](TcpClient::Ptr client, const char *data, size_t size) {
        requestData.append(data, size);

        if (requestData.find("\r\n\r\n") == std::string::npos)
            return;

        static const std::string response =
            "HTTP/1.1 204 No Content\r\n"
            "\r\n";

        const NetIoResult result = client->send(response);

        if (result.status != NetIoStatus::Ok || result.bytes != response.size())
            failed.store(true);
    };

    REQUIRE(server->start("127.0.0.1", 0));

    auto client = JobHttpClient::createShared(loop.loop, resolver, nullptr);

    client->onResponse = [&](const JobHttpResponse &response) {
        if (response.code() == 204)
            complete.store(true);
    };

    client->onError = [&](std::string_view) {
        failed.store(true);
    };

    JobUrl url("http://127.0.0.1:" + std::to_string(server->port()) + "/api/items?q=hello%20world");

    JobHttpRequest request(HttpMethod::Get, url);

    REQUIRE(request.headers().append("Accept", "application/json"));
    REQUIRE(request.headers().append("X-Job-Test", "yes"));

    REQUIRE(client->send(request));

    REQUIRE(waitHttpUntil([&]() {
        return complete.load() || failed.load();
    }));

    REQUIRE_FALSE(failed.load());
    REQUIRE(complete.load());

    REQUIRE(requestData.find("GET /api/items?q=hello%20world HTTP/1.1\r\n") != std::string::npos);

    REQUIRE(requestData.find(
                "Host: 127.0.0.1:" + std::to_string(server->port()) + "\r\n") != std::string::npos);

    REQUIRE(requestData.find("Accept: application/json\r\n") != std::string::npos);
    REQUIRE(requestData.find("X-Job-Test: yes\r\n") != std::string::npos);

    server->stop();
}

TEST_CASE("JobHttpClient sends request body bytes", "[job_net][http_client][usage][post][body]")
{
    TestLoop loop;

    JobFifoCtx resolverCtx(2);
    auto resolver = JobResolver::create(loop.loop, resolverCtx.pool);

    auto server = std::make_shared<TcpServer>(loop.loop);

    const std::string body = "Hello from JOB";

    std::atomic<bool> complete{false};
    std::atomic<bool> failed{false};

    std::string requestData;

    server->onClientMessage = [&](TcpClient::Ptr client, const char *data, size_t size) {
        requestData.append(data, size);

        const std::size_t headerEnd = requestData.find("\r\n\r\n");

        if (headerEnd == std::string::npos)
            return;

        const std::size_t bodyOffset = headerEnd + 4;

        if (requestData.size() < bodyOffset + body.size())
            return;

        static const std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 2\r\n"
            "\r\n"
            "OK";

        const NetIoResult result = client->send(response);

        if (result.status != NetIoStatus::Ok || result.bytes != response.size())
            failed.store(true);
    };

    REQUIRE(server->start("127.0.0.1", 0));

    auto client = JobHttpClient::createShared(loop.loop, resolver, nullptr);

    client->onResponse = [&](const JobHttpResponse &response) {
        if (response.code() == 200)
            complete.store(true);
    };

    client->onError = [&](std::string_view) {
        failed.store(true);
    };

    JobUrl url("http://127.0.0.1:" + std::to_string(server->port()) + "/upload");

    JobHttpRequest request(HttpMethod::Post, url);

    request.setBody(std::span<const std::byte>{
        reinterpret_cast<const std::byte *>(body.data()),
        body.size()
    });

    REQUIRE(request.headers().append("Content-Length", std::to_string(body.size())));
    REQUIRE(request.headers().append("Content-Type", "text/plain"));

    REQUIRE(client->send(request));

    REQUIRE(waitHttpUntil([&]() {
        return complete.load() || failed.load();
    }));

    REQUIRE_FALSE(failed.load());
    REQUIRE(complete.load());

    REQUIRE(requestData.find("POST /upload HTTP/1.1\r\n") != std::string::npos);

    const std::size_t bodyOffset = requestData.find("\r\n\r\n") + 4;

    REQUIRE(requestData.substr(bodyOffset) == body);

    server->stop();
}

TEST_CASE("JobHttpClient reports response body progress", "[job_net][http_client][usage][progress]")
{
    TestLoop loop;

    JobFifoCtx resolverCtx(2);
    auto resolver = JobResolver::create(loop.loop, resolverCtx.pool);

    auto server = std::make_shared<TcpServer>(loop.loop);

    std::atomic<bool> complete{false};
    std::atomic<bool> failed{false};
    std::atomic<std::size_t> received{0};
    std::atomic<std::size_t> total{0};
    std::atomic<bool> totalKnown{false};

    server->onClientMessage = [&](TcpClient::Ptr client, const char *, size_t) {
        static const std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 11\r\n"
            "\r\n"
            "Hello World";

        const NetIoResult result = client->send(response);

        if (result.status != NetIoStatus::Ok || result.bytes != response.size())
            failed.store(true);
    };

    REQUIRE(server->start("127.0.0.1", 0));

    auto client = JobHttpClient::createShared(loop.loop, resolver, nullptr);

    client->onProgress = [&](std::size_t current,
                             std::size_t expected,
                             bool known) {
        received.store(current);
        total.store(expected);
        totalKnown.store(known);
    };

    client->onResponse = [&](const JobHttpResponse &) {
        complete.store(true);
    };

    client->onError = [&](std::string_view) {
        failed.store(true);
    };

    JobHttpRequest request(
        JobUrl("http://127.0.0.1:" + std::to_string(server->port()) + "/"));

    REQUIRE(client->send(request));

    REQUIRE(waitHttpUntil([&]() {
        return complete.load() || failed.load();
    }));

    REQUIRE_FALSE(failed.load());
    REQUIRE(complete.load());

    REQUIRE(received.load() == 11);
    REQUIRE(total.load() == 11);
    REQUIRE(totalKnown.load());

    server->stop();
}

/*
 * Block two: edge cases and failure behavior
 */

TEST_CASE("JobHttpClient rejects invalid requests", "[job_net][http_client][edge][request]")
{
    TestLoop loop;

    JobFifoCtx resolverCtx(2);
    auto resolver = JobResolver::create(loop.loop, resolverCtx.pool);

    auto client = JobHttpClient::createShared(loop.loop, resolver, nullptr);

    REQUIRE(client);

    JobHttpRequest request;

    REQUIRE_FALSE(request.isValid());
    REQUIRE_FALSE(client->send(request));

    REQUIRE(client->hasError());
    REQUIRE(client->state() == JobHttpClient::State::Error);
    REQUIRE_FALSE(client->lastError().empty());
}

TEST_CASE("JobHttpClient requires a resolver", "[job_net][http_client][edge][resolver]")
{
    TestLoop loop;

    auto client = JobHttpClient::createShared(loop.loop, nullptr, nullptr);

    REQUIRE(client);

    JobHttpRequest request(JobUrl("http://127.0.0.1:12345/"));

    REQUIRE(request.isValid());
    REQUIRE_FALSE(client->send(request));

    REQUIRE(client->hasError());
    REQUIRE_FALSE(client->lastError().empty());
}

TEST_CASE("JobHttpClient rejects unsupported URL schemes", "[job_net][http_client][edge][scheme]")
{
    TestLoop loop;

    JobFifoCtx resolverCtx(2);
    auto resolver = JobResolver::create(loop.loop, resolverCtx.pool);

    auto client = JobHttpClient::createShared(loop.loop, resolver, nullptr);

    JobHttpRequest request(JobUrl("tcp://127.0.0.1:12345"));

    REQUIRE(request.isValid());
    REQUIRE_FALSE(client->send(request));

    REQUIRE(client->hasError());
    REQUIRE_FALSE(client->lastError().empty());
}

TEST_CASE("JobHttpClient cancel during streamed response stops further delivery",
          "[job_net][http_client][edge][cancel][stream]")
{
    TestLoop loop;

    JobFifoCtx resolverCtx(2);
    auto resolver = JobResolver::create(loop.loop, resolverCtx.pool);

    REQUIRE(resolver);

    auto server = std::make_shared<TcpServer>(loop.loop);

    std::atomic<bool> firstChunkSent{false};
    std::atomic<bool> secondChunkSent{false};

    server->onClientMessage = [&](TcpClient::Ptr client, const char *, std::size_t) {
        static const std::string headers =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 12\r\n"
            "\r\n";

        static const std::string first = "Hello ";
        static const std::string second = "World!";

        REQUIRE(client->send(headers).ok());
        REQUIRE(client->send(first).ok());

        firstChunkSent.store(true);

        std::this_thread::sleep_for(50ms);

        const NetIoResult result = client->send(second);

        if (result.ok())
            secondChunkSent.store(true);
    };

    REQUIRE(server->start("127.0.0.1", 0));
    REQUIRE(server->port() != 0);

    auto client = JobHttpClient::createShared(loop.loop, resolver, nullptr);

    REQUIRE(client);

    std::atomic<bool> callbackCalled{false};
    std::atomic<bool> responseCalled{false};
    std::atomic<bool> errorCalled{false};

    std::size_t streamedBytes = 0;

    client->setBodyMode(JobHttpParser::BodyMode::Stream);

    client->setBodyCallback([&](std::span<const std::byte> data) {
        streamedBytes += data.size();
        callbackCalled.store(true);

        client->cancel();
    });

    client->onResponse = [&](const JobHttpResponse &) {
        responseCalled.store(true);
    };

    client->onError = [&](std::string_view) {
        errorCalled.store(true);
    };

    JobHttpRequest request(
        JobUrl("http://127.0.0.1:" + std::to_string(server->port()) + "/stream"));

    REQUIRE(client->send(request));

    REQUIRE(waitHttpUntil([&]() {
        return callbackCalled.load();
    }));

    REQUIRE(firstChunkSent.load());

    REQUIRE(waitHttpUntil([&]() {
        return client->state() == JobHttpClient::State::Idle;
    }));

    REQUIRE(client->state() == JobHttpClient::State::Idle);
    REQUIRE_FALSE(client->isBusy());
    REQUIRE_FALSE(client->isComplete());
    REQUIRE_FALSE(client->hasError());

    REQUIRE(streamedBytes > 0);
    REQUIRE_FALSE(responseCalled.load());
    REQUIRE_FALSE(errorCalled.load());

    server->stop();
}

TEST_CASE("JobHttpClient reports refused connections", "[job_net][http_client][edge][connect][failure]")
{
    TestLoop loop;

    JobFifoCtx resolverCtx(2);
    auto resolver = JobResolver::create(loop.loop, resolverCtx.pool);

    REQUIRE(resolver);

    auto server = std::make_shared<TcpServer>(loop.loop);

    REQUIRE(server->start("127.0.0.1", 0));

    const std::uint16_t port = server->port();

    REQUIRE(port != 0);

    server->stop();

    auto client = JobHttpClient::createShared(loop.loop, resolver, nullptr);

    REQUIRE(client);

    std::atomic<bool> failed{false};

    client->onError = [&](std::string_view) {
        failed.store(true);
    };

    JobHttpRequest request(
        JobUrl("http://127.0.0.1:" + std::to_string(port) + "/"));

    REQUIRE(client->send(request));

    REQUIRE(waitHttpUntil([&]() {
        return failed.load();
    }));

    REQUIRE(client->hasError());
    REQUIRE_FALSE(client->isComplete());
    REQUIRE_FALSE(client->lastError().empty());
}

TEST_CASE("JobHttpClient allows only one active request", "[job_net][http_client][edge][busy]")
{
    TestLoop loop;

    JobFifoCtx resolverCtx(2);
    auto resolver = JobResolver::create(loop.loop, resolverCtx.pool);

    auto server = std::make_shared<TcpServer>(loop.loop);

    REQUIRE(server->start("127.0.0.1", 0));

    auto client = JobHttpClient::createShared(loop.loop, resolver, nullptr);

    JobHttpRequest first(
        JobUrl("http://127.0.0.1:" + std::to_string(server->port()) + "/first"));

    JobHttpRequest second(
        JobUrl("http://127.0.0.1:" + std::to_string(server->port()) + "/second"));

    REQUIRE(client->send(first));
    REQUIRE(client->isBusy());

    REQUIRE_FALSE(client->send(second));

    client->cancel();

    REQUIRE_FALSE(client->isBusy());
    REQUIRE(client->state() == JobHttpClient::State::Idle);

    server->stop();
}

TEST_CASE("JobHttpClient reset returns completed client to idle state", "[job_net][http_client][usage][reset]")
{
    TestLoop loop;

    JobFifoCtx resolverCtx(2);
    auto resolver = JobResolver::create(loop.loop, resolverCtx.pool);

    auto server = std::make_shared<TcpServer>(loop.loop);

    std::atomic<bool> complete{false};

    server->onClientMessage = [&](TcpClient::Ptr client, const char *, size_t) {
        static const std::string response =
            "HTTP/1.1 204 No Content\r\n"
            "\r\n";

        static_cast<void>(client->send(response));
    };

    REQUIRE(server->start("127.0.0.1", 0));

    auto client = JobHttpClient::createShared(loop.loop, resolver, nullptr);

    client->onResponse = [&](const JobHttpResponse &) {
        complete.store(true);
    };

    JobHttpRequest request(
        JobUrl("http://127.0.0.1:" + std::to_string(server->port()) + "/"));

    REQUIRE(client->send(request));

    REQUIRE(waitHttpUntil([&]() {
        return complete.load();
    }));

    REQUIRE(client->isComplete());

    client->reset();

    REQUIRE(client->state() == JobHttpClient::State::Idle);
    REQUIRE_FALSE(client->isBusy());
    REQUIRE_FALSE(client->isComplete());
    REQUIRE_FALSE(client->hasError());

    REQUIRE(client->lastError().empty());

    server->stop();
}


#ifndef JOB_CI_BUILD

TEST_CASE("JobHttpClient performs an external HTTPS request and receives the reply",
          "[job_net][http_client][integration][network][https]")
{
    TestLoop loop;

    JobFifoCtx resolverCtx(2);
    auto resolver = JobResolver::create(loop.loop, resolverCtx.pool);

    REQUIRE(resolver);

    auto sslContext = std::make_shared<JobSslContext>(JobSslContext::SslMode::Client);

    REQUIRE(sslContext);
    REQUIRE(sslContext->isValid());

    sslContext->setVerifyMode(JobSslContext::VerifyMode::Peer);

    REQUIRE(sslContext->loadSystemCertificates());

    auto client = JobHttpClient::createShared(loop.loop, resolver, sslContext);

    REQUIRE(client);

    std::atomic<bool> complete{false};
    std::atomic<bool> failed{false};

    JobHttpResponse receivedResponse;
    std::string error;

    client->onResponse = [&](const JobHttpResponse &response) {
        receivedResponse = response;
        complete.store(true);
    };

    client->onError = [&](std::string_view message) {
        error.assign(message);
        failed.store(true);
    };

    JobHttpRequest request(
        JobUrl("https://postman-echo.com/get?source=job_net&message=hello"));

    REQUIRE(request.headers().append("User-Agent", "JOB/job_net"));
    REQUIRE(request.headers().append("X-Job-Test", "outside-world"));

    const bool sent = client->send(request);

    INFO("JobHttpClient send error: " << client->lastError());

    REQUIRE(sent);

    REQUIRE(waitHttpUntil([&]() {
        return complete.load() || failed.load();
    }, 10000ms));

    INFO("External HTTP error: " << error);

    REQUIRE_FALSE(failed.load());
    REQUIRE(complete.load());

    REQUIRE(receivedResponse.isValid());
    REQUIRE(receivedResponse.code() == 200);
    REQUIRE(receivedResponse.isSuccessful());

    REQUIRE_FALSE(receivedResponse.headers().isEmpty());
    REQUIRE(receivedResponse.hasBody());
    REQUIRE(receivedResponse.bodySize() > 0);

    const auto body = receivedResponse.bodyView();
    const std::string text(reinterpret_cast<const char *>(body.data()), body.size());

    INFO("External HTTP response: " << text);

    REQUIRE(text.find("job_net") != std::string::npos);
    REQUIRE(text.find("hello") != std::string::npos);
    REQUIRE(text.find("outside-world") != std::string::npos);
    REQUIRE(text.find("JOB/job_net") != std::string::npos);
}


TEST_CASE("JobHttpClient performs an external HTTPS POST and receives the echoed body",
          "[job_net][http_client][integration][network][https][post]")
{
    TestLoop loop;

    JobFifoCtx resolverCtx(2);
    auto resolver = JobResolver::create(loop.loop, resolverCtx.pool);

    REQUIRE(resolver);

    auto sslContext = std::make_shared<JobSslContext>(JobSslContext::SslMode::Client);

    REQUIRE(sslContext);
    REQUIRE(sslContext->isValid());

    sslContext->setVerifyMode(JobSslContext::VerifyMode::Peer);

    REQUIRE(sslContext->loadSystemCertificates());

    auto client = JobHttpClient::createShared(loop.loop, resolver, sslContext);

    REQUIRE(client);

    std::atomic<bool> complete{false};
    std::atomic<bool> failed{false};

    JobHttpResponse receivedResponse;
    std::string error;

    client->onResponse = [&](const JobHttpResponse &response) {
        receivedResponse = response;
        complete.store(true);
    };

    client->onError = [&](std::string_view message) {
        error.assign(message);
        failed.store(true);
    };

    static constexpr std::string_view payload = R"({"source":"job_net","message":"hello from JOB","value":42})";

    JobHttpRequest request(HttpMethod::Post,
                           JobUrl("https://postman-echo.com/post"));

    request.setBody(std::span<const std::byte>{
        reinterpret_cast<const std::byte *>(payload.data()),
        payload.size()
    });

    REQUIRE(request.headers().append("User-Agent", "JOB/job_net"));
    REQUIRE(request.headers().append("Content-Type", "application/json"));
    REQUIRE(request.headers().append("X-Job-Test", "outside-world-post"));
    REQUIRE(request.headers().append("Content-Length", std::to_string(payload.size())));

    const bool sent = client->send(request);

    INFO("JobHttpClient send error: " << client->lastError());

    REQUIRE(sent);

    REQUIRE(waitHttpUntil([&]() {
        return complete.load() || failed.load();
    }, 10000ms));

    INFO("External HTTP error: " << error);

    REQUIRE_FALSE(failed.load());
    REQUIRE(complete.load());

    REQUIRE(receivedResponse.isValid());
    REQUIRE(receivedResponse.code() == 200);
    REQUIRE(receivedResponse.isSuccessful());

    REQUIRE_FALSE(receivedResponse.headers().isEmpty());
    REQUIRE(receivedResponse.hasBody());
    REQUIRE(receivedResponse.bodySize() > 0);

    const auto body = receivedResponse.bodyView();
    const std::string text(reinterpret_cast<const char *>(body.data()), body.size());

    WARN("External HTTP response: " << text);

    REQUIRE(text.find("job_net") != std::string::npos);
    REQUIRE(text.find("hello from JOB") != std::string::npos);
    REQUIRE(text.find("42") != std::string::npos);
    REQUIRE(text.find("outside-world-post") != std::string::npos);
    REQUIRE(text.find("application/json") != std::string::npos);
}

TEST_CASE("JobHttpClient receives an external HTTPS redirect without following it",
          "[job_net][http_client][integration][network][https][redirect]")
{
    TestLoop loop;

    JobFifoCtx resolverCtx(2);
    auto resolver = JobResolver::create(loop.loop, resolverCtx.pool);

    REQUIRE(resolver);

    auto sslContext = std::make_shared<JobSslContext>(JobSslContext::SslMode::Client);

    REQUIRE(sslContext);
    REQUIRE(sslContext->isValid());

    sslContext->setVerifyMode(JobSslContext::VerifyMode::Peer);

    REQUIRE(sslContext->loadSystemCertificates());

    auto client = JobHttpClient::createShared(loop.loop, resolver, sslContext);

    REQUIRE(client);

    std::atomic<bool> complete{false};
    std::atomic<bool> failed{false};

    JobHttpResponse receivedResponse;
    std::string error;

    client->onResponse = [&](const JobHttpResponse &response) {
        receivedResponse = response;
        complete.store(true);
    };

    client->onError = [&](std::string_view message) {
        error.assign(message);
        failed.store(true);
    };

    JobHttpRequest request(JobUrl("https://httpbin.org/redirect-to?url=https%3A%2F%2Fhttpbin.org%2Fget"));

    REQUIRE(request.headers().append("User-Agent", "JOB/job_net"));

    const bool sent = client->send(request);

    INFO("JobHttpClient send error: " << client->lastError());

    REQUIRE(sent);

    REQUIRE(waitHttpUntil([&]() {
        return complete.load() || failed.load();
    }, 10000ms));

    INFO("External HTTP error: " << error);

    REQUIRE_FALSE(failed.load());
    REQUIRE(complete.load());

    REQUIRE(receivedResponse.isValid());
    REQUIRE(receivedResponse.code() == 302);
    REQUIRE(receivedResponse.isRedirect());

    REQUIRE(receivedResponse.headers().contains("location"));

    const std::string_view location = receivedResponse.headers().value("location");
    WARN("Redirect location: " << location);

    REQUIRE_FALSE(location.empty());
    REQUIRE(location.find("httpbin.org") != std::string_view::npos);
    REQUIRE(location.find("/get") != std::string_view::npos);

    REQUIRE(client->isComplete());
    REQUIRE_FALSE(client->hasError());
}

TEST_CASE("JobHttpClient preserves external 307 POST redirect information",
          "[job_net][http_client][integration][network][https][redirect][307]")
{
    TestLoop loop;

    JobFifoCtx resolverCtx(2);
    auto resolver = JobResolver::create(loop.loop, resolverCtx.pool);

    REQUIRE(resolver);

    auto sslContext = std::make_shared<JobSslContext>(JobSslContext::SslMode::Client);

    REQUIRE(sslContext);
    REQUIRE(sslContext->isValid());

    sslContext->setVerifyMode(JobSslContext::VerifyMode::Peer);

    REQUIRE(sslContext->loadSystemCertificates());

    auto client = JobHttpClient::createShared(loop.loop, resolver, sslContext);

    REQUIRE(client);

    std::atomic<bool> complete{false};
    std::atomic<bool> failed{false};

    JobHttpResponse receivedResponse;
    std::string error;

    client->onResponse = [&](const JobHttpResponse &response) {
        receivedResponse = response;
        complete.store(true);
    };

    client->onError = [&](std::string_view message) {
        error.assign(message);
        failed.store(true);
    };

    static constexpr std::string_view payload =
        R"({"source":"job_net","redirect":307,"message":"preserve me"})";

    JobHttpRequest request(
        HttpMethod::Post,
        JobUrl(
            "https://httpbin.org/redirect-to?"
            "url=https%3A%2F%2Fhttpbin.org%2Fpost&"
            "status_code=307"));

    request.setBody(std::span<const std::byte>{
        reinterpret_cast<const std::byte *>(payload.data()),
        payload.size()
    });

    REQUIRE(request.headers().append("User-Agent", "JOB/job_net"));
    REQUIRE(request.headers().append("Content-Type", "application/json"));
    REQUIRE(request.headers().append("X-Job-Test", "outside-world-307"));
    REQUIRE(request.headers().append("Content-Length", std::to_string(payload.size())));

    const bool sent = client->send(request);

    INFO("JobHttpClient send error: " << client->lastError());

    REQUIRE(sent);

    REQUIRE(waitHttpUntil([&]() {
        return complete.load() || failed.load();
    }, 10000ms));

    INFO("External HTTP error: " << error);

    REQUIRE_FALSE(failed.load());
    REQUIRE(complete.load());

    REQUIRE(receivedResponse.isValid());
    REQUIRE(receivedResponse.code() == 307);
    REQUIRE(receivedResponse.isRedirect());

    REQUIRE(receivedResponse.headers().contains("location"));

    const std::string_view location =
        receivedResponse.headers().value("location");

    INFO("Redirect location: " << location);

    REQUIRE_FALSE(location.empty());
    REQUIRE(location.find("httpbin.org") != std::string_view::npos);
    REQUIRE(location.find("/post") != std::string_view::npos);

    REQUIRE(client->request().method() == HttpMethod::Post);
    REQUIRE(client->request().methodString() == "POST");

    REQUIRE(client->request().bodySize() == payload.size());

    const auto requestBody = client->request().bodyView();

    REQUIRE(requestBody.size() == payload.size());

    const std::string_view requestPayload(
        reinterpret_cast<const char *>(requestBody.data()),
        requestBody.size());

    REQUIRE(requestPayload == payload);

    REQUIRE(client->request().headers().value("content-type") == "application/json");
    REQUIRE(client->request().headers().value("x-job-test") == "outside-world-307");

    REQUIRE(client->isComplete());
    REQUIRE_FALSE(client->hasError());
}

#endif

