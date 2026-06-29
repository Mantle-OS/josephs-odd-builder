#ifndef QSODIUM_H
#define QSODIUM_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <memory>

#include "qsecuremem.h"
#include "qsodiumkeys.h"
#include "qsodiumcryptosign.h"
#include "qsodiumsecretbox.h"
#include "qsodiumpasswordutils.h"
#include "qsodiumhash.h"

class QSodium : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool initialized READ isInitialized NOTIFY initializationChanged)

public:
    explicit QSodium(QObject *parent = nullptr);
    ~QSodium() override = default;

    bool isInitialized() const noexcept;

    bool verifyFileSignature(const QString &filePath,
                             const QString &publicKeyBase64,
                             const QString &signatureBase64) noexcept;

    QString computeFileBlake2b(const QString &filePath) noexcept;

    static bool encryptConfig(const QByteArray &plainText, const QSecureMem &key,
                              QByteArray &outCipherText, QByteArray &outNonce) noexcept;

    static bool decryptConfig(const QByteArray &cipherText, const QSecureMem &key,
                              const QByteArray &nonce, QSecureMem &outPlainText) noexcept;

Q_SIGNALS:
    void initializationChanged(bool initialized);

private:
    bool m_initSuccess{false};
};

#endif // QSODIUM_H