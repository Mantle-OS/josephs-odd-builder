#pragma once

#include <random>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

#include "job_io_async_thread.h"
#include "job_tui_event.h"
namespace job::tui {

// FIXME use JobRandom
inline std::string generateRandomId()
{
    using namespace std::chrono;
    auto seed = duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<uint64_t> dist;

    uint64_t value = dist(rng);

    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << value;
    return ss.str();
}

class JobTuiItem;

class JobTuiObject {
public:
    using Ptr = std::shared_ptr<JobTuiObject>;
    using Callback = std::function<void()>;

    explicit JobTuiObject(JobTuiObject *parent = nullptr);
    virtual ~JobTuiObject();

    void setId(const std::string &id);
    std::string id() const;
    void debugInfo() const;

    // CHILDREN
    virtual void addChild(JobTuiObject::Ptr child);
    virtual void removeChild(JobTuiObject *child);
    virtual void removeChild(const std::string &id);
    JobTuiObject::Ptr findChild(const std::string &id);
    JobTuiObject *findChild(JobTuiObject *obj);
    JobTuiObject::Ptr findChildRecursive(const std::string &id);
    std::unordered_map<std::string, JobTuiObject::Ptr> &children();
    std::vector<JobTuiObject*> children() const;

    template<typename T>
    std::vector<T*> childrenAs() const
    {
        std::vector<T*> list;
        for (const auto &[_, child] : m_children)
            if (auto casted = dynamic_cast<T *>(child.get()))
                list.push_back(casted);

        return list;
    }

    void debugChildren(int depth = 0) const;

    // TIMER(FIXME use JobTimer)
    bool hasTimer() const;
    bool stopTimer();
    std::chrono::milliseconds heartbeat() const;
    void setHeartbeat(std::chrono::milliseconds hb);
    bool startTimer(std::chrono::milliseconds hb);
    bool startTimer(int msec);

    // EVENTS
    virtual void handleEvent(const Event &event);
    std::function<void(const Event &event)> onEvent;

    bool moveToThread(std::thread::id threadId);
    std::thread::id thread() const;

    JobTuiObject *parent() const;
    void setParent(JobTuiObject *parent);

    void setTimerCallback(Callback cb);

protected:
    std::string                                             m_id;
    JobTuiObject                                            *m_parent = nullptr;
    std::unordered_map<std::string, JobTuiObject::Ptr>      m_children = {};
    std::chrono::milliseconds                               m_heartbeat{1000};
    Callback                                                m_timerCallback = nullptr;
    std::atomic<bool>                                       m_timerActive{false};
    std::atomic<uint64_t>                                   m_timerToken{0}; // 🔒 Used to avoid timer overlap
    std::thread::id                                         m_threadId;
    mutable std::mutex                                      m_childMutex;
};

}
