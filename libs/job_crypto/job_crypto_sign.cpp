#include "job_crypto_sign.h"

#include <fstream>
#include <iostream>
#include <vector>

#include <sodium.h>

namespace job::crypto {

JobCryptoSign::JobCryptoSign() :
    JobCryptoKeys()
{
}

JobCryptoSign::File_Ptr JobCryptoSign::file() const noexcept
{
    return m_file;
}

void JobCryptoSign::setFile(JobCryptoSign::File_Ptr newFile)
{
    if (m_file == newFile)
        return;
    if (m_file && m_file.use_count() == 1 && m_file->is_open())
        m_file->close();

    m_file = std::move(newFile);
    m_associatedPath.clear();
}

bool JobCryptoSign::signFile(const std::string &filePath, std::string &outSignatureBase64) noexcept
{
    std::ifstream stream(filePath, std::ios::binary);
    if (!stream.is_open()) {
#ifndef NDEBUG
        std::cerr << "[jobcrypto::JobCryptoSign] Cannot open target file for signing: " << filePath << "\n";
#endif
        return false;
    }

    if (!isValid() || privateKey().size() != crypto_sign_SECRETKEYBYTES) {
#ifndef NDEBUG
        std::cerr << "[jobcrypto::JobCryptoSign] Invalid state envelope context or unexpected private key dimensions.\n";
#endif
        return false;
    }

    crypto_sign_state state;
    crypto_sign_init(&state);

    std::vector<char> buffer(kChunkSize);
    while (stream.read(buffer.data(), kChunkSize) || stream.gcount() > 0) {
        crypto_sign_update(&state, reinterpret_cast<const unsigned char*>(buffer.data()), stream.gcount());
    }

    std::vector<unsigned char> sigBin(crypto_sign_BYTES);
    unsigned long long sigLen = 0;

    if (crypto_sign_final_create(&state, sigBin.data(), &sigLen, privateKey().data()) != 0)
        return false;

    std::string outB64(sodium_base64_encoded_len(sigBin.size(), sodium_base64_VARIANT_ORIGINAL), '\0');
    sodium_bin2base64(outB64.data(), outB64.size(), sigBin.data(), sigBin.size(), sodium_base64_VARIANT_ORIGINAL);

    if (!outB64.empty() && outB64.back() == '\0')
        outB64.pop_back();

    outSignatureBase64 = std::move(outB64);
    return true;
}

bool JobCryptoSign::signAssociatedFile(std::string &outSignatureBase64) noexcept
{
    if (!m_file || m_associatedPath.empty()) // ## m_associatedPath is empty AND m_file is null
        return false;

    bool wasOpen = m_file->is_open();
    if (!wasOpen) {
        m_file->open(m_associatedPath, std::ios::binary);
        if (!m_file->is_open())
            return false;
    }

    std::streampos originalPosition = m_file->tellg();
    m_file->seekg(0, std::ios::beg);

    bool const result = signFile(m_associatedPath, outSignatureBase64);

    if (wasOpen)
        m_file->seekg(originalPosition);
    else
        m_file->close();

    return result;
}

bool JobCryptoSign::signAssociatedFile(const std::string &associatedPath, std::string &outSignatureBase64) noexcept
{
    if (associatedPath.empty())
        return false;
    return signFile(associatedPath, outSignatureBase64);
}



bool JobCryptoSign::verifyAssociatedFile(const std::string &signatureBase64) noexcept
{
    if (!m_file || m_associatedPath.empty())
        return false;

    bool wasOpen = m_file->is_open();
    if (!wasOpen) {
        m_file->open(m_associatedPath, std::ios::binary);
        if (!m_file->is_open())
            return false;
    }

    std::streampos originalPosition = m_file->tellg();
    m_file->seekg(0, std::ios::beg);

    bool const result = verifyFile(m_associatedPath, signatureBase64);

    if (wasOpen)
        m_file->seekg(originalPosition);
    else
        m_file->close();

    return result;
}

} // namespace job::crypto