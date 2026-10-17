#include "job_render_application_args.h"
#ifndef NDEBUG
#include <job_logger.h>
#endif
namespace job::render {

JobRenderApplicationArgs::JobRenderApplicationArgs() = default;

JobRenderApplicationArgs::JobRenderApplicationArgs(int argc, char *argv[])
{
    if(!parse(argc, argv)){
#ifndef NDEBUG
        JOB_LOG_DEBUG("[JobRenderApplicationArgs] Unable to parse arguments");
#endif
    }
}

JobRenderApplicationArgs::~JobRenderApplicationArgs() = default;

bool JobRenderApplicationArgs::parse(int argc, char *argv[])
{
    m_valid = true;
    m_backend = RenderBackend::Default;
    m_device.clear();
    m_seat = "seat0";
    m_error.clear();
    m_help = false;
    m_version = false;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            m_help = true;
            continue;
        }

        if (arg == "--version") {
            m_version = true;
            continue;
        }

        if (arg == "--backend") {
            if (i + 1 >= argc) {
                m_valid = false;
                m_error = "Missing value for --backend.";
                return false;
            }

            if (!parseBackend(argv[++i]))
                return false;

            continue;
        }

        if (arg == "--device") {
            if (i + 1 >= argc) {
                m_valid = false;
                m_error = "Missing value for --device.";
                return false;
            }

            m_device = argv[++i];
            continue;
        }

        if (arg == "--seat") {
            if (i + 1 >= argc) {
                m_valid = false;
                m_error = "Missing value for --seat.";
                return false;
            }

            m_seat = argv[++i];

            if (m_seat.empty()) {
                m_valid = false;
                m_error = "Seat cannot be empty.";
                return false;
            }

            continue;
        }

        m_valid = false;
        m_error = "Unknown argument: ";
        m_error += arg;
        return false;
    }

    return true;
}

bool JobRenderApplicationArgs::valid() const noexcept
{
    return m_valid;
}

RenderBackend JobRenderApplicationArgs::backend() const noexcept
{
    return m_backend;
}

const std::string &JobRenderApplicationArgs::device() const noexcept
{
    return m_device;
}

const std::string &JobRenderApplicationArgs::seat() const noexcept
{
    return m_seat;
}

bool JobRenderApplicationArgs::help() const noexcept
{
    return m_help;
}

bool JobRenderApplicationArgs::version() const noexcept
{
    return m_version;
}

const std::string &JobRenderApplicationArgs::error() const noexcept
{
    return m_error;
}

std::string_view JobRenderApplicationArgs::backendName(RenderBackend backend) noexcept
{
    switch (backend) {
    case RenderBackend::Default:
        return "default";
    case RenderBackend::Kms:
        return "kms";
    case RenderBackend::KmsSimple:
        return "kms-simple";
    case RenderBackend::Framebuffer:
        return "framebuffer";
    }

    return "unknown";
}

std::string JobRenderApplicationArgs::helpText()
{
    return
        "Joseph's Odd Builder Render\n"
        "\n"
        "Usage:\n"
        "  application [options]\n"
        "\n"
        "Options:\n"
        "  -h, --help                 Show this help text.\n"
        "      --version              Show version information.\n"
        "      --backend <backend>    Select render backend.\n"
        "                             Valid values: kms, kms-simple, framebuffer, fb.\n"
        "      --device <path>        Override the render device.\n"
        "      --seat <seat>          Select the input seat. Default: seat0.\n";
}

std::string JobRenderApplicationArgs::versionText()
{
    return "Joseph's Odd Builder Render 0.1";
}

bool JobRenderApplicationArgs::parseBackend(std::string_view value)
{
    if (value == "kms") {
        m_backend = RenderBackend::Kms;
        return true;
    }

    if (value == "kms-simple") {
        m_backend = RenderBackend::KmsSimple;
        return true;
    }

    if (value == "framebuffer" || value == "fb") {
        m_backend = RenderBackend::Framebuffer;
        return true;
    }

    m_valid = false;
    m_error = "Unknown render backend: ";
    m_error += value;

    return false;
}

}