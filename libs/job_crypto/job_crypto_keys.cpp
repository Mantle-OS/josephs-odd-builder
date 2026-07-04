#include "job_crypto_keys.h"

#include <cstring>
#include <iostream>

#include <sodium.h>
#include <sodium/crypto_kx.h>
#include <sodium/crypto_sign.h>
#include <sodium/utils.h>


#include "job_crypto_init.h"
#include "job_crypto_utils.h"

namespace job::crypto {


JobCryptoKeys::JobCryptoKeys()
{
    if (!JobCryptoInit::isInitialized() && !JobCryptoInit::initialize())
        return;

    // Default configuration fallback paths using system environment definitions
    std::filesystem::path baseConfig;
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME")) {
        baseConfig = std::filesystem::path(xdg) / "jobcrypto";
    } else if (const char *home = std::getenv("HOME")) {
        baseConfig = std::filesystem::path(home) / ".config" / "jobcrypto";
    } else {
        baseConfig = std::filesystem::current_path() / "config" / "jobcrypto";
    }
    m_keysDirs.push_back(baseConfig);
}

bool JobCryptoKeys::setPublicKey(const std::vector<unsigned char> &publicKeyBytes) noexcept
{
    if (publicKeyBytes.empty())
        return false;

    m_publicKey = std::string(reinterpret_cast<const char*>(publicKeyBytes.data()),
                              publicKeyBytes.size());
    return true;
}

void JobCryptoKeys::setPrivateKey(const JobSecureMem &privKey)
{
    m_privateKey.free();
    if (privKey.size() > 0) {
        if (!m_privateKey.allocate(privKey.size())) {
            std::cerr << "[jobcrypto::JobCryptoKeys] CRITICAL: Secure page allocation failed for private key layer.\n";
            m_validKeys = false;
            return;
        }
        std::memcpy(m_privateKey.data(), privKey.data(), privKey.size());
    }
    m_validKeys = (m_privateKey.size() > 0 && !m_publicKey.empty());
}

void JobCryptoKeys::addKeyDirectory(const std::filesystem::path &dir)
{
    for (const auto& path : m_keysDirs) {
        if (path == dir) return;
    }
    m_keysDirs.push_back(dir);
}

bool JobCryptoKeys::createKeys(KeyType type) noexcept
{
    m_validKeys = false;
    JobSecureMem pubTemp;
    if (type == KeyType::Exchange) {
        if (!m_privateKey.allocate(crypto_kx_SECRETKEYBYTES) || !pubTemp.allocate(crypto_kx_PUBLICKEYBYTES))
            return false;

        if (crypto_kx_keypair(pubTemp.data(), m_privateKey.data()) != 0)
            return false;

    } else if (type == KeyType::Sign) {
        if (!m_privateKey.allocate(crypto_sign_SECRETKEYBYTES) || !pubTemp.allocate(crypto_sign_PUBLICKEYBYTES))
            return false;

        if (crypto_sign_keypair(pubTemp.data(), m_privateKey.data()) != 0)
            return false;
    }

    m_publicKey = pubTemp.toBase64();
    m_validKeys = true;


    return true;
}

bool JobCryptoKeys::createSeedKeys(KeyType type, const JobSecureMem &seed) noexcept
{
    m_validKeys = false;
    JobSecureMem pubTemp;

    if (type == KeyType::Exchange) {
        if (seed.size() != crypto_kx_SEEDBYTES)
            return false;

        if (!m_privateKey.allocate(crypto_kx_SECRETKEYBYTES) || !pubTemp.allocate(crypto_kx_PUBLICKEYBYTES))
            return false;

        if (crypto_kx_seed_keypair(pubTemp.data(), m_privateKey.data(), seed.data()) != 0)
            return false;

    } else if (type == KeyType::Sign) {
        if (seed.size() != crypto_sign_SEEDBYTES)
            return false;

        if (!m_privateKey.allocate(crypto_sign_SECRETKEYBYTES) || !pubTemp.allocate(crypto_sign_PUBLICKEYBYTES))
            return false;

        if (crypto_sign_seed_keypair(pubTemp.data(), m_privateKey.data(), seed.data()) != 0)
            return false;
    }

    m_publicKey = pubTemp.toBase64();
    m_validKeys = true;
    return true;
}

bool JobCryptoKeys::createClientSessionKeys(JobSecureMem &rx, JobSecureMem &tx, const std::string &serverPublicKey) noexcept
{
    if (!m_validKeys || m_privateKey.size() != crypto_kx_SECRETKEYBYTES)
        return false;

    std::vector<unsigned char> serverPubBin;
    if (!crypto::utils::base64ToBin(serverPubBin, serverPublicKey) || serverPubBin.size() != crypto_kx_PUBLICKEYBYTES)
        return false;

    std::vector<unsigned char> clientPubBin;
    if (!crypto::utils::base64ToBin(clientPubBin, m_publicKey) || clientPubBin.size() != crypto_kx_PUBLICKEYBYTES)
        return false;

    if (!rx.allocate(crypto_kx_SESSIONKEYBYTES) || !tx.allocate(crypto_kx_SESSIONKEYBYTES)) {
        return false;
    }

    int const result = crypto_kx_client_session_keys(
        rx.data(), tx.data(),
        clientPubBin.data(), m_privateKey.data(), serverPubBin.data()
        );

    return (result == 0);
}

bool JobCryptoKeys::createServerSessionKeys(JobSecureMem &rx, JobSecureMem &tx, const std::string &clientPublicKey) noexcept
{
    if (!m_validKeys || m_privateKey.size() != crypto_kx_SECRETKEYBYTES)
        return false;

    std::vector<unsigned char> clientPubBin;
    if (!crypto::utils::base64ToBin(clientPubBin, clientPublicKey) || clientPubBin.size() != crypto_kx_PUBLICKEYBYTES)
        return false;

    std::vector<unsigned char> serverPubBin;
    if (!crypto::utils::base64ToBin(serverPubBin, m_publicKey) || serverPubBin.size() != crypto_kx_PUBLICKEYBYTES)
        return false;

    if (!rx.allocate(crypto_kx_SESSIONKEYBYTES) || !tx.allocate(crypto_kx_SESSIONKEYBYTES))
        return false;

    int const result = crypto_kx_server_session_keys(
        rx.data(), tx.data(),
        serverPubBin.data(), m_privateKey.data(), clientPubBin.data()
        );

    return (result == 0);
}

} // namespace job::crypto