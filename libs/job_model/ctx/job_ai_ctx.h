#pragma once

#include <memory>

namespace job::model {

class JobAiCtx
{
public:
    using Ptr  = std::shared_ptr<JobAiCtx>;
    using WPtr = std::weak_ptr<JobAiCtx>;
    using UPtr = std::unique_ptr<JobAiCtx>;


    JobAiCtx();
};
}
