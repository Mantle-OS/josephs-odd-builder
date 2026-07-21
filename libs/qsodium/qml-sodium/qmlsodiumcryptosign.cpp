#include "qmlsodiumcryptosign.h"
#include <QDebug>
#include <QFile>
#include <QFileInfo>

QmlSodiumCryptoSign::QmlSodiumCryptoSign(QObject *parent) :
    QObject{parent},
    m_signer{new QSodiumCryptoSign{}}
{

    connect(this, &QmlSodiumCryptoSign::filePathChanged, this, [this]([[maybe_unused]]const QString &file) {
        set_lastStage(QmlSodiumCryptoSign::File);
    });
    // PATHS on win32....
    connect(this, &QmlSodiumCryptoSign::publicKeyChanged, this, [this](const QString &key) {
        QFileInfo pubFi(key);
        QFileInfo priFi(get_privateKey());
        if(pubFi.exists() && priFi.exists() && m_signer->loadKeys(key, get_privateKey()))
            set_lastStage(QmlSodiumCryptoSign::PublicKey);
    });

    connect(this, &QmlSodiumCryptoSign::privateKeyChanged, this, [this](const QString &key) {
        QFileInfo pubFi(get_publicKey());
        QFileInfo priFi(key);
        if(pubFi.exists() && priFi.exists() && m_signer->loadKeys(get_publicKey(), key))
            set_lastStage(QmlSodiumCryptoSign::PrivateKey);
    });
}

QmlSodiumCryptoSign::~QmlSodiumCryptoSign()
{
    delete m_signer;
    m_signer = nullptr;
}

bool QmlSodiumCryptoSign::signFile() noexcept
{
    if (!hasKeys() || get_filePath().isEmpty())
        return false;

    QString outSig;
    // Extracts underlying properties natively, then syncs state back out to QML row properties
    if (m_signer->signFile(get_filePath(), outSig)) {
        set_signatureBase64(outSig);
        return true;
    }
    return false;
}

bool QmlSodiumCryptoSign::signAssociatedFile() noexcept
{
    if (!hasKeys() || get_filePath().isEmpty())
        return false;

    QString outSig;
    if (m_signer->signAssociatedFile(get_filePath(), outSig)) {
        set_signatureBase64(outSig);
        return true;
    }
    return false;
}

bool QmlSodiumCryptoSign::verifyAssociatedFile() noexcept
{
    if (!hasKeys() || get_filePath().isEmpty())
        return false;

    return m_signer->verifyFile(get_filePath(), get_signatureBase64());
}

QString QmlSodiumCryptoSign::computeFileBlake2b() noexcept
{
    if (get_filePath().isEmpty())
        return {};

    QByteArray const binaryHash = QSodiumHash::hashFile(get_filePath());
    return QString::fromLatin1(binaryHash.toHex());
}

bool QmlSodiumCryptoSign::hasKeys() noexcept
{
    if (!m_signer || !m_signer->isValid())
        return false;
    return true;
}

bool QmlSodiumCryptoSign::update_filePath(const QUrl &url) noexcept
{
    QString ok;
    if(urlStr(url, ok)){
        set_filePath(ok);
        return true;
    }
    return false;
}

bool QmlSodiumCryptoSign::update_publicKey(const QUrl &url) noexcept
{
    QString ok;
    if(urlStr(url, ok)){
        set_publicKey(ok);
        return true;
    }
    return false;
}

bool QmlSodiumCryptoSign::update_privateKey(const QUrl &url) noexcept
{
    QString ok;
    if(urlStr(url, ok)){
        set_privateKey(ok);
        return true;
    }
    return false;
}

bool QmlSodiumCryptoSign::loadKeysFromDisk() noexcept
{
    if (!m_signer)
        return false;

    return m_signer->loadKeys(get_publicKey(), get_privateKey());
}

bool QmlSodiumCryptoSign::urlStr(const QUrl &url, QString &path) const noexcept
{
    if (!url.isValid() || !url.isLocalFile())
        return false;

    path = url.toLocalFile();
    return QFile::exists(path);
}
