#pragma once

#include <cstddef>

#include <QByteArray>
#include <QString>

#include <sodium/crypto_generichash.h>
#include "qsodium_export.h"
class QSODIUM_EXPORT QSodiumHash {
public:
    QSodiumHash() = default;
    ~QSodiumHash() = default;

    [[nodiscard]] static QByteArray hashFile(const QString &filePath,
                                             std::size_t hashSize = crypto_generichash_BYTES,
                                             const QByteArray &key = QByteArray()) noexcept;

    [[nodiscard]] static QByteArray hashBuffer(const QByteArray &data,
                                               std::size_t hashSize = crypto_generichash_BYTES,
                                               const QByteArray &key = QByteArray()) noexcept;
};