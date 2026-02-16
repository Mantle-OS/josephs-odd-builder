#include "isocket_io.h"



#include <job_logger.h>
namespace job::net {

ISocketIO::ISocketIO(threads::JobIoAsyncThread::Ptr loop) :
    m_loop(std::move(loop))
{}

int ISocketIO::fd() const noexcept
{
    return m_fd;
}

void ISocketIO::setLoop(const threads::JobIoAsyncThread::Ptr &loop)
{
    m_loop = loop;
}

void ISocketIO::registerEvents(uint32_t events)
{
    if (m_fd < 0) {
        JOB_LOG_ERROR("[ISocketIO] registerEvents called on invalid fd");
        return;
    }
    if (auto loop = m_loop.lock()) {
        if (!loop->registerFD(m_fd, events, [this](uint32_t e) { onEvents(e); }))
            JOB_LOG_ERROR("[ISocketIO] Failed to register FD {}", m_fd);

    } else {
        JOB_LOG_ERROR("[ISocketIO] Failed to register FD {}: Event loop is null", m_fd);
    }
}


} // namespace job::net
// CHECKPOINT: v1.0
