#pragma once

#include <memory>
#include <cstddef>
#include <string>
#include <sodium/utils.h>
#include "jobcrypto_export.h"

namespace job::crypto {

class JOBCRYPTO_EXPORT JobSecureMem
{
public:
    using Ptr  = std::shared_ptr<JobSecureMem>;
    using UPtr = std::unique_ptr<JobSecureMem>;

    explicit JobSecureMem(size_t size = 0);    
    [[nodiscard]] static Ptr createShared(size_t size = 0)
    {
        return std::make_shared<JobSecureMem>(size);
    }

    [[nodiscard]] static UPtr createUniq(size_t size = 0)
    {
        return std::make_unique<JobSecureMem>(size);
    }

    JobSecureMem(const JobSecureMem &other);
    JobSecureMem &operator=(const JobSecureMem &other);
    JobSecureMem(JobSecureMem &&other) noexcept;
    JobSecureMem &operator=(JobSecureMem &&other) noexcept;
    ~JobSecureMem();

    [[nodiscard]] bool allocate(size_t size) noexcept;
    void copyFrom(const void *src, size_t len);
    void clear() noexcept;
    void free() noexcept;

    [[nodiscard]] unsigned char *data() noexcept;
    [[nodiscard]] const unsigned char *data() const noexcept;
    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept { return m_size == 0 || !m_data; }

    // Public keys only....
    [[nodiscard]] std::string toBase64(int variant = sodium_base64_VARIANT_ORIGINAL) const;
    bool fromBase64(const std::string &encoded, int variant = sodium_base64_VARIANT_ORIGINAL);

    [[nodiscard]] bool operator==(const JobSecureMem &other) const noexcept;
    [[nodiscard]] bool operator!=(const JobSecureMem &other) const noexcept;

    void swap(JobSecureMem &other) noexcept;

private:
    unsigned char *m_data{nullptr};
    size_t m_size{0};
};

} // namespace job::crypto