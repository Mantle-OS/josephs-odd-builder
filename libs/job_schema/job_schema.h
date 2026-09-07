#pragma once

#include <memory>

#include <job_base_obj.h>

#include "jobschema_export.h"

namespace job::schema {

class JOBSCHEMA_EXPORT JobSchema : public job::core::BaseObject
{
public:
    using Ptr  = std::shared_ptr<JobSchema>;
    using WPtr = std::weak_ptr<JobSchema>;
    using UPtr = std::unique_ptr<JobSchema>;

    JobSchema() = default;
    virtual ~JobSchema() = default;

    JobSchema(const JobSchema &) = delete;
    JobSchema &operator=(const JobSchema &) = delete;
    JobSchema(JobSchema &&) noexcept = default;
    JobSchema &operator=(JobSchema &&) noexcept = default;
};

} // namespace job::schema