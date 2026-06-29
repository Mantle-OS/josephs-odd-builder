#include "qzstdoptions.h"
#include "qzstdio.h"

QZstdOptions::QZstdOptions(QObject *parent)
    : QObject{parent}
    , m_current(0)
    , m_total(0)
    , m_compressionLevel(3)
    , m_errorString{"none"}
{}

QString QZstdOptions::input() const
{
    return m_input;
}

void QZstdOptions::setInput(const QString &newInput)
{
    if (m_input == newInput)
        return;
    m_input = newInput;
    Q_EMIT inputChanged();
}

QString QZstdOptions::output() const
{
    return m_output;
}

void QZstdOptions::setOutput(const QString &newOutput)
{
    if (m_output == newOutput)
        return;
    m_output = newOutput;
    Q_EMIT outputChanged();
}

int QZstdOptions::current() const
{
    return m_current;
}

void QZstdOptions::setCurrent(int newCurrent)
{
    if (m_current == newCurrent)
        return;
    m_current = newCurrent;
    Q_EMIT currentChanged(newCurrent);
}

int QZstdOptions::total() const
{
    return m_total;
}

void QZstdOptions::setTotal(int newTotal)
{
    if (m_total == newTotal)
        return;
    m_total = newTotal;
    Q_EMIT totalChanged(newTotal);
}

int QZstdOptions::compressionLevel() const
{
    return m_compressionLevel;
}

void QZstdOptions::setCompressionLevel(int newCompressionLevel)
{
    const int minLevel = ZSTD_minCLevel();
    const int maxLevel = ZSTD_maxCLevel();
    if (m_compressionLevel != newCompressionLevel) {
        if (newCompressionLevel >= minLevel && newCompressionLevel <= maxLevel)
            m_compressionLevel = newCompressionLevel;
        else
            m_compressionLevel = 3;

        Q_EMIT compressionLevelChanged();
    }
}
QString QZstdOptions::errorString() const
{
    return m_errorString;
}

void QZstdOptions::setErrorString(const QString &newErrorString)
{
    if (m_errorString == newErrorString)
        return;
    m_errorString = newErrorString;
    Q_EMIT errorStringChanged(newErrorString);
}
