#include "qmlsecuremem.h"

QmlSecureMem::QmlSecureMem(QObject *parent) :
    QObject{parent},
    m_mem{new QSecureMem{64}} // WE OWN THIS
{

}

QmlSecureMem::~QmlSecureMem()
{
    if (m_mem)
        m_mem->clear();
    delete m_mem;
    m_mem = nullptr;
}

QSecureMem *QmlSecureMem::internalBuffer() noexcept
{
    return m_mem;
}

const QSecureMem *QmlSecureMem::internalBuffer() const noexcept
{
    return m_mem;
}

bool QmlSecureMem::copyFromSecureMem(const QSecureMem &source) noexcept
{
    if (!m_mem || source.empty())
        return false;

    if (!m_mem->allocate(source.size()))
        return false;

    m_mem->copyFrom(source.data(), source.size());
    return true;
}

QSecureMem *QmlSecureMem::mem() const noexcept
{
    return m_mem;
}

