#pragma once

#include <cstdint>

#include "job_drm_mode.h"

namespace job::render {

struct JobPresentRestore
{
    std::uint64_t connectorCrtcId{};
    std::uint64_t crtcActive{};

    std::uint64_t planeFramebufferId{};
    std::uint64_t planeCrtcId{};

    std::uint64_t planeSourceX{};
    std::uint64_t planeSourceY{};
    std::uint64_t planeSourceWidth{};
    std::uint64_t planeSourceHeight{};

    std::uint64_t planeCrtcX{};
    std::uint64_t planeCrtcY{};
    std::uint64_t planeCrtcWidth{};
    std::uint64_t planeCrtcHeight{};

    bool modeValid{};
    JobDrmMode mode;
};

}