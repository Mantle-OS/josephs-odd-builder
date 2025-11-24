#pragma once

#include <cstdint>
#include <memory>

// This is meant to be a inhherited(always) !!

namespace job::threads  {
class JobIDescriptor {
public:
    using Ptr = std::shared_ptr<JobIDescriptor>;
    virtual ~JobIDescriptor() = default;

    [[nodiscard]] uint64_t id() const noexcept;
    void setId(uint64_t id) noexcept;

    [[nodiscard]] int priority() const noexcept;
    void setPriority(int priority) noexcept;

    [[nodiscard]] uint64_t size() const noexcept;
    void setSize(uint64_t size) noexcept;

    [[nodiscard]] uint32_t hash() const noexcept;
    void setHash(uint32_t hash) noexcept;

    [[nodiscard]] double latency() const noexcept;
    void setLatency(double latency) noexcept;

    [[nodiscard]] double bps() const noexcept;
    void setBps(double bps) noexcept;

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
// CHECKPOINT: v1.2
