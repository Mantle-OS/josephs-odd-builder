#pragma once

#include <QByteArray>

#include "qsecuremem.h"
#include "qsodium_export.h"
class QSODIUM_EXPORT QSodiumSecretBox {
public:
    QSodiumSecretBox() = default;
    ~QSodiumSecretBox() = default;
    // [[BACKLOG]] Fab 5 later if need be
    [[nodiscard]] static bool encrypt(const QByteArray &plainText,
                                      const QSecureMem &key,
                                      QByteArray &outCipherText,
                                      QByteArray &outNonce) noexcept;

    [[nodiscard]] static bool decrypt(const QByteArray &cipherText,
                                      const QSecureMem &key,
                                      const QByteArray &nonce,
                                      QSecureMem &outPlainText) noexcept;

    [[nodiscard]] static QByteArray generateNonce() noexcept;
};