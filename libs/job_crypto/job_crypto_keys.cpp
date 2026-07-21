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

std::string JobCryptoKeys::publicKeyData(const std::filesystem::path &pub) noexcept
{
    std::error_code existsEc;
    if (pub.empty() || !std::filesystem::exists(pub, existsEc))
        return {};

    std::ifstream file(pub, std::ios::binary);
    if (!file)
        return {};

    std::ostringstream content;
    content << file.rdbuf();

    std::vector<unsigned char> pubKeyBin;
    if (!crypto::utils::base64ToBin(pubKeyBin, content.str()))
        return {};

    return crypto::utils::toBase64(pubKeyBin);
}

bool JobCryptoKeys::validPublicKey(const std::filesystem::path &publicKeyFile) noexcept
{
    std::error_code existsEc;
    if (publicKeyFile.empty() || !std::filesystem::exists(publicKeyFile, existsEc))
        return false;

    std::ifstream file(publicKeyFile, std::ios::binary);
    if (!file)
        return false;

    std::ostringstream content;
    content << file.rdbuf();

    std::vector<unsigned char> pubKeyBin;
    if (!crypto::utils::base64ToBin(pubKeyBin, content.str()))
        return false;

    return pubKeyBin.size() == crypto_sign_PUBLICKEYBYTES;
}

bool JobCryptoKeys::privateKeyMatchesPublicKey(KeyType type) const noexcept
{
    std::vector<unsigned char> pubKeyBin;
    if (!crypto::utils::base64ToBin(pubKeyBin, publicKey()))
        return false;

    switch (type) {
    case KeyType::Exchange: {
        if (privateKey().size() != crypto_kx_SECRETKEYBYTES ||
            pubKeyBin.size() != crypto_kx_PUBLICKEYBYTES)
            return false;

        unsigned char derivedPub[crypto_kx_PUBLICKEYBYTES];

        if (crypto_scalarmult_base(derivedPub, privateKey().data()) != 0)
            return false;

        bool const matches = sodium_memcmp(
                                 derivedPub,
                                 pubKeyBin.data(),
                                 crypto_kx_PUBLICKEYBYTES
                                 ) == 0;

        sodium_memzero(derivedPub, sizeof(derivedPub));
        return matches;
    }

    case KeyType::Sign: {
        if (privateKey().size() != crypto_sign_SECRETKEYBYTES ||
            pubKeyBin.size() != crypto_sign_PUBLICKEYBYTES)
            return false;

        unsigned char derivedPub[crypto_sign_PUBLICKEYBYTES];
        unsigned char derivedSk[crypto_sign_SECRETKEYBYTES];

        if (crypto_sign_seed_keypair(derivedPub, derivedSk, privateKey().data()) != 0) {
            sodium_memzero(derivedSk, sizeof(derivedSk));
            sodium_memzero(derivedPub, sizeof(derivedPub));
            return false;
        }

        bool const matches = sodium_memcmp(
                                 derivedPub,
                                 pubKeyBin.data(),
                                 crypto_sign_PUBLICKEYBYTES
                                 ) == 0;

        sodium_memzero(derivedSk, sizeof(derivedSk));
        sodium_memzero(derivedPub, sizeof(derivedPub));
        return matches;
    }
    }

    return false;
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

bool JobCryptoKeys::createAndSaveKeys(const std::filesystem::path &dirPath, KeyType type,
                                      const std::string &pubName, const std::string &priName) noexcept
{
    if (!createKeys(type))
        return false;

    return saveKeys(dirPath, pubName, priName);
}

bool JobCryptoKeys::saveKeys(const std::filesystem::path &dirPath, const std::string &pubName, const std::string &priName) noexcept
{
    if (!isValid())
        return false;

    std::error_code ec;
    if (!std::filesystem::exists(dirPath)) {
        std::filesystem::create_directories(dirPath, ec);
        if (ec)
            return false;
    }

    std::filesystem::path const pubPath = dirPath / pubName;
    std::filesystem::path const priPath = dirPath / priName;

    std::ofstream pubFile(pubPath, std::ios::out | std::ios::binary);
    if (!pubFile.is_open()) return false;

    std::string const pubData = publicKey();
    pubFile.write(pubData.data(), static_cast<std::streamsize>(pubData.size()));
    pubFile.close();
    if (pubFile.fail())
        return false;

    if (m_privateKey.empty())
        return false;

    std::ofstream priFile(priPath, std::ios::out | std::ios::binary);
    if (!priFile.is_open())
        return false;

    priFile.write(reinterpret_cast<const char*>(m_privateKey.data()),
                  static_cast<std::streamsize>(m_privateKey.size()));

    priFile.flush();
    priFile.close();
    if (priFile.fail())
        return false;

#ifndef NDEBUG
    // JOB_LOG_DEBUG("[DEBUG SAVE] Private Key Size in memory: %" << m_privateKey.size())";
#endif

    return true;
}

} // namespace job::crypto