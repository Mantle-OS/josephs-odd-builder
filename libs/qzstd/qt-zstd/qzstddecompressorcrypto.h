#ifndef QZSTDDECOMPRESSORCRYPTO_H
#define QZSTDDECOMPRESSORCRYPTO_H

#include "qzstddecompressor.h"
#include <qsodiumsecretbox.h>
#include <qsecuremem.h>

class QZstdDecompressorCrypto : public QZstdDecompressor
{
    Q_OBJECT
public:
    explicit QZstdDecompressorCrypto(QObject *parent = nullptr);
    ~QZstdDecompressorCrypto() override = default;

    QSecureMem decryptionKey() const;
    void setDecryptionKey(const QSecureMem &key);
    void setDecryptionKeyB64(const QString &base64Key);

public Q_SLOTS:
    bool execute() override final;
    bool decompressFolder() override final;
    bool decompressFile() override final;

Q_SIGNALS:
    void decryptionKeyChanged();

private:
    QSecureMem m_decryptionKey;
};

#endif // QZSTDDECOMPRESSORCRYPTO_H