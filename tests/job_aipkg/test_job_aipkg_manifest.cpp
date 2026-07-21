#include <catch2/catch_test_macros.hpp>
#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif
#include <string>
#include <vector>
#include <array>

#include <job_aipkg_manifest.h>

using namespace job::aipkg;

namespace {

[[nodiscard]] AiPkgCache makeValidMockCache() noexcept
{
    AiPkgCache cache{};
    cache.project_name = "TurboVision-OS";
    cache.version = "1.2.0";
    cache.sha.fill(0xAA);
    cache.sections = {"txt2img", "low_vram"};

    AiPkgFile file{};
    file.name = "model.safetensors";
    file.enabled = true;
    file.size = 4200000000ULL; // 4.2 GB footprint
    file.sha.fill(0xBB);

    AiPkgPackage pkg{};
    pkg.type = static_cast<uint32_t>(PackageType::Unet);
    pkg.provider = static_cast<uint32_t>(PackageProvider::HuggingFace);
    pkg.url = "Comfy-Org/z_image_turbo";
    pkg.sha = "a1b2c3d4e5f6";
    pkg.license = "MIT";
    pkg.files.push_back(file);

    cache.packages.push_back(pkg);
    return cache;
}

} // namespace

TEST_CASE("JobAiPkgManifest Domain Validation Engine Suite", "[aipkg][manifest][validate]")
{
    // ========================================================================
    // BLOCK ONE: Usage / Real-World Documentation Examples
    // ========================================================================
    SECTION("Block 1: Canonical manifest ingestion guidelines")
    {
        // Example 1: Standard clean configuration setup.
        SECTION("Validating a properly constructed workspace repository cache")
        {
            AiPkgCache cache = makeValidMockCache();

            REQUIRE(JobAiPkgManifest::isValid(cache));

            auto const errors = JobAiPkgManifest::validate(cache);
            REQUIRE(errors.empty());
        }

        // Example 2: Filtering assets cleanly by typed categorical classifications.
        SECTION("Filtering nested records by explicit package target type definitions")
        {
            AiPkgCache cache = makeValidMockCache();

            auto const unetPackages = JobAiPkgManifest::packagesOfType(cache, PackageType::Unet);
            REQUIRE(unetPackages.size() == 1);
            REQUIRE(unetPackages[0].url == "Comfy-Org/z_image_turbo");

            auto const loraPackages = JobAiPkgManifest::packagesOfType(cache, PackageType::Lora);
            REQUIRE(loraPackages.empty());
        }
    }

    // ========================================================================
    // BLOCK TWO: Corner Cases & Invariant Hardening
    // ========================================================================
    SECTION("Block 2: Hardening and nested layer injection boundaries")
    {
        SECTION("Top-level identities field requirements checks")
        {
            AiPkgCache cache = makeValidMockCache();
            cache.project_name.clear();

            auto const errors = JobAiPkgManifest::validate(cache);
            REQUIRE(errors.size() == 1);
            REQUIRE(errors[0].field == "project_name");
            REQUIRE_FALSE(JobAiPkgManifest::isValid(cache));
        }

        SECTION("Zero-filled snapshot tracking array verification rules")
        {
            AiPkgCache cache = makeValidMockCache();
            cache.sha.fill(0x00); // Wipe top level digest back to bare canvas state

            auto const errors = JobAiPkgManifest::validate(cache);
            REQUIRE(errors.size() == 1);
            REQUIRE(errors[0].field == "sha");
        }

        SECTION("Completely unallocated packages array list boundaries")
        {
            AiPkgCache cache = makeValidMockCache();
            cache.packages.clear(); // Empty target pool allocation vector entirely

            auto const errors = JobAiPkgManifest::validate(cache);
            REQUIRE(errors.size() == 1);
            REQUIRE(errors[0].field == "packages");
        }

        SECTION("Nested structural tracking parameter requirements inside active units")
        {
            AiPkgCache cache = makeValidMockCache();
            cache.packages[0].url.clear(); // Clear provider destination route mid-flight

            auto const errors = JobAiPkgManifest::validate(cache);
            REQUIRE(errors.size() == 1);
            REQUIRE(errors[0].field == "packages[0].url");
        }

        SECTION("Validating empty file layouts inside typed package assignments")
        {
            AiPkgCache cache = makeValidMockCache();
            cache.packages[0].files.clear(); // No targets allocated behind this tag

            auto const errors = JobAiPkgManifest::validate(cache);
            REQUIRE(errors.size() == 1);
            REQUIRE(errors[0].field == "packages[0]");
        }

        SECTION("Enforcing name constraints inside downstream storage leaf records")
        {
            AiPkgCache cache = makeValidMockCache();
            cache.packages[0].files[0].name.clear(); // Unnamed installation asset

            auto const errors = JobAiPkgManifest::validate(cache);
            REQUIRE(errors.size() == 1);
            REQUIRE(errors[0].field == "packages[0].files[0].name");
        }

        SECTION("Enforcing zero-filled payload digest blocks on nested binary nodes")
        {
            AiPkgCache cache = makeValidMockCache();
            cache.packages[0].files[0].sha.fill(0x00); // Target file hash is unpopulated

            auto const errors = JobAiPkgManifest::validate(cache);
            REQUIRE(errors.size() == 1);
            REQUIRE(errors[0].field == "packages[0].files[0].sha");
        }

        SECTION("Enforcing size dimension properties on enabled processing targets")
        {
            AiPkgCache cache = makeValidMockCache();
            cache.packages[0].files[0].size = 0; // Claims zero bytes footprint while active

            auto const errors = JobAiPkgManifest::validate(cache);
            REQUIRE(errors.size() == 1);
            REQUIRE(errors[0].field == "packages[0].files[0].size");
        }

        SECTION("Multi-variant package with disabled un-fetched files passes validation")
        {
            AiPkgCache cache = makeValidMockCache();

            // Append a disabled, sibling quantization variant (e.g., GGUF Q4_K_M vs Q8_0)
            AiPkgFile disabledVariant{};
            disabledVariant.name = "model_q4_k_m.gguf";
            disabledVariant.enabled = false;
            disabledVariant.size = 0;             // Not fetched yet, perfectly valid!
            disabledVariant.sha.fill(0x00);        // Zero-filled hash is expected here.

            cache.packages[0].files.push_back(disabledVariant);

            // This must pass clean now that the layout rules are correctly symmetric
            auto const errors = JobAiPkgManifest::validate(cache);
            REQUIRE(errors.empty());
            REQUIRE(JobAiPkgManifest::isValid(cache));
        }
    }

// ========================================================================
// BLOCK THREE: Performance Benchmarks / Stress Validation
// ========================================================================
#ifdef JOB_TEST_BENCHMARKS
    SECTION("Block 3: Deep inspection and string parsing calculation loops")
    {
        AiPkgCache cache = makeValidMockCache();

        // Push it to the limit. Evaluate scanning performance of nested vectors and strings.
        BENCHMARK("Manifest payload parsing and layout tree schema checking cost") {
            return JobAiPkgManifest::validate(cache);
        };

        BENCHMARK("Typed category extraction loop velocity pass") {
            return JobAiPkgManifest::packagesOfType(cache, PackageType::Unet);
        };
    }
#endif
}