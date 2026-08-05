#include "ssl_socket.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif
#include <security.h>
#include <schannel.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <utility>
#include <vector>

#include <job_logger.h>

namespace job::net {

namespace {

constexpr size_t DEFAULT_ENCRYPTED_BUFFER_SIZE = 16 * 1024;
constexpr size_t DEFAULT_PLAINTEXT_BUFFER_SIZE = 16 * 1024;

[[nodiscard]] bool isContinueStatus(SECURITY_STATUS status) noexcept
{
    return status == SEC_I_CONTINUE_NEEDED ||
           status == SEC_I_COMPLETE_NEEDED ||
           status == SEC_I_COMPLETE_AND_CONTINUE;
}

[[nodiscard]] bool needsCompleteAuthToken(SECURITY_STATUS status) noexcept
{
    return status == SEC_I_COMPLETE_NEEDED ||
           status == SEC_I_COMPLETE_AND_CONTINUE;
}

[[nodiscard]] bool isIncompleteMessage(SECURITY_STATUS status) noexcept
{
    return status == SEC_E_INCOMPLETE_MESSAGE;
}

} // namespace

struct SslSocket::Impl {
    CtxtHandle contextHandle{};
    bool hasContextHandle{false};

    SecPkgContext_StreamSizes streamSizes{};
    bool hasStreamSizes{false};

    std::vector<uint8_t> encryptedInput;
    std::vector<uint8_t> encryptedOutput;
    std::vector<uint8_t> plaintextBuffer;

    size_t encryptedInputSize{0};

    Impl()
    {
        encryptedInput.resize(DEFAULT_ENCRYPTED_BUFFER_SIZE);
        plaintextBuffer.resize(DEFAULT_PLAINTEXT_BUFFER_SIZE);
    }

    ~Impl()
    {
        release();
    }

    void release() noexcept
    {
        if (hasContextHandle) {
            ::DeleteSecurityContext(&contextHandle);
            hasContextHandle = false;
        }

        contextHandle = {};
        streamSizes = {};
        hasStreamSizes = false;

        encryptedInput.clear();
        encryptedOutput.clear();
        plaintextBuffer.clear();

        encryptedInputSize = 0;
    }

    [[nodiscard]] bool ensureEncryptedCapacity(size_t required)
    {
        if (required <= encryptedInput.size())
            return true;

        try {
            encryptedInput.resize(required);
        } catch (...) {
            return false;
        }

        return true;
    }
};
// DECL
SslSocket::SslSocket(PrivateTag, ISocketIO::Ptr socket, JobSslContext::Ptr context) :
    m_socket(std::move(socket)),
    m_context(std::move(context)),
    m_impl(std::make_unique<Impl>())
{
    m_errors.onLastError([this](JobSslError::SslErrNo error, const std::string &message) {
        if (onSslError)
            onSslError(error, message);
    });

    if (!m_socket) {
        recordSslError(JobSslError::SslErrNo::InvalidState, "SSL socket transport is invalid");
        return;
    }

    if (!m_context || !m_context->isValid()) {
        recordSslError(JobSslError::SslErrNo::InvalidState, "SSL context is invalid");
        return;
    }

    m_state.store(State::WaitingForTransport);
}

SslSocket::~SslSocket()
{
    detachSocketCallbacks();
    releaseSsl();

    if (m_socket)
        m_socket->disconnect();
}

bool SslSocket::setupSsl()
{
    if (!m_socket || m_socket->fd() < 0) {
        recordSslError(JobSslError::SslErrNo::InvalidState, "SSL transport descriptor is invalid");
        return false;
    }

    if (!m_context || !m_context->isValid()) {
        recordSslError(JobSslError::SslErrNo::InvalidState, "SSL context is invalid");
        return false;
    }

    if (!m_context->ensureCredentials()) {
        recordSslError(m_context->lastError(), m_context->lastErrorString());
        return false;
    }

    auto *credentials = static_cast<CredHandle *>(m_context->nativeHandle());

    if (!credentials) {
        recordSslError(JobSslError::SslErrNo::InvalidState, "Native Schannel credentials are invalid");
        return false;
    }

    if (!m_impl)
        m_impl = std::make_unique<Impl>();

    if (m_impl->encryptedInput.empty())
        m_impl->encryptedInput.resize(DEFAULT_ENCRYPTED_BUFFER_SIZE);

    if (m_impl->plaintextBuffer.empty())
        m_impl->plaintextBuffer.resize(DEFAULT_PLAINTEXT_BUFFER_SIZE);

    return true;
}

bool SslSocket::driveHandshake()
{
    if (!m_impl || !m_context || !m_socket) {
        recordSslError(JobSslError::SslErrNo::InvalidState, "Schannel handshake state is invalid");
        return false;
    }

    auto *credentials = static_cast<CredHandle *>(m_context->nativeHandle());

    if (!credentials) {
        recordSslError(JobSslError::SslErrNo::InvalidState, "Native Schannel credentials are invalid");
        return false;
    }

    const bool clientMode = m_context->mode() == JobSslContext::SslMode::Client;

    if (m_impl->hasContextHandle && m_impl->encryptedInputSize == 0) {
        const int64_t readResult = m_socket->read(
            m_impl->encryptedInput.data(),
            m_impl->encryptedInput.size()
            );

        if (readResult > 0) {
            m_impl->encryptedInputSize = static_cast<size_t>(readResult);
        } else if (readResult < 0) {
            return updateEvents(JobSslError::SslErrNo::WantRead);
        }
    }

    SecBuffer inputBuffers[2]{};
    SecBufferDesc inputDescriptor{};

    if (m_impl->encryptedInputSize > 0) {
        inputBuffers[0].BufferType = SECBUFFER_TOKEN;
        inputBuffers[0].pvBuffer = m_impl->encryptedInput.data();
        inputBuffers[0].cbBuffer = static_cast<unsigned long>(m_impl->encryptedInputSize);

        inputBuffers[1].BufferType = SECBUFFER_EMPTY;

        inputDescriptor.ulVersion = SECBUFFER_VERSION;
        inputDescriptor.cBuffers = 2;
        inputDescriptor.pBuffers = inputBuffers;
    }

    SecBuffer outputBuffer{};
    outputBuffer.BufferType = SECBUFFER_TOKEN;

    SecBufferDesc outputDescriptor{};
    outputDescriptor.ulVersion = SECBUFFER_VERSION;
    outputDescriptor.cBuffers = 1;
    outputDescriptor.pBuffers = &outputBuffer;

    ULONG attributes = 0;
    TimeStamp expiry{};

    constexpr ULONG clientRequirements =
        ISC_REQ_SEQUENCE_DETECT |
        ISC_REQ_REPLAY_DETECT |
        ISC_REQ_CONFIDENTIALITY |
        ISC_REQ_EXTENDED_ERROR |
        ISC_REQ_ALLOCATE_MEMORY |
        ISC_REQ_STREAM;

    ULONG serverRequirements =
        ASC_REQ_SEQUENCE_DETECT |
        ASC_REQ_REPLAY_DETECT |
        ASC_REQ_CONFIDENTIALITY |
        ASC_REQ_EXTENDED_ERROR |
        ASC_REQ_ALLOCATE_MEMORY |
        ASC_REQ_STREAM;

    if (m_context->verifyMode() == JobSslContext::VerifyMode::RequirePeer)
        serverRequirements |= ASC_REQ_MUTUAL_AUTH;

    SECURITY_STATUS status = SEC_E_INTERNAL_ERROR;

    if (clientMode) {
        const char *targetName = m_peerName.empty() ? nullptr : m_peerName.c_str();

        status = ::InitializeSecurityContextA(
            credentials,
            m_impl->hasContextHandle ? &m_impl->contextHandle : nullptr,
            const_cast<SEC_CHAR *>(targetName),
            clientRequirements,
            0,
            SECURITY_NATIVE_DREP,
            m_impl->encryptedInputSize > 0 ? &inputDescriptor : nullptr,
            0,
            &m_impl->contextHandle,
            &outputDescriptor,
            &attributes,
            &expiry
            );
    } else {
        if (m_impl->encryptedInputSize == 0)
            return updateEvents(JobSslError::SslErrNo::WantRead);

        status = ::AcceptSecurityContext(
            credentials,
            m_impl->hasContextHandle ? &m_impl->contextHandle : nullptr,
            &inputDescriptor,
            serverRequirements,
            SECURITY_NATIVE_DREP,
            &m_impl->contextHandle,
            &outputDescriptor,
            &attributes,
            &expiry
            );
    }

    if (status != SEC_E_INVALID_HANDLE)
        m_impl->hasContextHandle = true;

    if (needsCompleteAuthToken(status)) {
        const SECURITY_STATUS completeStatus = ::CompleteAuthToken(
            &m_impl->contextHandle,
            &outputDescriptor
            );

        if (completeStatus != SEC_E_OK) {
            m_errors.recordNativeError(static_cast<int>(completeStatus));
            recordSslError(m_errors.lastError(), m_errors.lastErrorString());

            if (outputBuffer.pvBuffer)
                ::FreeContextBuffer(outputBuffer.pvBuffer);

            return false;
        }
    }

    if (outputBuffer.pvBuffer && outputBuffer.cbBuffer > 0) {
        const auto outputSize = static_cast<size_t>(outputBuffer.cbBuffer);

        const int64_t written = m_socket->write(
            outputBuffer.pvBuffer,
            outputSize
            );

        ::FreeContextBuffer(outputBuffer.pvBuffer);
        outputBuffer.pvBuffer = nullptr;
        outputBuffer.cbBuffer = 0;

        if (written < 0)
            return updateEvents(JobSslError::SslErrNo::WantWrite);

        if (static_cast<size_t>(written) != outputSize) {
            recordSslError(JobSslError::SslErrNo::Syscall, "Incomplete Schannel handshake token write");
            return false;
        }
    }

    if (m_impl->encryptedInputSize > 0) {
        size_t extraBytes = 0;
        const uint8_t *extraData = nullptr;

        for (const auto &buffer : inputBuffers) {
            if (buffer.BufferType == SECBUFFER_EXTRA) {
                extraBytes = buffer.cbBuffer;
                extraData = static_cast<const uint8_t *>(buffer.pvBuffer);
                break;
            }
        }

        if (extraBytes > 0 && extraData) {
            std::memmove(
                m_impl->encryptedInput.data(),
                extraData,
                extraBytes
                );

            m_impl->encryptedInputSize = extraBytes;
        } else {
            m_impl->encryptedInputSize = 0;
        }
    }

    if (status == SEC_E_OK) {
        const SECURITY_STATUS streamStatus = ::QueryContextAttributesA(
            &m_impl->contextHandle,
            SECPKG_ATTR_STREAM_SIZES,
            &m_impl->streamSizes
            );

        if (streamStatus != SEC_E_OK) {
            m_errors.recordNativeError(static_cast<int>(streamStatus));
            recordSslError(m_errors.lastError(), m_errors.lastErrorString());
            return false;
        }

        m_impl->hasStreamSizes = true;

        const size_t encryptedOutputSize =
            static_cast<size_t>(m_impl->streamSizes.cbHeader) +
            static_cast<size_t>(m_impl->streamSizes.cbMaximumMessage) +
            static_cast<size_t>(m_impl->streamSizes.cbTrailer);

        m_impl->encryptedOutput.resize(encryptedOutputSize);
        m_impl->plaintextBuffer.resize(m_impl->streamSizes.cbMaximumMessage);

        if (!verifyPeer()) {
            m_state.store(State::Error);
            return false;
        }

        if (!m_socket->setEvents(
                threads::IOEvent::Read |
                threads::IOEvent::Error |
                threads::IOEvent::HangUp |
                threads::IOEvent::EdgeTriggered))
        {
            recordSslError(JobSslError::SslErrNo::InternalError, "Failed to update socket events after Schannel handshake");
            return false;
        }

        m_state.store(State::Encrypted);

        if (onEncrypted)
            onEncrypted();

        return true;
    }

    if (isIncompleteMessage(status))
        return updateEvents(JobSslError::SslErrNo::WantRead);

    if (isContinueStatus(status)) {
        if (m_impl->encryptedInputSize > 0)
            return driveHandshake();

        return updateEvents(JobSslError::SslErrNo::WantRead);
    }

    m_errors.recordNativeError(static_cast<int>(status));
    recordSslError(m_errors.lastError(), m_errors.lastErrorString());

    return false;
}

bool SslSocket::processSslError(int result)
{
    const auto status = static_cast<SECURITY_STATUS>(result);

    if (status == SEC_E_INCOMPLETE_MESSAGE)
        return updateEvents(JobSslError::SslErrNo::WantRead);

    if (status == SEC_I_CONTINUE_NEEDED)
        return updateEvents(JobSslError::SslErrNo::WantRead);

    if (status == SEC_E_OK)
        return true;

    m_errors.recordNativeError(result);
    m_state.store(State::Error);

    return false;
}

bool SslSocket::verifyPeer()
{
    if (!m_impl || !m_impl->hasContextHandle || !m_context)
        return false;

    if (m_context->verifyMode() == JobSslContext::VerifyMode::None)
        return true;

    PCCERT_CONTEXT peerCertificate = nullptr;

    const SECURITY_STATUS status = ::QueryContextAttributesA(
        &m_impl->contextHandle,
        SECPKG_ATTR_REMOTE_CERT_CONTEXT,
        &peerCertificate
        );

    if (status != SEC_E_OK || !peerCertificate) {
        m_errors.recordNativeError(static_cast<int>(status));
        recordSslError(JobSslError::SslErrNo::CertificateVerifyFailed, "TLS peer did not provide a certificate");
        return false;
    }

    ::CertFreeCertificateContext(peerCertificate);
    return true;
}

int64_t SslSocket::read(void *buffer, size_t size)
{
    if (!buffer || size == 0)
        return 0;

    if (m_state.load() != State::Encrypted ||
        !m_impl ||
        !m_impl->hasContextHandle)
    {
        recordSslError(JobSslError::SslErrNo::InvalidState, "SSL read requires an encrypted connection");
        return -1;
    }

    std::lock_guard<std::mutex> lock(m_readMutex);

    while (true) {
        if (m_impl->encryptedInputSize == 0) {
            const int64_t received = m_socket->read(
                m_impl->encryptedInput.data(),
                m_impl->encryptedInput.size()
                );

            if (received == 0)
                return 0;

            if (received < 0) {
                if (!updateEvents(JobSslError::SslErrNo::WantRead))
                    JOB_LOG_ERROR("[SslSocket] Failed to wait for encrypted Schannel input");

                return -1;
            }

            m_impl->encryptedInputSize = static_cast<size_t>(received);
        }

        SecBuffer buffers[4]{};

        buffers[0].BufferType = SECBUFFER_DATA;
        buffers[0].pvBuffer = m_impl->encryptedInput.data();
        buffers[0].cbBuffer = static_cast<unsigned long>(m_impl->encryptedInputSize);

        buffers[1].BufferType = SECBUFFER_EMPTY;
        buffers[2].BufferType = SECBUFFER_EMPTY;
        buffers[3].BufferType = SECBUFFER_EMPTY;

        SecBufferDesc descriptor{};
        descriptor.ulVersion = SECBUFFER_VERSION;
        descriptor.cBuffers = 4;
        descriptor.pBuffers = buffers;

        const SECURITY_STATUS status = ::DecryptMessage(
            &m_impl->contextHandle,
            &descriptor,
            0,
            nullptr
            );

        if (status == SEC_E_INCOMPLETE_MESSAGE) {
            const size_t oldSize = m_impl->encryptedInputSize;
            const size_t required = oldSize + DEFAULT_ENCRYPTED_BUFFER_SIZE;

            if (!m_impl->ensureEncryptedCapacity(required)) {
                recordSslError(JobSslError::SslErrNo::InternalError, "Failed to expand Schannel encrypted input buffer");
                return -1;
            }

            const int64_t received = m_socket->read(
                m_impl->encryptedInput.data() + oldSize,
                m_impl->encryptedInput.size() - oldSize
                );

            if (received <= 0) {
                if (!updateEvents(JobSslError::SslErrNo::WantRead))
                    JOB_LOG_ERROR("[SslSocket] Failed to wait for remaining Schannel record data");

                return -1;
            }

            m_impl->encryptedInputSize += static_cast<size_t>(received);
            continue;
        }

        if (status == SEC_I_CONTEXT_EXPIRED) {
            if (tryBeginShutdown()) {
                releaseSsl();

                if (m_socket)
                    m_socket->disconnect();
                else
                    m_state.store(State::Closed);
            }

            return 0;
        }

        if (status == SEC_I_RENEGOTIATE) {
            m_state.store(State::Handshaking);

            if (!driveHandshake())
                JOB_LOG_ERROR("[SslSocket] Failed to process Schannel renegotiation");

            return 0;
        }

        if (status != SEC_E_OK) {
            m_errors.recordNativeError(static_cast<int>(status));
            recordSslError(m_errors.lastError(), m_errors.lastErrorString());
            return -1;
        }

        const uint8_t *decryptedData = nullptr;
        size_t decryptedSize = 0;

        const uint8_t *extraData = nullptr;
        size_t extraSize = 0;

        for (const auto &item : buffers) {
            if (item.BufferType == SECBUFFER_DATA) {
                decryptedData = static_cast<const uint8_t *>(item.pvBuffer);
                decryptedSize = item.cbBuffer;
            } else if (item.BufferType == SECBUFFER_EXTRA) {
                extraData = static_cast<const uint8_t *>(item.pvBuffer);
                extraSize = item.cbBuffer;
            }
        }

        if (extraSize > 0 && extraData) {
            std::memmove(
                m_impl->encryptedInput.data(),
                extraData,
                extraSize
                );

            m_impl->encryptedInputSize = extraSize;
        } else {
            m_impl->encryptedInputSize = 0;
        }

        if (!decryptedData || decryptedSize == 0)
            return 0;

        const size_t copied = std::min(size, decryptedSize);
        std::memcpy(buffer, decryptedData, copied);

        return static_cast<int64_t>(copied);
    }
}

int64_t SslSocket::write(const void *buffer, size_t size)
{
    if (!buffer || size == 0)
        return 0;

    if (m_state.load() != State::Encrypted ||
        !m_impl ||
        !m_impl->hasContextHandle ||
        !m_impl->hasStreamSizes)
    {
        recordSslError(JobSslError::SslErrNo::InvalidState, "SSL write requires an encrypted connection");
        return -1;
    }

    std::lock_guard<std::mutex> lock(m_writeMutex);

    const size_t maximumMessage = m_impl->streamSizes.cbMaximumMessage;
    const size_t plaintextSize = std::min(size, maximumMessage);

    const size_t headerSize = m_impl->streamSizes.cbHeader;
    const size_t trailerSize = m_impl->streamSizes.cbTrailer;
    const size_t requiredSize = headerSize + plaintextSize + trailerSize;

    if (m_impl->encryptedOutput.size() < requiredSize)
        m_impl->encryptedOutput.resize(requiredSize);

    uint8_t *output = m_impl->encryptedOutput.data();

    std::memcpy(
        output + headerSize,
        buffer,
        plaintextSize
        );

    SecBuffer buffers[4]{};

    buffers[0].BufferType = SECBUFFER_STREAM_HEADER;
    buffers[0].pvBuffer = output;
    buffers[0].cbBuffer = static_cast<unsigned long>(headerSize);

    buffers[1].BufferType = SECBUFFER_DATA;
    buffers[1].pvBuffer = output + headerSize;
    buffers[1].cbBuffer = static_cast<unsigned long>(plaintextSize);

    buffers[2].BufferType = SECBUFFER_STREAM_TRAILER;
    buffers[2].pvBuffer = output + headerSize + plaintextSize;
    buffers[2].cbBuffer = static_cast<unsigned long>(trailerSize);

    buffers[3].BufferType = SECBUFFER_EMPTY;

    SecBufferDesc descriptor{};
    descriptor.ulVersion = SECBUFFER_VERSION;
    descriptor.cBuffers = 4;
    descriptor.pBuffers = buffers;

    const SECURITY_STATUS status = ::EncryptMessage(
        &m_impl->contextHandle,
        0,
        &descriptor,
        0
        );

    if (status != SEC_E_OK) {
        m_errors.recordNativeError(static_cast<int>(status));
        recordSslError(m_errors.lastError(), m_errors.lastErrorString());
        return -1;
    }

    const size_t encryptedSize =
        static_cast<size_t>(buffers[0].cbBuffer) +
        static_cast<size_t>(buffers[1].cbBuffer) +
        static_cast<size_t>(buffers[2].cbBuffer);

    const int64_t written = m_socket->write(
        output,
        encryptedSize
        );

    if (written < 0) {
        if (!updateEvents(JobSslError::SslErrNo::WantWrite))
            JOB_LOG_ERROR("[SslSocket] Failed to wait for Schannel encrypted output readiness");

        return -1;
    }

    if (static_cast<size_t>(written) != encryptedSize) {
        recordSslError(JobSslError::SslErrNo::Syscall, "Incomplete encrypted Schannel record write");
        return -1;
    }

    return static_cast<int64_t>(plaintextSize);
}

void SslSocket::disconnect()
{
    if (!tryBeginShutdown())
        return;

    if (!m_socket) {
        releaseSsl();
        m_state.store(State::Closed);
        return;
    }

    if (!m_impl || !m_impl->hasContextHandle) {
        releaseSsl();

        if (m_socket->state() == ISocketIO::SocketState::Unconnected) {
            m_state.store(State::Closed);
            return;
        }

        m_socket->disconnect();
        return;
    }

    shutdownSsl();
}

void SslSocket::shutdownSsl() noexcept
{
    if (state() != State::ShuttingDown)
        return;

    if (!m_impl || !m_impl->hasContextHandle) {
        if (m_socket)
            m_socket->disconnect();
        else
            m_state.store(State::Closed);

        return;
    }

    DWORD shutdownToken = SCHANNEL_SHUTDOWN;

    SecBuffer controlBuffer{};
    controlBuffer.BufferType = SECBUFFER_TOKEN;
    controlBuffer.pvBuffer = &shutdownToken;
    controlBuffer.cbBuffer = sizeof(shutdownToken);

    SecBufferDesc controlDescriptor{};
    controlDescriptor.ulVersion = SECBUFFER_VERSION;
    controlDescriptor.cBuffers = 1;
    controlDescriptor.pBuffers = &controlBuffer;

    SECURITY_STATUS status = ::ApplyControlToken(&m_impl->contextHandle, &controlDescriptor);

    if (status != SEC_E_OK) {
        m_errors.recordNativeError(static_cast<int>(status));
        JOB_LOG_ERROR("[SslSocket] Failed to apply Schannel shutdown token: {}", m_errors.lastErrorString());

        releaseSsl();
        m_socket->disconnect();
        return;
    }

    SecBuffer outputBuffer{};
    outputBuffer.BufferType = SECBUFFER_TOKEN;

    SecBufferDesc outputDescriptor{};
    outputDescriptor.ulVersion = SECBUFFER_VERSION;
    outputDescriptor.cBuffers = 1;
    outputDescriptor.pBuffers = &outputBuffer;

    ULONG attributes = 0;
    TimeStamp expiry{};

    if (m_context->mode() == JobSslContext::SslMode::Client) {
        auto *credentials = static_cast<CredHandle *>(m_context->nativeHandle());

        status = ::InitializeSecurityContextA(
            credentials,
            &m_impl->contextHandle,
            m_peerName.empty() ? nullptr : const_cast<SEC_CHAR *>(m_peerName.c_str()),
            ISC_REQ_SEQUENCE_DETECT |
                ISC_REQ_REPLAY_DETECT |
                ISC_REQ_CONFIDENTIALITY |
                ISC_REQ_EXTENDED_ERROR |
                ISC_REQ_ALLOCATE_MEMORY |
                ISC_REQ_STREAM,
            0,
            SECURITY_NATIVE_DREP,
            nullptr,
            0,
            &m_impl->contextHandle,
            &outputDescriptor,
            &attributes,
            &expiry
            );
    } else {
        status = ::AcceptSecurityContext(
            static_cast<CredHandle *>(m_context->nativeHandle()),
            &m_impl->contextHandle,
            nullptr,
            ASC_REQ_SEQUENCE_DETECT |
                ASC_REQ_REPLAY_DETECT |
                ASC_REQ_CONFIDENTIALITY |
                ASC_REQ_EXTENDED_ERROR |
                ASC_REQ_ALLOCATE_MEMORY |
                ASC_REQ_STREAM,
            SECURITY_NATIVE_DREP,
            &m_impl->contextHandle,
            &outputDescriptor,
            &attributes,
            &expiry
            );
    }

    if (status != SEC_E_OK && status != SEC_I_CONTEXT_EXPIRED && status != SEC_I_CONTINUE_NEEDED) {
        m_errors.recordNativeError(static_cast<int>(status));
        JOB_LOG_ERROR("[SslSocket] Failed to generate Schannel shutdown token: {}", m_errors.lastErrorString());
    }

    if (outputBuffer.pvBuffer && outputBuffer.cbBuffer > 0) {
        const int64_t written = m_socket->write(outputBuffer.pvBuffer, outputBuffer.cbBuffer);
        ::FreeContextBuffer(outputBuffer.pvBuffer);

        if (written != static_cast<int64_t>(outputBuffer.cbBuffer))
            JOB_LOG_WARN("[SslSocket] Schannel shutdown token was not written completely");
    }

    releaseSsl();
    m_socket->disconnect();
}

void SslSocket::releaseSsl() noexcept
{
    if (m_impl)
        m_impl->release();
}

} // namespace job::net