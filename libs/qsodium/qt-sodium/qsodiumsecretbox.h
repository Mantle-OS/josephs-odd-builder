#pragma once

#include <QByteArray>

#include "qsecuremem.h"

class QSodiumSecretBox {
public:
    QSodiumSecretBox() = default;
    ~QSodiumSecretBox() = default;

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