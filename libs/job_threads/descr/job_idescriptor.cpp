#include "job_idescriptor.h"

namespace job::threads {

uint64_t JobIDescriptor::id() const noexcept
{
    return m_id;
}

void JobIDescriptor::setId(uint64_t id) noexcept
{
    m_id = id;
}

int JobIDescriptor::priority() const noexcept
{
    return m_priority;
}

void JobIDescriptor::setPriority(int priority) noexcept
{
    m_priority = priority;
}

uint64_t JobIDescriptor::size() const noexcept
{
    return m_size;
}

void JobIDescriptor::setSize(uint64_t size) noexcept
{
    m_size = size;
}

uint32_t JobIDescriptor::hash() const noexcept
{
    return m_hash;
}

void JobIDescriptor::setHash(uint32_t hash) noexcept
{
    m_hash = hash;
}

double JobIDescriptor::latency() const noexcept
{
    return m_latency;
}

void JobIDescriptor::setLatency(double latency) noexcept
{
    m_latency = latency;
}

double JobIDescriptor::bps() const noexcept
{
    return m_bps;
}

void JobIDescriptor::setBps(double bps) noexcept
{
    m_bps = bps;
}

}

