#include "job_secret_box.h"

#ifndef NDEBUG
#include <iostream>
#endif

#include <sodium.h>
#include <sodium/crypto_secretbox.h>

#include "job_crypto_init.h"
#include "job_random.h"

namespace job::crypto {

std::vector<unsigned char> JobSecretBox::generateNonce() noexcept
{
    std::vector<unsigned char> nonce(crypto_secretbox_NONCEBYTES);
    // primitive to pack secure noise rows
    JobRandom::secureBytes(nonce.data(), nonce.size());
    return nonce;
}

bool JobSecretBox::encrypt(const std::vector<unsigned char> &plainText,
                           const JobSecureMem &key,
                           std::vector<unsigned char> &outCipherText,
                           std::vector<unsigned char> &outNonce) noexcept
{
    if (!JobCryptoInit::isInitialized() && !JobCryptoInit::initialize())
        return false;

    if (key.size() != crypto_secretbox_KEYBYTES) {
#ifndef NDEBUG
        std::cerr << "[JobSecretBox] Encryption aborted: Invalid key size envelope.\n";
#endif
        return false;
    }

    outNonce = generateNonce();
    outCipherText.resize(plainText.size() + crypto_secretbox_MACBYTES);

    int const result = crypto_secretbox_easy(
        outCipherText.data(),
        plainText.data(),
        plainText.size(),
        outNonce.data(),
        key.data()
        );

    if (result != 0) {
#ifndef NDEBUG
        std::cerr << "[JobSecretBox] Symmetric encryption loop failed.\n";
#endif
        outCipherText.clear();
        outNonce.clear();
        // outCipherText = {};
        return false;
    }

    return true;
}

bool JobSecretBox::decrypt(const std::vector<unsigned char> &cipherText,
                           const JobSecureMem &key,
                           const std::vector<unsigned char> &nonce,
                           JobSecureMem &outPlainText) noexcept
{
    if (!JobCryptoInit::isInitialized() && !JobCryptoInit::initialize())
        return false;

    if (key.size() != crypto_secretbox_KEYBYTES)
        return false;

    if (nonce.size() != crypto_secretbox_NONCEBYTES)
        return false;

    if (cipherText.size() < crypto_secretbox_MACBYTES)
        return false;

    std::size_t const plainTextSize = cipherText.size() - crypto_secretbox_MACBYTES;

    // Alloc locked mem page buff
    if (outPlainText.size() != plainTextSize)
        outPlainText = JobSecureMem(plainTextSize);

    int const result = crypto_secretbox_open_easy(
        outPlainText.data(),
        cipherText.data(),
        cipherText.size(),
        nonce.data(),
        key.data()
        );

    if (result != 0) {
#ifndef NDEBUG
        std::cerr << "[JobSecretBox] Decryption failed! The payload was corrupted or modified on disk.\n";
#endif
        // immediately,
        outPlainText.clear();
        return false;
    }

    return true;
}

} // namespace job::crypto