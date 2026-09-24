#include "job_present_queue.h"

#include <utility>

namespace job::render {

bool JobPresentQueue::push(Entry entry)
{
    if (!entry.valid())
        return false;

    m_entries.push_back(std::move(entry));

    return true;
}

JobPresentQueue::Entry JobPresentQueue::pop() noexcept
{
    if (m_entries.empty())
        return {};

    Entry entry = std::move(m_entries.front());
    m_entries.pop_front();
    return entry;
}

}