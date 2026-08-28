#pragma once

#include <QByteArray>
#include <QString>

#include <cstddef>

#include <job_secure_mem.h>

#include "qsodium_export.h"

class QSODIUM_EXPORT QSecureMem : public job::crypto::JobSecureMem
{
public:
    explicit QSecureMem(size_t size = 0);

    QSecureMem(const job::crypto::JobSecureMem &other);
    QSecureMem(job::crypto::JobSecureMem &&other) noexcept;

    QSecureMem(const QSecureMem &other) = default;
    QSecureMem &operator=(const QSecureMem &other) = default;
    QSecureMem(QSecureMem &&other) noexcept = default;
    QSecureMem &operator=(QSecureMem &&other) noexcept = default;

    QSecureMem &operator=(const job::crypto::JobSecureMem &other);
    QSecureMem &operator=(job::crypto::JobSecureMem &&other) noexcept;

    bool operator==(const QSecureMem &other) const noexcept;
    bool operator!=(const QSecureMem &other) const noexcept;

    [[nodiscard]] size_t size() const noexcept;

    [[nodiscard]] bool isEmpty() const noexcept
    {
        return empty();
    }
};