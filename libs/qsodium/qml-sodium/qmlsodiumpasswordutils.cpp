#include "qmlsodiumpasswordutils.h"

#include <QDebug>

#include <qsodiumpasswordutils.h>

QmlSodiumPasswordUtils::QmlSodiumPasswordUtils(QObject *parent) :
    QObject{parent},
    m_password{new QmlSecureMem{this}}
{
}

QmlSodiumPasswordUtils::~QmlSodiumPasswordUtils()
{
    clearPassword();
}

QmlSecureMem *QmlSodiumPasswordUtils::password() const noexcept
{
    return m_password;
}

bool QmlSodiumPasswordUtils::setPassword(QmlSecureMem *source) noexcept
{
    if (!source || !source->internalBuffer() || !m_password || !m_password->internalBuffer()) {
        qWarning() << "[QmlSodiumPasswordUtils] Missing password memory.";
        return false;
    }

    QSecureMem *src = source->internalBuffer();
    QSecureMem *dst = m_password->internalBuffer();

    if (!src || src->empty() || !dst) {
        qWarning() << "[QmlSodiumPasswordUtils] Source password memory is empty.";
        return false;
    }

    dst->clear();

    if (!dst->allocate(src->size())) {
        qWarning() << "[QmlSodiumPasswordUtils] Failed to allocate password memory.";
        return false;
    }

    dst->copyFrom(src->data(), src->size());

    Q_EMIT passwordChanged(m_password);
    return true;
}

void QmlSodiumPasswordUtils::clearPassword() noexcept
{
    if (m_password && m_password->internalBuffer())
        m_password->internalBuffer()->clear();

    Q_EMIT passwordChanged(m_password);
}

QString QmlSodiumPasswordUtils::hashForStorage() const noexcept
{
    if (!m_password || !m_password->internalBuffer()) {
        qWarning() << "[QmlSodiumPasswordUtils] Missing password.";
        return {};
    }

    QSecureMem *passwordMem = m_password->internalBuffer();

    if (!passwordMem || passwordMem->empty()) {
        qWarning() << "[QmlSodiumPasswordUtils] Cannot generate storage hash from empty password.";
        return {};
    }

    return QSodiumPasswordUtils::hashPasswordForStorage(*passwordMem);
}

bool QmlSodiumPasswordUtils::verifyAgainstStorage(const QString &storedHash) const noexcept
{
    if (storedHash.isEmpty())
        return false;

    if (!m_password || !m_password->internalBuffer())
        return false;

    QSecureMem *passwordMem = m_password->internalBuffer();

    if (!passwordMem || passwordMem->empty())
        return false;

    return QSodiumPasswordUtils::verifyPasswordAgainstStorage(*passwordMem, storedHash);
}