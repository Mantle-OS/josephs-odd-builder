#pragma once

#include <string>
#include <vector>

#include <pkg/pkg_cache.hpp>
#include <pkg/pkg_package.hpp>
#include <pkg/pkg_file.hpp>
#include <pkg/pkg_depends.hpp>

namespace job::aipkg {

using job::serializer::generated::AiPkgCache;
using job::serializer::generated::AiPkgPackage;
using job::serializer::generated::AiPkgFile;
using job::serializer::generated::AiPkgDepends;

enum class PackageType : uint32_t
{
    Checkpoints = 0,
    Unet        = 1,
    TextEncoder = 2,
    Lora        = 3,
    Embedding   = 4,
    ControlNet  = 5,
    Upscale     = 6,
    Vae         = 7,
    AudioVae    = 8,
};

enum class PackageProvider : uint32_t
{
    HuggingFace = 0,
    GitHub      = 1,
};

struct ValidationError
{
    std::string field;   // Target contextual path alignment (e.g., "packages[0].files[0].sha")
    std::string message; // Specific error string text
};

class JobAiPkgManifest
{
public:
    JobAiPkgManifest() = default;

    struct ValidationError
    {
        std::string field;   // Target contextual path alignment
        std::string message; // Specific error string text
    };

    [[nodiscard]] static std::vector<ValidationError> validate(const AiPkgCache &cache) noexcept;
    [[nodiscard]] static bool isValid(const AiPkgCache &cache) noexcept;
    [[nodiscard]] static std::vector<AiPkgPackage> packagesOfType(const AiPkgCache &cache, PackageType type) noexcept;
};

} // namespace job::aipkg