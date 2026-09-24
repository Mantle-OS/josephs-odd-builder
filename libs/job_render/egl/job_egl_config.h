#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

#include <EGL/egl.h>

namespace job::render {

class JobEglConfig
{
public:
    using Ptr  = std::shared_ptr<JobEglConfig>;
    using WPtr = std::weak_ptr<JobEglConfig>;
    using UPtr = std::unique_ptr<JobEglConfig>;

    using Id        = EGLint;
    using Attribute = EGLint;
    using Value     = EGLint;

    struct Requirements
    {
        Value redSize{8};
        Value greenSize{8};
        Value blueSize{8};
        Value alphaSize{};
        Value depthSize{};
        Value stencilSize{};
        Value sampleBuffers{};
        Value samples{};

        Value surfaceType{EGL_WINDOW_BIT};
        Value renderableType{EGL_OPENGL_BIT};

        std::optional<Value> nativeVisualId;

        [[nodiscard]] constexpr bool valid() const noexcept
        {
            return redSize >= 0 &&
                   greenSize >= 0 &&
                   blueSize >= 0 &&
                   alphaSize >= 0 &&
                   depthSize >= 0 &&
                   stencilSize >= 0 &&
                   sampleBuffers >= 0 &&
                   samples >= 0 &&
                   surfaceType != 0 &&
                   renderableType != 0;
        }
    };

    JobEglConfig() = default;

    explicit JobEglConfig(EGLConfig config) noexcept :
        m_config(config)
    {
    }

    ~JobEglConfig() = default;

    JobEglConfig(const JobEglConfig &) = default;
    JobEglConfig &operator=(const JobEglConfig &) = default;

    JobEglConfig(JobEglConfig &&) noexcept = default;
    JobEglConfig &operator=(JobEglConfig &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobEglConfig>();
    }

    [[nodiscard]] static Ptr createShared(EGLConfig config)
    {
        return std::make_shared<JobEglConfig>(config);
    }

    [[nodiscard]] static Ptr createShared(const JobEglConfig &config)
    {
        return std::make_shared<JobEglConfig>(config);
    }

    [[nodiscard]] static Ptr createShared(JobEglConfig &&config)
    {
        return std::make_shared<JobEglConfig>(std::move(config));
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobEglConfig>();
    }

    [[nodiscard]] static UPtr createUniq(EGLConfig config)
    {
        return std::make_unique<JobEglConfig>(config);
    }

    [[nodiscard]] static UPtr createUniq(const JobEglConfig &config)
    {
        return std::make_unique<JobEglConfig>(config);
    }

    [[nodiscard]] static UPtr createUniq(JobEglConfig &&config)
    {
        return std::make_unique<JobEglConfig>(std::move(config));
    }

    [[nodiscard]] static std::optional<JobEglConfig> select(
        EGLDisplay display,
        const Requirements &requirements);

    [[nodiscard]] bool query(EGLDisplay display);

    [[nodiscard]] bool matches(const Requirements &requirements) const noexcept;

    [[nodiscard]] EGLConfig config() const noexcept
    {
        return m_config;
    }

    [[nodiscard]] constexpr Id id() const noexcept
    {
        return m_id;
    }

    [[nodiscard]] constexpr Value redSize() const noexcept
    {
        return m_redSize;
    }

    [[nodiscard]] constexpr Value greenSize() const noexcept
    {
        return m_greenSize;
    }

    [[nodiscard]] constexpr Value blueSize() const noexcept
    {
        return m_blueSize;
    }

    [[nodiscard]] constexpr Value alphaSize() const noexcept
    {
        return m_alphaSize;
    }

    [[nodiscard]] constexpr Value depthSize() const noexcept
    {
        return m_depthSize;
    }

    [[nodiscard]] constexpr Value stencilSize() const noexcept
    {
        return m_stencilSize;
    }

    [[nodiscard]] constexpr Value bufferSize() const noexcept
    {
        return m_bufferSize;
    }

    [[nodiscard]] constexpr Value samples() const noexcept
    {
        return m_samples;
    }

    [[nodiscard]] constexpr Value sampleBuffers() const noexcept
    {
        return m_sampleBuffers;
    }

    [[nodiscard]] constexpr Value surfaceType() const noexcept
    {
        return m_surfaceType;
    }

    [[nodiscard]] constexpr Value renderableType() const noexcept
    {
        return m_renderableType;
    }

    [[nodiscard]] constexpr Value conformant() const noexcept
    {
        return m_conformant;
    }

    [[nodiscard]] constexpr Value nativeVisualId() const noexcept
    {
        return m_nativeVisualId;
    }

    [[nodiscard]] constexpr Value nativeVisualType() const noexcept
    {
        return m_nativeVisualType;
    }

    [[nodiscard]] constexpr Value configCaveat() const noexcept
    {
        return m_configCaveat;
    }

    [[nodiscard]] constexpr bool windowSurface() const noexcept
    {
        return (m_surfaceType & EGL_WINDOW_BIT) != 0;
    }

    [[nodiscard]] constexpr bool pbufferSurface() const noexcept
    {
        return (m_surfaceType & EGL_PBUFFER_BIT) != 0;
    }

    [[nodiscard]] constexpr bool pixmapSurface() const noexcept
    {
        return (m_surfaceType & EGL_PIXMAP_BIT) != 0;
    }

    [[nodiscard]] constexpr bool openGl() const noexcept
    {
        return (m_renderableType & EGL_OPENGL_BIT) != 0;
    }

    [[nodiscard]] constexpr bool openGlEs2() const noexcept
    {
        return (m_renderableType & EGL_OPENGL_ES2_BIT) != 0;
    }

#ifdef EGL_OPENGL_ES3_BIT
    [[nodiscard]] constexpr bool openGlEs3() const noexcept
    {
        return (m_renderableType & EGL_OPENGL_ES3_BIT) != 0;
    }
#endif

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_config != nullptr && m_queried;
    }

private:
    EGLConfig m_config{};
    bool m_queried{};

    Id m_id{};

    Value m_redSize{};
    Value m_greenSize{};
    Value m_blueSize{};
    Value m_alphaSize{};

    Value m_depthSize{};
    Value m_stencilSize{};
    Value m_bufferSize{};

    Value m_samples{};
    Value m_sampleBuffers{};

    Value m_surfaceType{};
    Value m_renderableType{};
    Value m_conformant{};

    Value m_nativeVisualId{};
    Value m_nativeVisualType{};
    Value m_configCaveat{EGL_NONE};
};

} // namespace job::render