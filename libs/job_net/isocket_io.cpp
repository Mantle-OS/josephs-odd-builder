#include "isocket_io.h"

#include <job_logger.h>
namespace job::net {

ISocketIO::ISocketIO(threads::JobIoAsyncThread::Ptr loop) :
    m_loop(std::move(loop))
{

}

int ISocketIO::fd() const noexcept
{
    return m_fd;
}

void ISocketIO::setLoop(const threads::JobIoAsyncThread::Ptr &loop)
{
    m_loop = loop;
}


void ISocketIO::registerEvents(threads::IOEvent events)
{
    if (m_fd < 0) {
        JOB_LOG_ERROR("[ISocketIO] registerEvents called on invalid fd");
        return;
    }

    if (auto loop = m_loop.lock()) {
        const auto weakSelf = weak_from_this();

        if (!loop->registerFD(m_fd, events, [weakSelf](threads::IOEvent e) {
                const auto self = weakSelf.lock();

                if (self)
                    self->onEvents(e);
            })) {
            JOB_LOG_ERROR("[ISocketIO] Failed to register FD {}", m_fd);
        }
    } else {
        JOB_LOG_ERROR("[ISocketIO] Failed to register FD {}: Event loop is null", m_fd);
    }
}
/*
void ISocketIO::registerEvents(threads::IOEvent events)
{
    if (m_fd < 0) {
        JOB_LOG_ERROR("[ISocketIO] registerEvents called on invalid fd");
        return;
    }

    if (auto loop = m_loop.lock()) {
        if (!loop->registerFD(m_fd, events, [this](threads::IOEvent e) { onEvents(e); }))
            JOB_LOG_ERROR("[ISocketIO] Failed to register FD {}", m_fd);

    } else {
        JOB_LOG_ERROR("[ISocketIO] Failed to register FD {}: Event loop is null", m_fd);
    }
}
*/
void ISocketIO::modifyEvents(threads::IOEvent events)
{
    if (m_fd < 0) {
        JOB_LOG_ERROR("[ISocketIO] modifyEvents called on invalid fd");
        return;
    }
    if (auto loop = m_loop.lock()) {
        if (!loop->modifyFD(m_fd, events))
            JOB_LOG_ERROR("[ISocketIO] Failed to modify FD {}", m_fd);
    } else {
        JOB_LOG_ERROR("[ISocketIO] Failed to modify FD {}: Event loop is null", m_fd);
    }
}

} // namespace job::net

