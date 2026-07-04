#pragma once

#include <QByteArray>
#include <QString>

#include <sodium/crypto_generichash.h>

class QSodiumHash
{
public:
    QSodiumHash() = default;
    ~QSodiumHash() = default;

    [[nodiscard]] static QByteArray hashFile(const QString &filePath,
                                             size_t hashSize = crypto_generichash_BYTES,
                                             const QByteArray &key = QByteArray()) noexcept;

    [[nodiscard]] static QByteArray hashBuffer(const QByteArray &data,
                                               size_t hashSize = crypto_generichash_BYTES,
                                               const QByteArray &key = QByteArray()) noexcept;
};