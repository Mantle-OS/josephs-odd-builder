#ifndef QSODIUMSECRETBOX_H
#define QSODIUMSECRETBOX_H

#include <QByteArray>
#include "qsecuremem.h"
class QSodiumSecretBox {
public:
    static bool encrypt(const QByteArray &plainText, const QSecureMem &key,
                        QByteArray &outCipherText, QByteArray &outNonce) noexcept;

    static bool decrypt(const QByteArray &cipherText, const QSecureMem &key,
                        const QByteArray &nonce, QSecureMem &outPlainText) noexcept;

    static QByteArray generateNonce() noexcept;
};

#endif // QSODIUMSECRETBOX_H
