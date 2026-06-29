#include "qzstdio.h"
#include <QDebug>
#include <zstd.h>

QZstdIO::QZstdIO(QIODevice* targetDevice, QObject* parent) :
    QIODevice{parent},
    m_targetDevice(targetDevice),
    m_compressionLevel(3),
    m_decompressionFinished(false)
{
}

QZstdIO::~QZstdIO()
{
    QZstdIO::close();
}

void QZstdIO::setCompressionLevel(int level)
{
    if (m_compressionLevel != level && !isOpen()) {
        if (level >= 0 && level <= 22)
            m_compressionLevel = level;
        else
            m_compressionLevel = 3;

        Q_EMIT compressionLevelChanged(m_compressionLevel);
    }
}

int QZstdIO::compressionLevel() const
{
    return m_compressionLevel;
}

bool QZstdIO::open(OpenMode mode)
{
    if (isOpen())
        return false;

    if (!(mode & ReadOnly) && !(mode & WriteOnly)) {
        setErrorString(QStringLiteral("Use ReadOnly or WriteOnly exclusively."));
        return false;
    }

    if (!m_targetDevice) {
        setErrorString(QStringLiteral("Target storage device is null."));
        return false;
    }

    if ((mode & ReadWrite) == ReadWrite || (mode & Append) || (mode & NewOnly)) {
        setErrorString(QStringLiteral("Unsupported open mode."));
        return false;
    }

    m_decompressionFinished = false;

    if (mode & ReadOnly) {

        if (!m_targetDevice->isOpen()) {
            if (!m_targetDevice->open(QIODevice::ReadOnly)) {
                setErrorString(m_targetDevice->errorString());
                return false;
            }
        }

        if (!m_targetDevice->isReadable()) {
            setErrorString(QStringLiteral("Target device is not readable."));
            return false;
        }

        if (!initDecompression())
            return false;
    } else if (mode & WriteOnly) {
        if (!m_targetDevice->isOpen()) {
            if (!m_targetDevice->open(QIODevice::WriteOnly)) {
                setErrorString(m_targetDevice->errorString());
                return false;
            }
        }

        if (!m_targetDevice->isWritable()) {
            setErrorString(QStringLiteral("Target device is not writable."));
            return false;
        }

        if (!initCompression())
            return false;
    }

    return QIODevice::open(mode);
}

void QZstdIO::close()
{
    if (!isOpen())
        return;

    // Flush routines flag explicit device errors gracefully on close pass ...Kinda
    if (openMode() & WriteOnly)
        if (!flushCompressionWindow())
            qWarning() << "[!] qzstd error: Failed to cleanly flush stream during explicit close phase.";

    if (m_cStream) {
        ZSTD_freeCStream(m_cStream);
        m_cStream = nullptr;
    }

    if (m_dStream) {
        ZSTD_freeDStream(m_dStream);
        m_dStream = nullptr;
    }

    m_inBuffer.clear();
    m_outBuffer.clear();
    m_inBufferPos = 0;
    m_inBufferSize = 0;
    m_decompressionFinished = false;

    QIODevice::close();
}

bool QZstdIO::initCompression()
{
    m_cStream = ZSTD_createCStream();
    if (!m_cStream)
        return false;

    size_t const initResult = ZSTD_initCStream(m_cStream, m_compressionLevel);

    if (ZSTD_isError(initResult)) {
        setErrorString(QString::fromUtf8(ZSTD_getErrorName(initResult)));
        ZSTD_freeCStream(m_cStream);
        m_cStream = nullptr;
        return false;
    }

    m_inBuffer.resize(static_cast<qsizetype>(ZSTD_CStreamInSize()));
    m_outBuffer.resize(static_cast<qsizetype>(ZSTD_CStreamOutSize()));

    return true;
}

bool QZstdIO::initDecompression()
{
    m_dStream = ZSTD_createDStream();
    if (!m_dStream)
        return false;

    size_t const initResult = ZSTD_initDStream(m_dStream);

    if (ZSTD_isError(initResult)) {
        setErrorString(QString::fromUtf8(ZSTD_getErrorName(initResult)));
        ZSTD_freeDStream(m_dStream);
        m_dStream = nullptr;
        return false;
    }

    m_inBuffer.resize(static_cast<qsizetype>(ZSTD_DStreamInSize()));
    m_outBuffer.resize(static_cast<qsizetype>(ZSTD_DStreamOutSize()));

    return true;
}

qint64 QZstdIO::readData(char *data, qint64 maxlen)
{
    if (maxlen <= 0 || !m_dStream || m_decompressionFinished)
        return 0;

    ZSTD_outBuffer output = {
        data,
        static_cast<size_t>(maxlen),
        0
    };

    while (output.pos < output.size) {
        if (m_inBufferPos >= m_inBufferSize) {
            if (fillInputBuffer() <= 0 && m_targetDevice->atEnd())
                break; // EOF
        }

        size_t const remainingInput = m_inBufferSize - m_inBufferPos;

        ZSTD_inBuffer input = {
            m_inBuffer.constData() + m_inBufferPos,
            remainingInput,
            0
        };

        size_t const decompressResult = ZSTD_decompressStream(m_dStream, &output, &input);
        m_inBufferPos += input.pos;

        if (ZSTD_isError(decompressResult)) {
            setErrorString(QString::fromUtf8(ZSTD_getErrorName(decompressResult)));
            ZSTD_freeDStream(m_dStream);
            m_dStream = nullptr;
            return -1;
        }

        // Set state flag for single-frame streaming layouts when frame concludes ?
        if (decompressResult == 0) {
            m_decompressionFinished = true;
            break;
        }
    }

    return static_cast<qint64>(output.pos);
}

qint64 QZstdIO::writeData(const char *data, qint64 len)
{
    if (len <= 0 || !m_cStream)
        return 0;

    ZSTD_inBuffer input = {
        data,
        static_cast<size_t>(len),
        0
    };

    while (input.pos < input.size) {

        ZSTD_outBuffer output = {
            m_outBuffer.data(),
            static_cast<size_t>(m_outBuffer.size()),
            0
        };

        size_t const compressResult = ZSTD_compressStream(m_cStream, &output, &input);

        if (ZSTD_isError(compressResult)) {
            setErrorString(QString::fromUtf8(ZSTD_getErrorName(compressResult)));
            ZSTD_freeCStream(m_cStream);
            m_cStream = nullptr;
            return -1;
        }

        if (output.pos > 0) {
            qint64 const written = m_targetDevice->write(
                m_outBuffer.constData(),
                static_cast<qint64>(output.pos)
                );

            if (written != static_cast<qint64>(output.pos)) {
                setErrorString(m_targetDevice->errorString());
                return -1;
            }
        }
    }

    return static_cast<qint64>(input.pos);
}

qint64 QZstdIO::fillInputBuffer()
{
    m_inBufferPos = 0;
    qint64 const bytesRead = m_targetDevice->read(
        m_inBuffer.data(),
        m_inBuffer.size()
    );

    m_inBufferSize = (bytesRead > 0) ?
                         static_cast<size_t>(bytesRead) :
                         0;
    return bytesRead;
}

bool QZstdIO::flushCompressionWindow()
{
    if (!m_cStream || !m_targetDevice)
        return false;

    size_t remaining = 0;
    do {
        ZSTD_outBuffer output = {
            m_outBuffer.data(),
            static_cast<size_t>(m_outBuffer.size()),
            0
        };

        remaining = ZSTD_endStream(m_cStream, &output);

        if (ZSTD_isError(remaining)) {
            setErrorString(QString::fromUtf8(ZSTD_getErrorName(remaining)));
            ZSTD_freeCStream(m_cStream);
            m_cStream = nullptr;
            return false;
        }

        if (output.pos > 0) {
            qint64 const written = m_targetDevice->write(
                m_outBuffer.constData(),
                static_cast<qint64>(output.pos)
            );

            if (written != static_cast<qint64>(output.pos)) {
                setErrorString(m_targetDevice->errorString());
                return false;
            }
        }
    } while (remaining > 0);

    return true;
}

qint64 QZstdIO::bytesAvailable() const
{
    return QIODevice::bytesAvailable();
}

qint64 QZstdIO::bytesToWrite() const
{
    // I dont trust this at all its fine for now but .....
    return 0;
}

bool QZstdIO::atEnd() const
{
    if (openMode() & WriteOnly)
        return QIODevice::atEnd();

    return m_decompressionFinished && (m_inBufferPos >= m_inBufferSize);
}




