#pragma once

#include <QByteArray>
#include <QString>

#include <cstddef>

#include <job_secure_mem.h>

class QSecureMem : public job::crypto::JobSecureMem
{
public:
    // Pull constructors directly into our class scope footprint
    explicit QSecureMem(size_t size = 0);
    QSecureMem(const job::crypto::JobSecureMem &other);

    // Explicit overrides only needed for specialized Qt data format conversions
    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] bool isEmpty() const noexcept { return empty(); }

    // these ar grauded and only allowec in debug. and needs cmake flag
    [[nodiscard]] QString toString() const;
    [[nodiscard]] QString toBase64(int variant = 1 /* sodium_base64_VARIANT_ORIGINAL */) const;
    bool fromBase64(const QString &encoded, int variant = 1);
    [[nodiscard]] QString fromBase64toString(const QString &encoded, int variant = 1) const;

    void appendTo(QByteArray *out) const;    

    QSecureMem &operator=(const job::crypto::JobSecureMem &other)
    {
        if (this != &other) {
            job::crypto::JobSecureMem::operator=(other);
        }
        return *this;
    }

    [[nodiscard]] bool operator==(const QSecureMem &other) const noexcept;
    [[nodiscard]] bool operator!=(const QSecureMem &other) const noexcept;
};