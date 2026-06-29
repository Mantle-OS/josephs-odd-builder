#include "qsodiumhash.h"
#include "qsodiuminit.h"
#include <QFile>
#include <QDebug>

QByteArray QSodiumHash::hashBuffer(const QByteArray &data, size_t hashSize) noexcept
{
    if (!QSodiumInit::isInitialized() && !QSodiumInit::initialize()) return {};

    // Validate size boundaries specified by libsodium (Min: 16 bytes, Max: 64 bytes)
    if (hashSize < crypto_generichash_BYTES_MIN || hashSize > crypto_generichash_BYTES_MAX) {
        qWarning() << "[QSodiumHash] Invalid requested hash byte dimensions:" << hashSize;
        return {};
    }

    QByteArray outHash(hashSize, '\0');

    int const result = crypto_generichash(
        reinterpret_cast<unsigned char*>(outHash.data()), outHash.size(),
        reinterpret_cast<const unsigned char*>(data.constData()), data.size(),
        /* key */ nullptr, /* keylen */ 0 // Passing null avoids keyed MAC workflows
        );

    if (result != 0) {
        qWarning() << "[QSodiumHash] Failed to compute buffer hash pass.";
        return {};
    }

    return outHash;
}

QByteArray QSodiumHash::hashFile(const QString &filePath, size_t hashSize) noexcept
{
    if (!QSodiumInit::isInitialized() && !QSodiumInit::initialize())
        return {};

    if (hashSize < crypto_generichash_BYTES_MIN || hashSize > crypto_generichash_BYTES_MAX) {
        qWarning() << "[QSodiumHash] Invalid requested hash byte dimensions:" << hashSize;
        return {};
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[QSodiumHash] Cannot open file source for hashing loops:" << filePath;
        return {};
    }

    // Initialize the stateful streaming engine structure
    crypto_generichash_state state;
    crypto_generichash_init(&state, /* key */ nullptr, /* keylen */ 0, hashSize);

    QByteArray buffer(m_chunkSize, '\0');
    while (!file.atEnd()) {
        qint64 bytesRead = file.read(buffer.data(), m_chunkSize);
        if (bytesRead < 0) {
            qWarning() << "[QSodiumHash] Disk read error while calculating signature hash for:" << filePath;
            return {};
        }
        crypto_generichash_update(&state, reinterpret_cast<const unsigned char*>(buffer.constData()), bytesRead);
    }

    QByteArray outHash(hashSize, '\0');
    crypto_generichash_final(&state, reinterpret_cast<unsigned char*>(outHash.data()), outHash.size());

    return outHash;
}