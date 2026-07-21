#include "job_aipkg_manifest.h"
#include <algorithm>

namespace job::aipkg {

namespace {

[[nodiscard]] std::string typeName(PackageType type) noexcept
{
    switch (type) {
    case PackageType::Checkpoints: return "CHECKPOINTS";
    case PackageType::Unet:        return "UNET";
    case PackageType::TextEncoder: return "TEXTENCODER";
    case PackageType::Lora:        return "LORA";
    case PackageType::Embedding:   return "EMBEDDING";
    case PackageType::ControlNet:  return "CONTROLNET";
    case PackageType::Upscale:     return "UPSCALE";
    case PackageType::Vae:         return "VAE";
    case PackageType::AudioVae:    return "AUDIO_VAE";
    }
    return "UNKNOWN";
}

} // namespace

std::vector<AiPkgPackage> JobAiPkgManifest::packagesOfType(const AiPkgCache &cache, PackageType type) noexcept
{
    std::vector<AiPkgPackage> out;
    for (const auto &pkg : cache.packages)
        if (static_cast<PackageType>(pkg.type) == type)
            out.push_back(pkg);
    return out;
}

/**
 * @brief Performs strict structural verification over an integrated manifest payload.
 *
 * CRITICAL LIFECYCLE NOTE: This verification is explicitly designed to act as a final
 * structural gate executed AFTER files have been fully fetched and hashed (e.g., in the
 * fetch/bring-up pipeline) and IMMEDIATELY BEFORE signing/attestation generation.
 * Running this gate on a raw, freshly-authored manifest will reject valid configurations
 * due to zero-filled file shas and sizes that are completely expected at that stage.
 */
std::vector<JobAiPkgManifest::ValidationError> JobAiPkgManifest::validate(const AiPkgCache &cache) noexcept
{
    std::vector<ValidationError> errors;

    if (cache.project_name.empty())
        errors.push_back({"project_name", "Field is empty but required"});

    if (cache.version.empty())
        errors.push_back({"version", "Field is empty but required"});

    // Verify top-level cryptographic snapshot footprint
    const std::array<uint8_t, 32> zeroSha{};
    if (cache.sha == zeroSha)
        errors.push_back({"sha", "Top-level ledger tracking digest cannot be zero-filled"});

    if (cache.packages.empty()) {
        errors.push_back({"packages", "Cache must contain at least one package descriptor allocation"});
        return errors;
    }

    for (size_t pIdx = 0; pIdx < cache.packages.size(); ++pIdx) {
        const auto &pkg = cache.packages[pIdx];
        std::string const name = typeName(static_cast<PackageType>(pkg.type));

        if (pkg.url.empty())
            errors.push_back({"packages[" + std::to_string(pIdx) + "].url", "Target upstream source URL must be defined"});

        if (pkg.files.empty()) {
            errors.push_back({"packages[" + std::to_string(pIdx) + "]", "Package type " + name + " allocates no targets"});
        } else {
            for (size_t fIdx = 0; fIdx < pkg.files.size(); ++fIdx) {
                const auto &file = pkg.files[fIdx];

                if (file.name.empty())
                    errors.push_back({"packages[" + std::to_string(pIdx) + "].files[" + std::to_string(fIdx) + "].name", "File target designation name cannot be empty"});

                // Gated by enablement state: Unselected or un-fetched variants are expected to be empty.
                if (file.enabled) {
                    if (file.sha == zeroSha) {
                        errors.push_back({"packages[" + std::to_string(pIdx) + "].files[" + std::to_string(fIdx) + "].sha",
                                          "Active target file payload hash cannot be zero-filled"});
                    }
                    if (file.size == 0) {
                        errors.push_back({"packages[" + std::to_string(pIdx) + "].files[" + std::to_string(fIdx) + "].size",
                                          "Active target file cannot declare an unallocated footprint size"});
                    }
                }
            }
        }
    }

    return errors;
}

bool JobAiPkgManifest::isValid(const AiPkgCache &cache) noexcept
{
    return validate(cache).empty();
}

} // namespace job::aipkg