#include "qsodiumsecretbox.h"

#include <job_secret_box.h>

#include <vector>

QByteArray QSodiumSecretBox::generateNonce() noexcept
{
    std::vector<unsigned char> const nativeNonce = job::crypto::JobSecretBox::generateNonce();

    return QByteArray(reinterpret_cast<const char*>(nativeNonce.data()),
                      static_cast<int>(nativeNonce.size())
                      );
}

bool QSodiumSecretBox::encrypt(const QByteArray &plainText, const QSecureMem &key,
                               QByteArray &outCipherText, QByteArray &outNonce) noexcept
{

    std::vector<unsigned char> const nativePlain(
        plainText.constData(),
        plainText.constData() + plainText.size()
        );

    std::vector<unsigned char> nativeCipher;
    std::vector<unsigned char> nativeNonce;

    if (!job::crypto::JobSecretBox::encrypt(nativePlain, key, nativeCipher, nativeNonce)) {
        outCipherText.clear();
        outNonce.clear();
        return false;
    }

    outCipherText = QByteArray(reinterpret_cast<const char*>(nativeCipher.data()),
                               static_cast<int>(nativeCipher.size()));

    outNonce = QByteArray(reinterpret_cast<const char*>(nativeNonce.data()),
                          static_cast<int>(nativeNonce.size())
                          );
    return true;
}

bool QSodiumSecretBox::decrypt(const QByteArray &cipherText, const QSecureMem &key,
                               const QByteArray &nonce, QSecureMem &outPlainText) noexcept
{
    // vectors << QByteArray's
    std::vector<unsigned char> const nativeCipher(
        cipherText.constData(),
        cipherText.constData() + cipherText.size()
        );

    std::vector<unsigned char> const nativeNonce(
        nonce.constData(),
        nonce.constData() + nonce.size()
        );

    return job::crypto::JobSecretBox::decrypt(
        nativeCipher,
        key,
        nativeNonce,
        outPlainText
        );
}