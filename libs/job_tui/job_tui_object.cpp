#include "job_tui_object.h"
#include <job_io_async_thread.h>
#include <iostream>

using namespace job::tui;

JobTuiObject::JobTuiObject(JobTuiObject *parent) :
    m_parent{parent}
{
    m_id = generateRandomId();
    m_threadId = std::this_thread::get_id();
}

JobTuiObject::~JobTuiObject()
{
    stopTimer();
}

void JobTuiObject::setId(const std::string &id)
{
    if (!m_id.empty())
        throw std::logic_error("Cannot reassign object ID once set.");
    m_id = id;
}

std::string JobTuiObject::id() const
{
    return m_id;
}

void JobTuiObject::addChild(JobTuiObject::Ptr child)
{
    if (!child)
        return;

    if (child->id().empty()) {
        std::cerr << "Child object must have a non-empty id\n";
        return;
    }
    std::scoped_lock lock(m_childMutex);
    m_children[child->id()] = std::move(child);
}

void JobTuiObject::removeChild(JobTuiObject *child)
{
    if (!child)
        return;

    removeChild(child->id());
}

void JobTuiObject::removeChild(const std::string &id)
{
    std::scoped_lock lock(m_childMutex);
    auto it = m_children.find(id);
    if (it != m_children.end()) {
        it->second->setParent(nullptr);
        m_children.erase(it);
    }
}

std::shared_ptr<JobTuiObject> JobTuiObject::findChild(const std::string &id)
{
    std::scoped_lock lock(m_childMutex);
    auto it = m_children.find(id);
    return (it != m_children.end()) ? it->second : nullptr;
}

JobTuiObject *JobTuiObject::findChild(JobTuiObject *obj)
{
    std::scoped_lock lock(m_childMutex);
    for (const auto &[_, child] : m_children)
        if (child.get() == obj)
            return child.get();

    return nullptr;
}

JobTuiObject::Ptr JobTuiObject::findChildRecursive(const std::string &id)
{
    std::scoped_lock lock(m_childMutex);
    auto it = m_children.find(id);
    if (it != m_children.end())
        return it->second;

    for (auto &[_, child] : m_children) {
        auto res = child->findChildRecursive(id);
        if (res)
            return res;
    }
    return nullptr;
}

std::unordered_map<std::string, JobTuiObject::Ptr> &JobTuiObject::children()
{
    return m_children;
}

std::vector<JobTuiObject*> JobTuiObject::children() const
{
    std::scoped_lock lock(m_childMutex);
    std::vector<JobTuiObject *> list;

    for (const auto &[_, child] : m_children)
        if (child)
            list.push_back(child.get());

    return list;
}

void JobTuiObject::debugInfo() const
{
    std::cout << "JobTuiObject: id=" << m_id
              << " thread_id=" << m_threadId
              << " hasTimer=" << m_timerActive.load() << "\n";
}

void JobTuiObject::debugChildren(int depth) const
{
    std::scoped_lock lock(m_childMutex);
    std::string indent(depth * 2, ' ');
    for (const auto &[id, child] : m_children) {
        std::cout << indent << "Child: " << id << "\n";
        child->debugChildren(depth + 1);
    }
}

bool JobTuiObject::hasTimer() const
{
    return m_timerActive.load();
}

bool JobTuiObject::stopTimer()
{
    m_timerActive.store(false);
    ++m_timerToken;
    return true;
}

std::chrono::milliseconds JobTuiObject::heartbeat() const
{
    return m_heartbeat;
}

void JobTuiObject::setHeartbeat(std::chrono::milliseconds hb)
{
    m_heartbeat = hb;
}

bool JobTuiObject::startTimer(std::chrono::milliseconds hb)
{
    if (m_timerActive.load())
        return false;

    m_heartbeat = hb;
    m_timerActive.store(true);

    uint64_t thisToken = ++m_timerToken;

    threads::JobIoAsyncThread::globalLoop().post([this, thisToken]() {
        if (!m_timerActive.load() || m_timerToken.load() != thisToken)
            return;

        if (m_timerCallback) {
            try {
                m_timerCallback();
            } catch (const std::exception &e) {
                std::cerr << "Timer callback error: " << e.what() << "\n";
            }
        }

        if (m_timerActive.load() && m_timerToken.load() == thisToken)
            startTimer(m_heartbeat);
    }, 1);

    return true;
}

bool JobTuiObject::startTimer(int msec)
{
    return startTimer(std::chrono::milliseconds(msec));
}

void JobTuiObject::handleEvent(const Event &event)
{
    if (onEvent)
        onEvent(event);

    std::scoped_lock lock(m_childMutex);
    for (auto &[_, child] : m_children)
        child->handleEvent(event);
}

bool JobTuiObject::moveToThread(std::thread::id threadId)
{
    if (m_parent)
        return false;

    m_threadId = threadId;
    std::scoped_lock lock(m_childMutex);
    for (auto &[_, child] : m_children)
        child->moveToThread(threadId);

    return true;
}

std::thread::id JobTuiObject::thread() const
{
    return m_threadId;
}

JobTuiObject *JobTuiObject::parent() const
{
    return m_parent;
}

void JobTuiObject::setParent(JobTuiObject *parent)
{
    m_parent = parent;
}

void JobTuiObject::setTimerCallback(Callback cb)
{
    m_timerCallback = std::move(cb);
}
