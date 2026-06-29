#pragma once

#include "qzstdoptions.h"
#include <qsodiumcryptosign.h>
#include <qsecuremem.h>

class QZstdSign : public QZstdOptions
{
    Q_OBJECT
public:
    explicit QZstdSign(QObject *parent = nullptr);
    ~QZstdSign() override;
    void setSigningKeyPair(const QString& publicKey, const QSecureMem& privateKey);
    void setVerificationKey(const QString& publicKey);
    bool execute();
    bool sign();
    bool verify();
    void setSigningKeyPairB64(const QString& publicKey, const QString& privateKeyB64);
private:
    QSodiumCryptoSign *m_signer = nullptr; // We own this
};