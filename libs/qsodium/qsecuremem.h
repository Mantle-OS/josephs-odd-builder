#pragma once

#include <QByteArray>
#include <QString>

#include <cstddef>
#include <cstring>
#include <sodium/utils.h>

class QSecureMem
{
public:

    explicit QSecureMem(size_t size = 0);
    QSecureMem(const QSecureMem &other);
    QSecureMem &operator=(const QSecureMem &other);
    QSecureMem(QSecureMem &&other) noexcept;
    QSecureMem &operator=(QSecureMem &&other) noexcept;
    ~QSecureMem();


    [[nodiscard]] bool allocate(size_t size);
    void copyFrom(const void *src, size_t len);
    void clear() noexcept;
    void free() noexcept;

    [[nodiscard]] unsigned char *data() noexcept;
    [[nodiscard]] const unsigned char *data() const noexcept;
    [[nodiscard]] qsizetype size() const noexcept;
    [[nodiscard]] bool isEmpty() const noexcept{return m_size == 0;}

    [[nodiscard]] QString toString() const;
    [[nodiscard]] QString toBase64(int variant = sodium_base64_VARIANT_ORIGINAL) const;
    bool fromBase64(const QString &encoded, int variant = sodium_base64_VARIANT_ORIGINAL);
    [[nodiscard]] QString fromBase64toString(const QString &encoded,
                                                 int variant = sodium_base64_VARIANT_ORIGINAL) const;
    void appendTo(QByteArray *out) const
    {
        if (!out || !m_data || m_size == 0)
            return;

        out->append(reinterpret_cast<const char *>(m_data),
                    static_cast<qsizetype>(m_size));
    }

    [[nodiscard]] bool operator==(const QSecureMem &other) const noexcept;
    [[nodiscard]] bool operator!=(const QSecureMem &other) const noexcept;

private:
    unsigned char *m_data{nullptr};
    size_t m_size{0};
};


