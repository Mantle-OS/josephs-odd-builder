#ifndef QSODIUMCRYPTOSIGN_H
#define QSODIUMCRYPTOSIGN_H

#include <memory>

#include <QFile>
#include <QString>
#include <QByteArray>
#include "qsodiumkeys.h"

class QSodiumCryptoSign : public QSodiumKeys
{
public:
    explicit QSodiumCryptoSign();
    ~QSodiumCryptoSign() = default;
    QSodiumCryptoSign(const QSodiumCryptoSign &other) = default;
    QSodiumCryptoSign &operator=(const QSodiumCryptoSign &other) = default;
    QSodiumCryptoSign(QSodiumCryptoSign &&other) noexcept = default;
    QSodiumCryptoSign &operator=(QSodiumCryptoSign &&other) noexcept = default;

    // --- File Association ---
    [[nodiscard]] std::shared_ptr<QFile> file() const noexcept;
    void setFile(std::shared_ptr<QFile> newFile);

    // --- High-Level Detached File Signing (Streaming) ---
    // Generates a separate, standalone signature file (e.g., file.zst.sig)
    [[nodiscard]] bool signFile(const QString &filePath, QString &outSignatureBase64) noexcept;
    [[nodiscard]] bool signAssociatedFile(QString &outSignatureBase64) noexcept;

    // --- High-Level Detached File Verification (Streaming) ---
    // Validates a file against a provided standalone signature string
    [[nodiscard]] bool verifyFile(const QString &filePath, const QString &signatureBase64) noexcept;
    [[nodiscard]] bool verifyAssociatedFile(const QString &signatureBase64) noexcept;

private:
    std::shared_ptr<QFile> m_file{nullptr};

    // Internal streaming chunk size configuration (Default: 64KB chunks)
    static constexpr size_t m_chunkSize = 65536;
};

#endif // QSODIUMCRYPTOSIGN_H