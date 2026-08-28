#include "qsecuremem.h"
#include <utility>
QSecureMem::QSecureMem(size_t size) :
    job::crypto::JobSecureMem(size)
{
}

QSecureMem::QSecureMem(const job::crypto::JobSecureMem &other):
    job::crypto::JobSecureMem(other)
{
}

QSecureMem::QSecureMem(job::crypto::JobSecureMem &&other) noexcept :
    job::crypto::JobSecureMem(std::move(other))
{
}

QSecureMem &QSecureMem::operator=(job::crypto::JobSecureMem &&other) noexcept
{
    job::crypto::JobSecureMem::operator=(std::move(other));
    return *this;
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

