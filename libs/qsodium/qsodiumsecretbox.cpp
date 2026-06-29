#include "qsodiumsecretbox.h"
#include "qextrarandom.h"
#include "qsodiuminit.h"
#include <sodium.h>
#include <QDebug>

QByteArray QSodiumSecretBox::generateNonce() noexcept
{
    QByteArray nonce(crypto_secretbox_NONCEBYTES, '\0');
    QExtraRandom::secureBytes(reinterpret_cast<void*>(nonce.data()), nonce.size());
    return nonce;
}

bool QSodiumSecretBox::encrypt(const QByteArray &plainText, const QSecureMem &key,
                               QByteArray &outCipherText, QByteArray &outNonce) noexcept
{
    if (!QSodiumInit::isInitialized() && !QSodiumInit::initialize()) return false;

    if (key.size() != crypto_secretbox_KEYBYTES) {
        qWarning() << "[QSodiumSecretBox] Encryption aborted: Invalid key size envelope.";
        return false;
    }

    outNonce = generateNonce();
    outCipherText.resize(plainText.size() + crypto_secretbox_MACBYTES);

    int const result = crypto_secretbox_easy(
        reinterpret_cast<unsigned char*>(outCipherText.data()),
        reinterpret_cast<const unsigned char*>(plainText.constData()),
        plainText.size(),
        reinterpret_cast<const unsigned char*>(outNonce.constData()),
        key.data()
        );

    if (result != 0) {
        qWarning() << "[QSodiumSecretBox] Symmetric encryption loop failed.";
        outCipherText.clear();
        outNonce.clear();
        return false;
    }

    return true;
}

bool QSodiumSecretBox::decrypt(const QByteArray &cipherText, const QSecureMem &key,
                               const QByteArray &nonce, QSecureMem &outPlainText) noexcept
{
    if (!QSodiumInit::isInitialized() && !QSodiumInit::initialize()) return false;

    if (key.size() != crypto_secretbox_KEYBYTES) return false;
    if (nonce.size() != crypto_secretbox_NONCEBYTES) return false;
    if (cipherText.size() < crypto_secretbox_MACBYTES) return false;

    size_t const plainTextSize = cipherText.size() - crypto_secretbox_MACBYTES;

    if (!outPlainText.allocate(plainTextSize)) {
        qWarning() << "[QSodiumSecretBox] Failed to allocate kernel-locked target buffer for plaintext.";
        return false;
    }

    int const result = crypto_secretbox_open_easy(
        outPlainText.data(),
        reinterpret_cast<const unsigned char*>(cipherText.constData()),
        cipherText.size(),
        reinterpret_cast<const unsigned char*>(nonce.constData()),
        key.data()
        );

    if (result != 0) {
        qWarning() << "[QSodiumSecretBox] Decryption failed! The payload was corrupted or modified on disk.";
        outPlainText.free(); // Immediately wipe the failed tracking page
        return false;
    }

    return true;
}