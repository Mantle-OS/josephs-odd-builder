#pragma once
#include <cstddef>
#include <job_ggml_tensor.h>

#include "jobmodel_export.h"

namespace job::model{
class JOBMODEL_EXPORT JobITensorReader
{
public:
    JobITensorReader() = default;
    virtual ~JobITensorReader() = default;

    virtual void read(void *dst = nullptr,
                      std::size_t size = 0) = 0;

    virtual void read_tensor(ggml::JobGgmlTensor *tensor    = nullptr,
                             std::size_t offset             = 0 ,
                             std::size_t size               = 0 ) = 0;

    // bytes read so far
    virtual size_t nBytes() = 0;
    void readString(std::string &str)
    {

    }
};

}