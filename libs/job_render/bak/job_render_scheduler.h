#pragma once

#include <cstdint>

namespace job::render {

enum class RenderFrameState : std::uint8_t
{
    Idle,
    Dirty,
    Rendering,
    Ready,
    PresentPending
};

class JobRenderScheduler
{
public:
    JobRenderScheduler() = default;
    ~JobRenderScheduler() = default;

    JobRenderScheduler(const JobRenderScheduler &) = delete;
    JobRenderScheduler &operator=(const JobRenderScheduler &) = delete;
    JobRenderScheduler(JobRenderScheduler &&) = delete;
    JobRenderScheduler &operator=(JobRenderScheduler &&) = delete;

    [[nodiscard]] RenderFrameState state() const noexcept
    {
        return m_state;
    }

    [[nodiscard]] bool dirty() const noexcept
    {
        return m_state == RenderFrameState::Dirty;
    }

    [[nodiscard]] bool canRender() const noexcept
    {
        return m_state == RenderFrameState::Dirty;
    }

    [[nodiscard]] bool presentPending() const noexcept
    {
        return m_state == RenderFrameState::PresentPending;
    }

    void markDirty() noexcept
    {
        if (m_state == RenderFrameState::Idle)
            m_state = RenderFrameState::Dirty;
    }

    void beginRender() noexcept
    {
        m_state = RenderFrameState::Rendering;
    }

    void endRender() noexcept
    {
        m_state = RenderFrameState::Ready;
    }

    void beginPresent() noexcept
    {
        m_state = RenderFrameState::PresentPending;
    }

    void presented() noexcept
    {
        m_state = RenderFrameState::Idle;
    }

private:
    RenderFrameState m_state{RenderFrameState::Idle};
};

} // namespace job::render