#pragma once
#include <cstddef>

#include <job_ggml_tensor.h>
#include "jobmodel_export.h"

namespace job::model{
class JOBMODEL_EXPORT JobITensorWriter
{
public:
    JobITensorWriter() = default;
    virtual ~JobITensorWriter() = default;

    virtual void write(const void *src = nullptr, std::size_t size = 0) = 0;

    virtual void write_tensor(ggml::JobGgmlTensor   *tensor = nullptr,
                              std::size_t           offset  = 0,
                              std::size_t           size    = 0 ) = 0;

    // bytes written so far
    virtual std::size_t n_bytes() = 0;

    void writeString(const std::string &str = {})
    {

    }
};
}