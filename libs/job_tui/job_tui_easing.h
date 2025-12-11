#pragma once

#include <cmath>
#include <algorithm>

namespace job::tui {

enum class EasingType {
    Linear,
    InQuad,
    OutQuad,
    InOutQuad,
    OutInQuad,
    InCubic,
    OutCubic,
    InOutCubic,
    OutInCubic,
    InQuart,
    OutQuart,
    InOutQuart,
    OutInQuart,
    InQuint,
    OutQuint,
    InOutQuint,
    OutInQuint,
    InSine,
    OutSine,
    InOutSine,
    OutInSine,
    InExpo,
    OutExpo,
    InOutExpo,
    OutInExpo,
    InCirc,
    OutCirc,
    InOutCirc,
    OutInCirc,
    InElastic,
    OutElastic,
    InOutElastic,
    OutInElastic,
    InBack,
    OutBack,
    InOutBack,
    OutInBack,
    InBounce,
    OutBounce,
    InOutBounce,
    OutInBounce,
    BezierSpline // to be implemented separately
};

inline float bounceOut(float t)
{
    if (t < (1 / 2.75f)) {
        return 7.5625f * t * t;
    } else if (t < (2 / 2.75f)) {
        t -= (1.5f / 2.75f);
        return 7.5625f * t * t + 0.75f;
    } else if (t < (2.5 / 2.75f)) {
        t -= (2.25f / 2.75f);
        return 7.5625f * t * t + 0.9375f;
    } else {
        t -= (2.625f / 2.75f);
        return 7.5625f * t * t + 0.984375f;
    }
}

inline float bounceIn(float t)
{
    return 1.0f - bounceOut(1.0f - t);
}

inline float bounceInOut(float t)
{
    return (t < 0.5f) ? (0.5f * bounceIn(t * 2.0f)) :
               (0.5f * bounceOut(t * 2.0f - 1.0f) + 0.5f);
}

inline float backIn(float t, float s)
{
    return t * t * ((s + 1) * t - s);
}

inline float backOut(float t, float s)
{
    t -= 1.0f;
    return t * t * ((s + 1) * t + s) + 1.0f;
}

inline float backInOut(float t, float s)
{
    s *= 1.525f;
    if (t < 0.5f) {
        return 0.5f * (t * 2) * (t * 2) * ((s + 1) * (t * 2) - s);
    } else {
        float p = t * 2 - 2;
        return 0.5f * (p * p * ((s + 1) * p + s) + 2);
    }
}

inline float elasticIn(float t, float a, float p)
{
    if (t == 0 || t == 1)
        return t;
    float s = p / 4.0f;
    t -= 1.0f;
    return -(a * std::pow(2.0f, 10.0f * t) * std::sin((t - s) * (2.0f * M_PI) / p));
}

inline float elasticOut(float t, float a, float p)
{
    if (t == 0 || t == 1)
        return t;
    float s = p / 4.0f;
    return a * std::pow(2.0f, -10.0f * t) * std::sin((t - s) * (2.0f * M_PI) / p) + 1.0f;
}

inline float elasticInOut(float t, float a, float p)
{
    if (t == 0 || t == 1)
        return t;
    float s = p / 4.0f;
    t = t * 2 - 1;
    if (t < 0)
        return -0.5f * (a * std::pow(2.0f, 10.0f * t) * std::sin((t - s) * (2.0f * M_PI) / p));
    else
        return 0.5f * (a * std::pow(2.0f, -10.0f * t) * std::sin((t - s) * (2.0f * M_PI) / p)) + 1.0f;
}

inline float applyEasing(EasingType type, float t, float amplitude = 1.0f, float period = 0.3f, float overshoot = 1.70158f)
{
    t = std::clamp(t, 0.0f, 1.0f);
    switch (type) {
    case EasingType::Linear:
        return t;
    case EasingType::InQuad:
        return t * t;
    case EasingType::OutQuad:
        return t * (2 - t);
    case EasingType::InOutQuad:
        return t < 0.5f ? 2 * t * t : -1 + (4 - 2 * t) * t;
    case EasingType::OutInQuad:
        return t < 0.5f ? applyEasing(EasingType::OutQuad, t * 2) * 0.5f
                        : applyEasing(EasingType::InQuad, t * 2 - 1) * 0.5f + 0.5f;
    case EasingType::InCubic:
        return t * t * t;
    case EasingType::OutCubic: {
        float u = t - 1;
        return u * u * u + 1;
    }
    case EasingType::InOutCubic:
        return t < 0.5f ? 4 * t * t * t : (t - 1) * (2 * t - 2) * (2 * t - 2) + 1;
    case EasingType::OutInCubic:
        return t < 0.5f ? applyEasing(EasingType::OutCubic, t * 2) * 0.5f
                        : applyEasing(EasingType::InCubic, t * 2 - 1) * 0.5f + 0.5f;
    case EasingType::InQuart:
        return t * t * t * t;
    case EasingType::OutQuart: {
        float u = t - 1;
        return 1 - u * u * u * u;
    }
    case EasingType::InOutQuart: {
        if (t < 0.5f) {
            return 8.0f * t * t * t * t;
        } else {
            float u = t - 1.0f;
            return 1.0f - 8.0f * u * u * u * u;
        }
    }
    case EasingType::OutInQuart:
        return t < 0.5f ? applyEasing(EasingType::OutQuart, t * 2) * 0.5f
                        : applyEasing(EasingType::InQuart, t * 2 - 1) * 0.5f + 0.5f;
    case EasingType::InQuint:
        return t * t * t * t * t;
    case EasingType::OutQuint: {
        float u = t - 1;
        return 1 + u * u * u * u * u;
    }
    case EasingType::InOutQuint: {
        if (t < 0.5f) {
            return 16.0f * t * t * t * t * t;
        } else {
            float u = t - 1.0f;
            return 1.0f + 16.0f * u * u * u * u * u;
        }
    }
    case EasingType::OutInQuint:
        return t < 0.5f ? applyEasing(EasingType::OutQuint, t * 2) * 0.5f
                        : applyEasing(EasingType::InQuint, t * 2 - 1) * 0.5f + 0.5f;
    case EasingType::InSine:
        return 1 - std::cos(t * M_PI_2);
    case EasingType::OutSine:
        return std::sin(t * M_PI_2);
    case EasingType::InOutSine:
        return -0.5f * (std::cos(M_PI * t) - 1);
    case EasingType::OutInSine:
        return t < 0.5f ? applyEasing(EasingType::OutSine, t * 2) * 0.5f
                        : applyEasing(EasingType::InSine, t * 2 - 1) * 0.5f + 0.5f;
    case EasingType::InExpo:
        return t == 0 ? 0 : std::pow(2, 10 * (t - 1));
    case EasingType::OutExpo:
        return t == 1 ? 1 : 1 - std::pow(2, -10 * t);
    case EasingType::InOutExpo: {
        if (t == 0)
            return 0;

        if (t == 1)
            return 1;

        return t < 0.5f ? std::pow(2, 20 * t - 10) * 0.5f :
                   (2 - std::pow(2, -20 * t + 10)) * 0.5f;
    }
    case EasingType::OutInExpo:
        return t < 0.5f ? applyEasing(EasingType::OutExpo, t * 2) * 0.5f
                        : applyEasing(EasingType::InExpo, t * 2 - 1) * 0.5f + 0.5f;
    case EasingType::InCirc:
        return 1 - std::sqrt(1 - t * t);
    case EasingType::OutCirc: {
        float u = t - 1;
        return std::sqrt(1 - u * u);
    }
    case EasingType::InOutCirc:
        if (t < 0.5f)
            return 0.5f * (1 - std::sqrt(1 - 4 * t * t));
        t = 2 * t - 1;
        return 0.5f * (std::sqrt(1 - t * t) + 1);
    case EasingType::OutInCirc:
        return t < 0.5f ? applyEasing(EasingType::OutCirc, t * 2) * 0.5f
                        : applyEasing(EasingType::InCirc, t * 2 - 1) * 0.5f + 0.5f;
    case EasingType::InElastic:
        return elasticIn(t, amplitude, period);
    case EasingType::OutElastic:
        return elasticOut(t, amplitude, period);
    case EasingType::InOutElastic:
        return elasticInOut(t, amplitude, period);
    case EasingType::OutInElastic:
        return t < 0.5f ? elasticOut(t * 2, amplitude, period) * 0.5f
                        : elasticIn(t * 2 - 1, amplitude, period) * 0.5f + 0.5f;
    case EasingType::InBack:
        return backIn(t, overshoot);
    case EasingType::OutBack:
        return backOut(t, overshoot);
    case EasingType::InOutBack:
        return backInOut(t, overshoot);
    case EasingType::OutInBack:
        return t < 0.5f ? backOut(t * 2, overshoot) * 0.5f :
                   backIn(t * 2 - 1, overshoot) * 0.5f + 0.5f;
    case EasingType::InBounce:
        return bounceIn(t);
    case EasingType::OutBounce:
        return bounceOut(t);
    case EasingType::InOutBounce:
        return bounceInOut(t);
    case EasingType::OutInBounce:
        return t < 0.5f ? bounceOut(t * 2) * 0.5f
                        : bounceIn(t * 2 - 1) * 0.5f + 0.5f;
    case EasingType::BezierSpline:
        // FIXME: To be implemented via control point lookup
        return t;
    }

    return t;
}

}
