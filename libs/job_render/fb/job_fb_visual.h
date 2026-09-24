#pragma once

#include <cstdint>

#include <linux/fb.h>

namespace job::render {

enum class FbVisual : std::uint32_t
{
    Mono01              = FB_VISUAL_MONO01,
    Mono10              = FB_VISUAL_MONO10,
    TrueColor           = FB_VISUAL_TRUECOLOR,
    PseudoColor         = FB_VISUAL_PSEUDOCOLOR,
    DirectColor         = FB_VISUAL_DIRECTCOLOR,
    StaticPseudoColor   = FB_VISUAL_STATIC_PSEUDOCOLOR,
    FourCc              = FB_VISUAL_FOURCC
};

class JobFbVisual
{
public:
    using Value = std::uint32_t;

    constexpr JobFbVisual() = default;

    explicit constexpr JobFbVisual(Value value) noexcept :
        m_value(value)
    {
    }

    explicit constexpr JobFbVisual(FbVisual visual) noexcept :
        m_value(static_cast<Value>(visual))
    {
    }

    ~JobFbVisual() = default;

    JobFbVisual(const JobFbVisual &) = default;
    JobFbVisual &operator=(const JobFbVisual &) = default;

    JobFbVisual(JobFbVisual &&) noexcept = default;
    JobFbVisual &operator=(JobFbVisual &&) noexcept = default;

    [[nodiscard]] constexpr Value value() const noexcept
    {
        return m_value;
    }

    [[nodiscard]] constexpr bool is(FbVisual visual) const noexcept
    {
        return m_value == static_cast<Value>(visual);
    }

    [[nodiscard]] constexpr bool mono01() const noexcept
    {
        return is(FbVisual::Mono01);
    }

    [[nodiscard]] constexpr bool mono10() const noexcept
    {
        return is(FbVisual::Mono10);
    }

    [[nodiscard]] constexpr bool trueColor() const noexcept
    {
        return is(FbVisual::TrueColor);
    }

    [[nodiscard]] constexpr bool pseudoColor() const noexcept
    {
        return is(FbVisual::PseudoColor);
    }

    [[nodiscard]] constexpr bool directColor() const noexcept
    {
        return is(FbVisual::DirectColor);
    }

    [[nodiscard]] constexpr bool staticPseudoColor() const noexcept
    {
        return is(FbVisual::StaticPseudoColor);
    }

    [[nodiscard]] constexpr bool fourCc() const noexcept
    {
        return is(FbVisual::FourCc);
    }

    [[nodiscard]] constexpr bool monochrome() const noexcept
    {
        return mono01() || mono10();
    }

    [[nodiscard]] constexpr bool colorMapped() const noexcept
    {
        return pseudoColor() || staticPseudoColor() || directColor();
    }

    [[nodiscard]] constexpr bool known() const noexcept
    {
        return monochrome() || trueColor() || colorMapped() || fourCc();
    }

    [[nodiscard]] friend constexpr bool operator==(const JobFbVisual &, const JobFbVisual &) noexcept = default;

private:
    Value m_value{};
};

}