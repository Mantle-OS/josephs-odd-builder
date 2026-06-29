#pragma once

#include "qzstdcompressor.h"
#include <qsodiumsecretbox.h>
#include <qsecuremem.h>

class QZstdCompressorCrypto : public QZstdCompressor
{
    Q_OBJECT
public:
    explicit QZstdCompressorCrypto(QObject *parent = nullptr);
    ~QZstdCompressorCrypto() override = default;

    QSecureMem encryptionKey() const;
    void setEncryptionKey(const QSecureMem &key);
    void setEncryptionKeyB64(const QString &base64Key);

public Q_SLOTS:
    bool execute() override final;
    bool compressFolder() override final;
    bool compressFile() override final;

Q_SIGNALS:
    void encryptionKeyChanged();

private:
    QSecureMem m_encryptionKey;
};
