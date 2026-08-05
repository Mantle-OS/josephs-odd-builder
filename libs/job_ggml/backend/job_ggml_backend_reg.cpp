#include "job_ggml_backend_reg.h"

#include <stdexcept>

namespace job::ggml {

JobGgmlBackendReg::JobGgmlBackendReg(ggml_backend_reg_t backendReg) :
    m_backendReg{backendReg}
{
    if (!m_backendReg)
        throw std::invalid_argument{"JobGgmlBackendReg requires a valid ggml_backend_reg_t"};

    const char *backendName = ggml_backend_reg_name(m_backendReg);
    setName(backendName ? backendName : "unknown");
}

const std::string &JobGgmlBackendReg::name() const noexcept
{
    return m_name;
}

void JobGgmlBackendReg::setName(const std::string &name)
{
    if (!name.empty() && m_name != name)
        m_name = name;
}

std::size_t JobGgmlBackendReg::deviceCount() const noexcept
{
    return ggml_backend_reg_dev_count(m_backendReg);
}

ggml_backend_dev_t JobGgmlBackendReg::device(std::size_t index) const noexcept
{
    if (index >= deviceCount())
        return nullptr;

    return ggml_backend_reg_dev_get(m_backendReg, index);
}

void *JobGgmlBackendReg::procAddress(const std::string &name) const noexcept
{
    if (name.empty())
        return nullptr;

    return ggml_backend_reg_get_proc_address(
        m_backendReg,
        name.c_str()
        );
}

ggml_backend_reg_t JobGgmlBackendReg::backendReg() const noexcept
{
    return m_backendReg;
}

bool JobGgmlBackendReg::isValid() const noexcept
{
    return m_backendReg != nullptr;
}

} // namespace job::ggml