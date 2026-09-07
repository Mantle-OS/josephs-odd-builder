#include "qmlsodiumsha.h"

SodiumSha::SodiumSha(QObject *parent) :
    QObject{parent},
    m_shaTypes{new QmlStringList{this}}
{
    m_shaTypes->setStringList({
        QStringLiteral("SHA-224"),
        QStringLiteral("SHA-256"),
        QStringLiteral("SHA-384"),
        QStringLiteral("SHA-512"),
        QStringLiteral("SHA3-224"),
        QStringLiteral("SHA3-256"),
        QStringLiteral("SHA3-384"),
        QStringLiteral("SHA3-512"),
        QStringLiteral("GENERIC"),
        QStringLiteral("BLAKE2B")
    });
}

QString SodiumSha::computeHex(const QString &data) noexcept
{
    QString const sha = QSodiumSha::hashStringHex(m_jobType, data);

    if (sha.isEmpty()) {
        set_errorString(QStringLiteral("Failed to compute SHA"));
        return {};
    }

    set_errorString({});
    set_lastSha(sha);

    Q_EMIT finished(m_currentType, sha);
    return sha;
}

QString SodiumSha::computeHex(const QString &data, ShaType type) noexcept
{
    setCurrentType(type);
    return computeHex(data);
}

SodiumSha::ShaType SodiumSha::currentType() const noexcept
{
    return m_currentType;
}

void SodiumSha::setCurrentType(ShaType newCurrentType) noexcept
{
    if (m_currentType == newCurrentType)
        return;

    switch (newCurrentType) {
    case ShaType::Sha224:
        m_jobType = job::crypto::JobShaType::Sha224;
        break;
    case ShaType::Sha256:
        m_jobType = job::crypto::JobShaType::Sha256;
        break;
    case ShaType::Sha384:
        m_jobType = job::crypto::JobShaType::Sha384;
        break;
    case ShaType::Sha512:
        m_jobType = job::crypto::JobShaType::Sha512;
        break;
    case ShaType::Sha3_224:
        m_jobType = job::crypto::JobShaType::Sha3_224;
        break;
    case ShaType::Sha3_256:
        m_jobType = job::crypto::JobShaType::Sha3_256;
        break;
    case ShaType::Sha3_384:
        m_jobType = job::crypto::JobShaType::Sha3_384;
        break;
    case ShaType::Sha3_512:
        m_jobType = job::crypto::JobShaType::Sha3_512;
        break;
    case ShaType::ShaGeneric:
        m_jobType = job::crypto::JobShaType::ShaGeneric;
        break;
    case ShaType::ShaBlake2b:
        m_jobType = job::crypto::JobShaType::ShaBlake2b;
        break;
    default:
        return;
    }

    m_currentType = newCurrentType;
    Q_EMIT currentTypeChanged(m_currentType);
}