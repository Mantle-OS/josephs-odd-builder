#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace job::render {

enum class RenderBackend : std::uint8_t
{
    Default,
    Kms,
    KmsSimple,
    Framebuffer
};

class JobRenderApplicationArgs
{
public:
    using Ptr  = std::shared_ptr<JobRenderApplicationArgs>;
    using WPtr = std::weak_ptr<JobRenderApplicationArgs>;
    using UPtr = std::unique_ptr<JobRenderApplicationArgs>;

    JobRenderApplicationArgs();
    JobRenderApplicationArgs(int argc, char *argv[]);
    ~JobRenderApplicationArgs();

    JobRenderApplicationArgs(const JobRenderApplicationArgs &) = default;
    JobRenderApplicationArgs &operator=(const JobRenderApplicationArgs &) = default;

    JobRenderApplicationArgs(JobRenderApplicationArgs &&) noexcept = default;
    JobRenderApplicationArgs &operator=(JobRenderApplicationArgs &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobRenderApplicationArgs>();
    }

    [[nodiscard]] static Ptr createShared(int argc, char *argv[])
    {
        return std::make_shared<JobRenderApplicationArgs>(argc, argv);
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobRenderApplicationArgs>();
    }

    [[nodiscard]] static UPtr createUniq(int argc, char *argv[])
    {
        return std::make_unique<JobRenderApplicationArgs>(argc, argv);
    }

    [[nodiscard]] bool parse(int argc, char *argv[]);

    [[nodiscard]] bool valid() const noexcept;

    [[nodiscard]] RenderBackend backend() const noexcept;
    [[nodiscard]] const std::string &device() const noexcept;
    [[nodiscard]] const std::string &seat() const noexcept;

    [[nodiscard]] bool help() const noexcept;
    [[nodiscard]] bool version() const noexcept;

    [[nodiscard]] const std::string &error() const noexcept;

    [[nodiscard]] static std::string_view backendName(RenderBackend backend) noexcept;
    [[nodiscard]] static std::string helpText();
    [[nodiscard]] static std::string versionText();

private:
    [[nodiscard]] bool parseBackend(std::string_view value);

    bool m_valid{true};

    RenderBackend m_backend{RenderBackend::Default};

    std::string m_device;
    std::string m_seat{"seat0"};
    std::string m_error;

    bool m_help{false};
    bool m_version{false};
};

}