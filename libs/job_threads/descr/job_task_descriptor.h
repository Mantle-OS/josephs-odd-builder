#pragma once

#include "job_idescriptor.h"

namespace job::threads {
class JobTaskDescriptor : public JobIDescriptor {
public:
    using Ptr = std::shared_ptr<JobTaskDescriptor>;
    JobTaskDescriptor(uint64_t id, int priority) :
        JobIDescriptor{}
    {
        setId(id);
        setPriority(priority);
    }
};
} // namespace job::threads
// CHECKPOINT: v1.0
