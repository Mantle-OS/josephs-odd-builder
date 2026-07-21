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
    QSecureMem &operator=(const QSecureMem &other) = default;
    QSecureMem(QSecureMem &&other) noexcept = default;
    QSecureMem &operator=(QSecureMem &&other) noexcept = default;

    QSecureMem &operator=(const job::crypto::JobSecureMem &other);
    bool operator==(const QSecureMem &other) const noexcept;
    bool operator!=(const QSecureMem &other) const noexcept;

    // Explicit overrides only needed for specialized Qt data format conversions
    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] bool isEmpty() const noexcept
    {
        return empty();
    }

// You can use these for debugging not for production ,.... Fuck that dont use this at all.
#if 0
    void appendTo(QByteArray *out) const;
    [[nodiscard]] QString toString() const;
    [[nodiscard]] QString toBase64(int variant = 1 /* sodium_base64_VARIANT_ORIGINAL */) const;
    bool fromBase64(const QString &encoded, int variant = 1);
    [[nodiscard]] QString fromBase64toString(const QString &encoded, int variant = 1) const;
#endif
};