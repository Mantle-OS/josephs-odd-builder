#include "qsecuremem.h"
 // Pull constructors directly into our class scope footprint

QSecureMem::QSecureMem(size_t size) :
    job::crypto::JobSecureMem(size)
{
}

QSecureMem::QSecureMem(const job::crypto::JobSecureMem &other):
    job::crypto::JobSecureMem(other)
{
}

bool QSecureMem::operator==(const QSecureMem &other) const noexcept
{
    return job::crypto::JobSecureMem::operator==(other);
}

bool QSecureMem::operator!=(const QSecureMem &other) const noexcept

{
    return job::crypto::JobSecureMem::operator!=(other);
}

QSecureMem &QSecureMem::operator=(const  job::crypto::JobSecureMem &other)
{
    if (this != &other) {
        job::crypto::JobSecureMem::operator=(other);
    }
    return *this;
}

size_t QSecureMem::size() const noexcept
{
    return job::crypto::JobSecureMem::size();
}


#if 0
#include <string>
void QSecureMem::appendTo(QByteArray *out) const
{
    if (out && !isEmpty()) {
        out->append(reinterpret_cast<const char*>(data()), static_cast<int>(job::crypto::JobSecureMem::size()));
    }
}
QString QSecureMem::toString() const
{
    return QString::fromStdString(job::crypto::JobSecureMem::toString());
}

QString QSecureMem::toBase64(int variant) const
{
    return QString::fromStdString(job::crypto::JobSecureMem::toBase64(variant));
}

bool QSecureMem::fromBase64(const QString &encoded, int variant)
{
    return job::crypto::JobSecureMem::fromBase64(encoded.toStdString(), variant);
}

QString QSecureMem::fromBase64toString(const QString &encoded, int variant) const
{
    return QString::fromStdString(job::crypto::JobSecureMem::fromBase64toString(encoded.toStdString(), variant));
}
#endif