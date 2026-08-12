#include "job_ggml_abort_callback.h"
namespace job::ggml {

JobGgmlAbortCallback::JobGgmlAbortCallback(JobGgmlAbortCallbackFn callback, void *userData) :
    m_callback{std::move(callback)},
    m_userData{userData}
{
}

bool JobGgmlAbortCallback::isValid() const noexcept
{
    return static_cast<bool>(m_callback);
}

bool JobGgmlAbortCallback::invoke() const
{
    return m_callback ? m_callback(m_userData) : false;
}

void *JobGgmlAbortCallback::userData() const noexcept
{
    return m_userData;
}

void JobGgmlAbortCallback::setUserData(void *userData) noexcept
{
    m_userData = userData;
}

bool JobGgmlAbortCallback::callBouncer(void *data)
{
    auto *callback = static_cast<JobGgmlAbortCallback *>(data);
    return callback && callback->isValid() ? callback->invoke() : false;
}
}
