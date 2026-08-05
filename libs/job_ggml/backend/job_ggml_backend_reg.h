#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <ggml-backend.h>
#include "jobggml_export.h"

namespace job::ggml {
class JOBGGML_EXPORT JobGgmlBackendReg
{
public:
    using Ptr  = std::shared_ptr<JobGgmlBackendReg>;
    using UPtr = std::unique_ptr<JobGgmlBackendReg>;
    using WPtr = std::weak_ptr<JobGgmlBackendReg>;

    explicit JobGgmlBackendReg(ggml_backend_reg_t backendReg);
    ~JobGgmlBackendReg() = default;
    [[nodiscard]] static Ptr createShared(ggml_backend_reg_t backendReg) {return std::make_shared<JobGgmlBackendReg>(backendReg);}
    [[nodiscard]] static UPtr createUniq(ggml_backend_reg_t backendReg){return std::make_unique<JobGgmlBackendReg>(backendReg);}

    JobGgmlBackendReg(const JobGgmlBackendReg &) = delete;
    JobGgmlBackendReg &operator=(const JobGgmlBackendReg &) = delete;
    JobGgmlBackendReg(JobGgmlBackendReg &&) = delete;
    JobGgmlBackendReg &operator=(JobGgmlBackendReg &&) = delete;

    [[nodiscard]] const std::string &name() const noexcept;
    void setName(const std::string &name);

    [[nodiscard]] std::size_t deviceCount() const noexcept;


    //  maybe make override for JobGgmlDevice
    [[nodiscard]] ggml_backend_dev_t device(std::size_t index) const noexcept;

    [[nodiscard]] void *procAddress(const std::string &name) const noexcept;

    [[nodiscard]] ggml_backend_reg_t backendReg() const noexcept;

    [[nodiscard]] bool isValid() const noexcept;
private:
    ggml_backend_reg_t m_backendReg{nullptr}; // Borrowed from the GGML registry.
    std::string        m_name{"unknown"};
};
} // namespace job::ggml
