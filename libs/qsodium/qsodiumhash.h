#ifndef QSODIUMHASH_H
#define QSODIUMHASH_H

#include <QByteArray>
#include <QString>
#include <sodium.h>

class QSodiumHash
{
public:
    QSodiumHash() = delete;
    ~QSodiumHash() = delete;

    [[nodiscard]] static QByteArray hashFile(const QString &filePath,
                                             size_t hashSize = crypto_generichash_BYTES) noexcept;

    [[nodiscard]] static QByteArray hashBuffer(const QByteArray &data,
                                               size_t hashSize = crypto_generichash_BYTES) noexcept;

private:
    static constexpr size_t m_chunkSize = 65536; // 64 KB streaming bounds
};

#endif // QSODIUMHASH_H