#pragma once

#include <cstdint>
#include <memory>

namespace job::render {

class JobDrmCapabilities
{
public:
    using Ptr = std::shared_ptr<JobDrmCapabilities>;
    using WPtr = std::weak_ptr<JobDrmCapabilities>;
    using UPtr = std::unique_ptr<JobDrmCapabilities>;

    using Value = std::uint64_t;

    JobDrmCapabilities() = default;
    ~JobDrmCapabilities() = default;

    JobDrmCapabilities(const JobDrmCapabilities &) = default;
    JobDrmCapabilities &operator=(const JobDrmCapabilities &) = default;

    JobDrmCapabilities(JobDrmCapabilities &&) noexcept = default;
    JobDrmCapabilities &operator=(JobDrmCapabilities &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobDrmCapabilities>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobDrmCapabilities>();
    }

    [[nodiscard]] bool query(int fd) noexcept;

    void clear() noexcept;

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_valid;
    }

    [[nodiscard]] constexpr bool dumbBuffer() const noexcept
    {
        return m_dumbBuffer != 0;
    }

    [[nodiscard]] constexpr bool vblankHighCrtc() const noexcept
    {
        return m_vblankHighCrtc != 0;
    }

    [[nodiscard]] constexpr Value dumbPreferredDepth() const noexcept
    {
        return m_dumbPreferredDepth;
    }

    [[nodiscard]] constexpr bool dumbPreferShadow() const noexcept
    {
        return m_dumbPreferShadow != 0;
    }

    [[nodiscard]] constexpr Value prime() const noexcept
    {
        return m_prime;
    }

    [[nodiscard]] constexpr bool primeImport() const noexcept
    {
        return (m_prime & kPrimeImport) != 0;
    }

    [[nodiscard]] constexpr bool primeExport() const noexcept
    {
        return (m_prime & kPrimeExport) != 0;
    }

    [[nodiscard]] constexpr bool timestampMonotonic() const noexcept
    {
        return m_timestampMonotonic != 0;
    }

    [[nodiscard]] constexpr bool asyncPageFlip() const noexcept
    {
        return m_asyncPageFlip != 0;
    }

    [[nodiscard]] constexpr Value cursorWidth() const noexcept
    {
        return m_cursorWidth;
    }

    [[nodiscard]] constexpr Value cursorHeight() const noexcept
    {
        return m_cursorHeight;
    }

    [[nodiscard]] constexpr bool addFb2Modifiers() const noexcept
    {
        return m_addFb2Modifiers != 0;
    }

    [[nodiscard]] constexpr bool pageFlipTarget() const noexcept
    {
        return m_pageFlipTarget != 0;
    }

    [[nodiscard]] constexpr bool crtcInVblankEvent() const noexcept
    {
        return m_crtcInVblankEvent != 0;
    }

    [[nodiscard]] constexpr bool syncobj() const noexcept
    {
        return m_syncobj != 0;
    }

    [[nodiscard]] constexpr bool syncobjTimeline() const noexcept
    {
        return m_syncobjTimeline != 0;
    }

    [[nodiscard]] constexpr bool atomicAsyncPageFlip() const noexcept
    {
        return m_atomicAsyncPageFlip != 0;
    }

private:
    static constexpr Value kPrimeImport = 1ULL << 0;
    static constexpr Value kPrimeExport = 1ULL << 1;

    bool    m_valid{};
    Value   m_dumbBuffer{};
    Value   m_vblankHighCrtc{};
    Value   m_dumbPreferredDepth{};
    Value   m_dumbPreferShadow{};
    Value   m_prime{};
    Value   m_timestampMonotonic{};
    Value   m_asyncPageFlip{};
    Value   m_cursorWidth{};
    Value   m_cursorHeight{};
    Value   m_addFb2Modifiers{};
    Value   m_pageFlipTarget{};
    Value   m_crtcInVblankEvent{};
    Value   m_syncobj{};
    Value   m_syncobjTimeline{};
    Value   m_atomicAsyncPageFlip{};
};

}