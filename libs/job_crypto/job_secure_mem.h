#pragma once

#include <cstddef>
#include <cstring>
#include <string>
#include <sodium/utils.h>

namespace job::crypto {

class JobSecureMem
{
public:

    explicit JobSecureMem(size_t size = 0);
    JobSecureMem(const JobSecureMem &other);
    JobSecureMem &operator=(const JobSecureMem &other);
    JobSecureMem(JobSecureMem &&other) noexcept;
    JobSecureMem &operator=(JobSecureMem &&other) noexcept;
    ~JobSecureMem();


    [[nodiscard]] bool allocate(size_t size);
    void copyFrom(const void *src, size_t len);
    void clear() noexcept;
    void free() noexcept;

    [[nodiscard]] unsigned char *data() noexcept;
    [[nodiscard]] const unsigned char *data() const noexcept;
    [[nodiscard]] size_t size() const noexcept;

    [[nodiscard]] std::string toString() const;
    [[nodiscard]] std::string toBase64(int variant = sodium_base64_VARIANT_ORIGINAL) const;
    bool fromBase64(const std::string &encoded, int variant = sodium_base64_VARIANT_ORIGINAL);
    [[nodiscard]] std::string fromBase64toString(const std::string &encoded,
                                                 int variant = sodium_base64_VARIANT_ORIGINAL) const;


    [[nodiscard]] bool operator==(const JobSecureMem &other) const noexcept;
    [[nodiscard]] bool operator!=(const JobSecureMem &other) const noexcept;


private:
    unsigned char *m_data{nullptr};
    size_t m_size{0};
};

} // namespace job::crypto
// CHECKPOINT: v1
