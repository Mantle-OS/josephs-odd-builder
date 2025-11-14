#pragma once

#include <cstdint>
#include <memory>

// This is meant to be a interface !!

namespace job::threads  {
class JobIDescriptor {
public:
    using Ptr = std::shared_ptr<JobIDescriptor>;
    virtual ~JobIDescriptor() = default;

    uint64_t id() const;
    void setId(uint64_t id);

    int priority() const;
    void setPriority(int priority);

    uint64_t size() const;
    void setSize(uint64_t size);

    uint32_t hash() const;
    void setHash(uint32_t hash);

    double latency() const;
    void setLatency(double latency);

    double bps() const;
    void setBps(double bps);

protected:
    JobIDescriptor() = default;

private:
    uint64_t    m_id{0};
    int         m_priority{0};
    uint64_t    m_size{0};
    uint32_t    m_hash{0};
    double      m_latency{0.0};
    double      m_bps{0.0};
};
} // namespace job::threads

// CHECKPOINT: v1.1
