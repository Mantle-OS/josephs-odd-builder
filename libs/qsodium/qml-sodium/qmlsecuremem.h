#pragma once

#include <QObject>
#include <qqmlregistration.h>
#include <qsecuremem.h>

class QmlSecureMem : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit QmlSecureMem(QObject *parent = nullptr) :
        QObject{parent},
        m_mem{new QSecureMem{64}} // WE OWN THIS
    {}

    ~QmlSecureMem() override
    {
        if (m_mem) {
            m_mem->clear();
        }
        delete m_mem;
        m_mem = nullptr;
    }

    [[nodiscard]] QSecureMem *internalBuffer() noexcept
    {
        return m_mem;
    }

    [[nodiscard]] const QSecureMem *internalBuffer() const noexcept
    {
        return m_mem;
    }

    [[nodiscard]] bool copyFromSecureMem(const QSecureMem &source) noexcept
    {
        if (!m_mem || source.empty())
            return false;

        if (!m_mem->allocate(source.size()))
            return false;

        m_mem->copyFrom(source.data(), source.size());
        return true;
    }

    QSecureMem *mem() const {return m_mem;}

private:
    QSecureMem *m_mem = nullptr;
};