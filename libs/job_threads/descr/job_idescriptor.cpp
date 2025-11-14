#include "job_idescriptor.h"

namespace job::threads {

uint64_t JobIDescriptor::id() const
{
    return m_id;
}

void JobIDescriptor::setId(uint64_t id)
{
    m_id = id;
}

int JobIDescriptor::priority() const
{
    return m_priority;
}

void JobIDescriptor::setPriority(int priority)
{
    m_priority = priority;
}

uint64_t JobIDescriptor::size() const
{
    return m_size;
}

void JobIDescriptor::setSize(uint64_t size)
{
    m_size = size;
}

uint32_t JobIDescriptor::hash() const
{
    return m_hash;
}

void JobIDescriptor::setHash(uint32_t hash)
{
    m_hash = hash;
}

double JobIDescriptor::latency() const
{
    return m_latency;
}

void JobIDescriptor::setLatency(double latency)
{
    m_latency = latency;
}

double JobIDescriptor::bps() const
{
    return m_bps;
}

void JobIDescriptor::setBps(double bps)
{
    m_bps = bps;
}

}

// CHECKPOINT: v1.0
