#pragma once

#include <QIODevice>
#include <QByteArray>

#include <zstd.h>

typedef struct ZSTD_CCtx_s ZSTD_CCtx;
typedef struct ZSTD_DCtx_s ZSTD_DCtx;

class QZstdIO : public QIODevice
{
    Q_OBJECT
    Q_PROPERTY(int compressionLevel READ compressionLevel WRITE setCompressionLevel NOTIFY compressionLevelChanged FINAL)

public:
    explicit QZstdIO(QIODevice* targetDevice, QObject* parent = nullptr);
    ~QZstdIO() override;

    void setCompressionLevel(int level);
    [[nodiscard]] int compressionLevel() const;

    [[nodiscard]]  bool open(OpenMode mode) override;
    void close() override;

    [[nodiscard]] bool isSequential() const override { return true; }
    [[nodiscard]] qint64 bytesAvailable() const override;
    [[nodiscard]] qint64 bytesToWrite() const override;
    [[nodiscard]] bool atEnd() const override;

    // TODO I need to implement this do this after API and conncurent things get worked out
    // virtual bool waitForBytesWritten(int msecs = -1) override
    // virtual bool waitForReadyRead(int msecs = -1) override

Q_SIGNALS:
    void compressionLevelChanged(int level);

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

private:
    bool initCompression();
    bool initDecompression();

    bool flushCompressionWindow();
    qint64 fillInputBuffer();

private:
    QIODevice *m_targetDevice = nullptr;
    int m_compressionLevel = 3;

    // Opaque native streaming handles
    ZSTD_CStream *m_cStream = nullptr;
    ZSTD_DStream *m_dStream = nullptr;

    QByteArray m_inBuffer;
    QByteArray m_outBuffer;

    size_t m_inBufferPos = 0;
    size_t m_inBufferSize = 0; // Renamed from m_outBufferPos

    bool m_decompressionFinished = false;
};