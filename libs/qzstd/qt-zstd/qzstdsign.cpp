#include "qzstdsign.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>

QZstdSign::QZstdSign(QObject *parent) :
    QZstdOptions{parent},
    m_signer{new QSodiumCryptoSign{}}
{
}

QZstdSign::~QZstdSign()
{
    // Not owned in the QObjectTree
    if (m_signer) {
        delete m_signer;
        m_signer = nullptr;
    }
}

void QZstdSign::setSigningKeyPair(const QString& publicKey, const QSecureMem& privateKey)
{
    if (m_signer) {
        m_signer->setPublicKey(publicKey);
        m_signer->setPrivateKey(privateKey);
    }
}

void QZstdSign::setVerificationKey(const QString& publicKey)
{
    if (m_signer)
        m_signer->setPublicKey(publicKey);
}

bool QZstdSign::execute()
{
    return sign();
}

bool QZstdSign::sign()
{
    if (input().isEmpty() || output().isEmpty()) {
        setErrorString(QStringLiteral("Input or output pathways are unconfigured."));
        return false;
    }

    QFile fileToSign(input());
    if (!fileToSign.exists()) {
        setErrorString(QStringLiteral("Target data file does not exist."));
        return false;
    }

    setTotal(static_cast<int>(fileToSign.size()));
    setCurrent(0);

    QString signatureBase64;
    bool const successful = m_signer->signFile(input(), signatureBase64);

    if (!successful) {
        setErrorString(QStringLiteral("Crypto signing runtime failure. Ensure private key parameters are locked in."));
        return false;
    }

    QFile sigFile(output());
    if (!sigFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        setErrorString(sigFile.errorString());
        return false;
    }

    QTextStream out(&sigFile);
    out << signatureBase64;
    sigFile.flush();
    sigFile.close();

    setCurrent(total());
    // Q_EMIT finished();  // Therre was a issue here let's let others handle the "finished"
    return true;
}

bool QZstdSign::verify()
{
    if (input().isEmpty()) {
        setErrorString(QStringLiteral("Verification source (.sig) pathway is missing."));
        return false;
    }

    if (!input().endsWith(QStringLiteral(".sig"), Qt::CaseInsensitive)) {
        setErrorString(QStringLiteral("Verification input pathway must target a valid '.sig' companion asset."));
        return false;
    }

    QString const dataFilePath = input().left(input().length() - 4);
    QFile targetDataFile(dataFilePath);
    if (!targetDataFile.exists()) {
        setErrorString(QStringLiteral("Target payload matching this signature could not be located on disk."));
        return false;
    }

    QFile sigFile(input());
    if (!sigFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setErrorString(sigFile.errorString());
        return false;
    }

    QTextStream in(&sigFile);
    QString const signatureBase64 = in.readAll().trimmed();
    sigFile.close();

    if (signatureBase64.isEmpty()) {
        setErrorString(QStringLiteral("Signature companion payload is empty."));
        return false;
    }

    setTotal(static_cast<int>(targetDataFile.size()));
    setCurrent(0);

    bool const signatureValid = m_signer->verifyFile(dataFilePath, signatureBase64);

    if (!signatureValid) {
        setErrorString(QStringLiteral("Digital signature validation failed! Content payload has been compromised."));
        return false;
    }

    setCurrent(total());
    // Q_EMIT finished(); // Therre was a issue here let's let others handle the "finished"
    return true;
}

void QZstdSign::setSigningKeyPairB64(const QString &publicKey, const QString &privateKeyB64) {
    m_signer->setPublicKey(publicKey);

    QSecureMem privKey;
    if (privKey.fromBase64(privateKeyB64))
        m_signer->setPrivateKey(privKey);
}

