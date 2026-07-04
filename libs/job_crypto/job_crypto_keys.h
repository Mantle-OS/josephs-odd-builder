#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <iostream>

#include "job_secure_mem.h"

namespace job::crypto {

class JobCryptoKeys
{
public:
    enum class KeyType {
        Exchange,  // X25519
        Sign       // Ed25519
    };

    explicit JobCryptoKeys();
    ~JobCryptoKeys() = default;

    JobCryptoKeys(const JobCryptoKeys &other) = default;
    JobCryptoKeys &operator=(const JobCryptoKeys &other) = default;
    JobCryptoKeys(JobCryptoKeys &&other) noexcept = default;
    JobCryptoKeys &operator=(JobCryptoKeys &&other) noexcept = default;

    [[nodiscard]] std::string publicKey() const noexcept { return m_publicKey; }
    void setPublicKey(const std::string &pubKey) { m_publicKey = pubKey; }
    [[nodiscard]] bool setPublicKey(const std::vector<unsigned char> &publicKeyBytes) noexcept {
        if (publicKeyBytes.empty())
            return false;

        m_publicKey = std::string(reinterpret_cast<const char*>(publicKeyBytes.data()),
                                  publicKeyBytes.size());
        return true;
    }

    [[nodiscard]] JobSecureMem privateKey() const noexcept { return m_privateKey; }
    void setPrivateKey(const JobSecureMem &privKey);

    [[nodiscard]] bool isValid() const noexcept { return m_validKeys; }

    [[nodiscard]] bool createKeys(KeyType type) noexcept;
    [[nodiscard]] bool createSeedKeys(KeyType type, const JobSecureMem &seed) noexcept;

    [[nodiscard]] bool createClientSessionKeys(JobSecureMem &rx, JobSecureMem &tx, const std::string &serverPublicKey) noexcept;
    [[nodiscard]] bool createServerSessionKeys(JobSecureMem &rx, JobSecureMem &tx, const std::string &clientPublicKey) noexcept;


    [[nodiscard]] bool createAndSaveKeys(const std::filesystem::path &dirPath,
                                         KeyType type,
                                         const std::string &pubName = "public.key",
                                         const std::string &priName = "private.key") noexcept
    {
        if (!createKeys(type))
            return false;

        return saveKeys(dirPath, pubName, priName);
    }


    [[nodiscard]] bool saveKeys(const std::filesystem::path &dirPath,
                                         const std::string &pubName = "public.key",
                                         const std::string &priName = "private.key") noexcept
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

        std::cout << "[DEBUG SAVE] Private Key Size in memory: " << m_privateKey.size() << "\n";


        return true;
    }
    [[nodiscard]] bool loadKeysFromDisk(const std::filesystem::path &pubPath,
                                        const std::filesystem::path &priPath) noexcept
    {
        if (!std::filesystem::exists(pubPath) || !std::filesystem::exists(priPath))
            return false;

        if (isValid())
            clearKeys();

        std::ifstream pubFile(pubPath, std::ios::in | std::ios::binary | std::ios::ate);
        if (!pubFile.is_open()) return false;

        std::streamsize const pubSize = pubFile.tellg();
        pubFile.seekg(0, std::ios::beg);

        std::string newPubKey;
        newPubKey.resize(static_cast<size_t>(pubSize));

        if (!pubFile.read(newPubKey.data(), pubSize))
            return false;
        pubFile.close();

        // Read the Private Key directly into secure memory pages
        std::ifstream priFile(priPath, std::ios::in | std::ios::binary | std::ios::ate);
        if (!priFile.is_open())
            return false;

        std::streamsize const priSize = priFile.tellg();
        priFile.seekg(0, std::ios::beg);

        if (priSize <= 0)
            return false;

        // Allocate the secure virtual memory block dynamically to match the incoming key size
        JobSecureMem newPriKey(static_cast<size_t>(priSize));

        // Read directly out of the OS filesystem buffer right into locked RAM
        if (!priFile.read(reinterpret_cast<char*>(newPriKey.data()), priSize))
            return false;

        priFile.close();

        // Commit the newly loaded assets to your internal tracking states
        m_publicKey = std::move(newPubKey);
        m_privateKey = std::move(newPriKey);
        m_validKeys = true;

        return true;
    }

    void addKeyDirectory(const std::filesystem::path &dir);
    [[nodiscard]] std::vector<std::filesystem::path> keyDirectories() const noexcept { return m_keysDirs; }

private:
    void clearKeys() noexcept{
        m_privateKey.clear();
        m_publicKey.clear();
        m_validKeys = false;
    }
    std::vector<std::filesystem::path> m_keysDirs;
    std::string m_publicKey;
    JobSecureMem m_privateKey;
    bool m_validKeys{false};
    std::map<std::string, std::pair<std::string, JobSecureMem>> m_keyStore;
};

} // namespace job::crypto