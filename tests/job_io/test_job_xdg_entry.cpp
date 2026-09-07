#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <cstdint>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <job_random.h>
#include <job_tmp_file.h>
#include <job_xdg_entry.h>

#ifdef JOB_TEST_IO_PTY_DEBUG_SER
#include <yaml-cpp/yaml.h>
#endif


namespace job::io::test {

[[nodiscard]] std::string randomString(std::mt19937_64 &rng,
                                       std::size_t minSize,
                                       std::size_t maxSize)
{
    static constexpr std::string_view chars =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        "_-.:/";

    std::uniform_int_distribution<std::size_t> sizeDist(minSize, maxSize);
    std::uniform_int_distribution<std::size_t> charDist(0, chars.size() - 1);

    const std::size_t size = sizeDist(rng);

    std::string result(size, '\0');

    for (char &value : result)
        value = chars[charDist(rng)];

    return result;
}

[[nodiscard]] std::size_t randomSize(std::mt19937_64 &rng,
                                     std::size_t lo,
                                     std::size_t hi)
{
    std::uniform_int_distribution<std::size_t> dist(lo, hi);
    return dist(rng);
}

[[nodiscard]] JobXdgAction randomAction(std::mt19937_64 &rng,
                                               std::size_t extensionCount)
{
    JobXdgAction action;

    action.id = randomString(rng, 8, 32);
    action.name.value = randomString(rng, 16, 96);
    action.icon.value = randomString(rng, 8, 48);
    action.exec = randomString(rng, 32, 256);

    const std::size_t localeCount = randomSize(rng, 1, 8);

    for (std::size_t i = 0; i < localeCount; ++i) {
        const std::string locale = "locale_" + std::to_string(i);
        action.name.localized[locale] = randomString(rng, 8, 128);
        action.icon.localized[locale] = randomString(rng, 4, 64);
    }

    action.extensionEntries.reserve(extensionCount);

    for (std::size_t i = 0; i < extensionCount; ++i) {
        JobXdgRawEntry entry;
        entry.key = "X-JOB-" + randomString(rng, 8, 32);
        entry.value = randomString(rng, 0, 256);
        action.extensionEntries.push_back(std::move(entry));
    }

    return action;
}

[[nodiscard]] JobXdgRawGroup randomRawGroup(std::mt19937_64 &rng,
                                                   std::size_t entryCount)
{
    JobXdgRawGroup group;
    group.name = "X-JOB-" + randomString(rng, 8, 48);
    group.entries.reserve(entryCount);

    for (std::size_t i = 0; i < entryCount; ++i) {
        JobXdgRawEntry entry;
        entry.key = randomString(rng, 8, 48);
        entry.value = randomString(rng, 0, 512);
        group.entries.push_back(std::move(entry));
    }

    return group;
}

} // namespace job::io::test

using namespace job::io;

//////////////////////////////////////////////////////////
// Block 1: Usage / examples
//////////////////////////////////////////////////////////

TEST_CASE("JobXdgEntry default construction represents an empty desktop entry",
          "[job_io][xdg][entry][usage]")
{
    JobXdgEntry entry;

    REQUIRE(entry.path().empty());
    REQUIRE(entry.desktopFileId().empty());

    REQUIRE(entry.type() == JobXdgEntryType::Unknown);
    REQUIRE(entry.version().empty());

    REQUIRE(entry.name().value.empty());
    REQUIRE(entry.name().localized.empty());

    REQUIRE(entry.genericName().value.empty());
    REQUIRE(entry.genericName().localized.empty());

    REQUIRE(entry.comment().value.empty());
    REQUIRE(entry.comment().localized.empty());

    REQUIRE(entry.icon().value.empty());
    REQUIRE(entry.icon().localized.empty());

    REQUIRE(entry.keywords().value.empty());
    REQUIRE(entry.keywords().localized.empty());

    REQUIRE_FALSE(entry.noDisplay());
    REQUIRE_FALSE(entry.hidden());

    REQUIRE(entry.onlyShowIn().empty());
    REQUIRE(entry.notShowIn().empty());

    REQUIRE_FALSE(entry.dbusActivatable());

    REQUIRE(entry.tryExec().empty());
    REQUIRE(entry.exec().empty());
    REQUIRE(entry.workingDirectory().empty());

    REQUIRE_FALSE(entry.terminal());

    REQUIRE(entry.actions().empty());
    REQUIRE(entry.mimeTypes().empty());
    REQUIRE(entry.categories().empty());
    REQUIRE(entry.implements().empty());

    REQUIRE_FALSE(entry.startupNotify().has_value());
    REQUIRE(entry.startupWMClass().empty());

    REQUIRE_FALSE(entry.prefersNonDefaultGPU());
    REQUIRE_FALSE(entry.singleMainWindow());

    REQUIRE(entry.url().empty());

    REQUIRE(entry.desktopActions().empty());

    REQUIRE(entry.extensionEntries().empty());
    REQUIRE(entry.extensionGroups().empty());
}

TEST_CASE("JobXdgEntry factories create empty desktop entries",
          "[job_io][xdg][entry][usage][factory]")
{
    const auto shared = JobXdgEntry::createShared();
    const auto unique = JobXdgEntry::createUniq();

    REQUIRE(shared);
    REQUIRE(unique);

    REQUIRE(shared->type() == JobXdgEntryType::Unknown);
    REQUIRE(unique->type() == JobXdgEntryType::Unknown);

    REQUIRE(shared->path().empty());
    REQUIRE(unique->path().empty());

    REQUIRE(shared->desktopFileId().empty());
    REQUIRE(unique->desktopFileId().empty());
}

TEST_CASE("JobXdgEntry LocalizedString stores a default value and locale variants",
          "[job_io][xdg][entry][localized][usage]")
{
    JobXdgLocalizedString text;

    text.value = "Terminal";
    text.localized["de"] = "Terminal";
    text.localized["fr"] = "Terminal";
    text.localized["en_GB"] = "Terminal";

    REQUIRE(text.value == "Terminal");
    REQUIRE(text.localized.size() == 3);

    REQUIRE(text.localized.at("de") == "Terminal");
    REQUIRE(text.localized.at("fr") == "Terminal");
    REQUIRE(text.localized.at("en_GB") == "Terminal");
}

TEST_CASE("JobXdgEntry RawEntry preserves extension key and value",
          "[job_io][xdg][entry][extension][usage]")
{
    JobXdgRawEntry entry;

    entry.key = "X-JOB-Feature";
    entry.value = "Enabled";

    REQUIRE(entry.key == "X-JOB-Feature");
    REQUIRE(entry.value == "Enabled");
}

TEST_CASE("JobXdgEntry Action represents a desktop action",
          "[job_io][xdg][entry][action][usage]")
{
    JobXdgAction action;

    action.id = "NewWindow";
    action.name.value = "New Window";
    action.name.localized["de"] = "Neues Fenster";

    action.icon.value = "window-new";
    action.exec = "/usr/bin/example --new-window";

    JobXdgRawEntry extension;
    extension.key = "X-JOB-Action";
    extension.value = "true";

    action.extensionEntries.push_back(std::move(extension));

    REQUIRE(action.id == "NewWindow");

    REQUIRE(action.name.value == "New Window");
    REQUIRE(action.name.localized.size() == 1);
    REQUIRE(action.name.localized.at("de") == "Neues Fenster");

    REQUIRE(action.icon.value == "window-new");
    REQUIRE(action.exec == "/usr/bin/example --new-window");

    REQUIRE(action.extensionEntries.size() == 1);
    REQUIRE(action.extensionEntries.front().key == "X-JOB-Action");
    REQUIRE(action.extensionEntries.front().value == "true");
}

TEST_CASE("JobXdgEntry RawGroup preserves unknown desktop entry groups",
          "[job_io][xdg][entry][extension][group][usage]")
{
    JobXdgRawGroup group;

    group.name = "X-JOB Metadata";

    JobXdgRawEntry first;
    first.key = "Owner";
    first.value = "JOB";

    JobXdgRawEntry second;
    second.key = "Version";
    second.value = "1";

    group.entries.push_back(std::move(first));
    group.entries.push_back(std::move(second));

    REQUIRE(group.name == "X-JOB Metadata");
    REQUIRE(group.entries.size() == 2);

    REQUIRE(group.entries[0].key == "Owner");
    REQUIRE(group.entries[0].value == "JOB");

    REQUIRE(group.entries[1].key == "Version");
    REQUIRE(group.entries[1].value == "1");
}

TEST_CASE("JobXdgEntry schema objects are ordinary copyable values",
          "[job_io][xdg][entry][usage][copy]")
{
    JobXdgAction original;

    original.id = "Edit";
    original.name.value = "Edit";
    original.exec = "/usr/bin/example --edit";

    JobXdgRawEntry extension;
    extension.key = "X-Test";
    extension.value = "copy";

    original.extensionEntries.push_back(std::move(extension));

    const JobXdgAction copy = original;

    REQUIRE(copy.id == original.id);
    REQUIRE(copy.name.value == original.name.value);
    REQUIRE(copy.exec == original.exec);

    REQUIRE(copy.extensionEntries.size() == 1);
    REQUIRE(copy.extensionEntries.front().key == "X-Test");
    REQUIRE(copy.extensionEntries.front().value == "copy");
}

TEST_CASE("JobXdgEntry itself is copyable",
          "[job_io][xdg][entry][usage][copy]")
{
    JobXdgEntry original;
    const JobXdgEntry copy = original;

    REQUIRE(copy.path() == original.path());
    REQUIRE(copy.desktopFileId() == original.desktopFileId());
    REQUIRE(copy.type() == original.type());
    REQUIRE(copy.startupNotify() == original.startupNotify());
}

TEST_CASE("JobXdgEntry itself is movable",
          "[job_io][xdg][entry][usage][move]")
{
    JobXdgEntry original;
    JobXdgEntry moved(std::move(original));

    REQUIRE(moved.type() == JobXdgEntryType::Unknown);
    REQUIRE(moved.path().empty());
    REQUIRE(moved.desktopFileId().empty());
}

//////////////////////////////////////////////////////////
// Block 2: Edge cases / schema invariants
//////////////////////////////////////////////////////////

TEST_CASE("JobXdgEntry StartupNotify preserves unspecified state",
          "[job_io][xdg][entry][startup_notify][edge]")
{
    JobXdgEntry entry;

    const auto startupNotify = entry.startupNotify();

    REQUIRE_FALSE(startupNotify.has_value());
}

TEST_CASE("JobXdgEntry localized values allow an empty base value",
          "[job_io][xdg][entry][localized][edge]")
{
    JobXdgLocalizedString text;

    text.localized["de"] = "Anwendung";
    text.localized["fr"] = "Application";

    REQUIRE(text.value.empty());
    REQUIRE(text.localized.size() == 2);
    REQUIRE(text.localized.at("de") == "Anwendung");
    REQUIRE(text.localized.at("fr") == "Application");
}

TEST_CASE("JobXdgEntry localized values preserve locale keys exactly",
          "[job_io][xdg][entry][localized][edge]")
{
    JobXdgLocalizedString text;

    text.localized["en"] = "English";
    text.localized["en_GB"] = "British English";
    text.localized["sr@latin"] = "Latin Serbian";
    text.localized["zh_CN"] = "Simplified Chinese";

    REQUIRE(text.localized.size() == 4);

    REQUIRE(text.localized.contains("en"));
    REQUIRE(text.localized.contains("en_GB"));
    REQUIRE(text.localized.contains("sr@latin"));
    REQUIRE(text.localized.contains("zh_CN"));
}

TEST_CASE("JobXdgEntry RawEntry preserves empty extension values",
          "[job_io][xdg][entry][extension][edge]")
{
    JobXdgRawEntry entry;

    entry.key = "X-JOB-Empty";
    entry.value.clear();

    REQUIRE(entry.key == "X-JOB-Empty");
    REQUIRE(entry.value.empty());
}

TEST_CASE("JobXdgEntry RawGroup can represent an empty unknown group",
          "[job_io][xdg][entry][extension][group][edge]")
{
    JobXdgRawGroup group;

    group.name = "X-Empty Group";

    REQUIRE(group.name == "X-Empty Group");
    REQUIRE(group.entries.empty());
}

TEST_CASE("JobXdgEntry Action can preserve extension data without standard fields",
          "[job_io][xdg][entry][action][edge]")
{
    JobXdgAction action;

    JobXdgRawEntry extension;
    extension.key = "X-Custom";
    extension.value = "value";

    action.extensionEntries.push_back(std::move(extension));

    REQUIRE(action.id.empty());
    REQUIRE(action.name.value.empty());
    REQUIRE(action.icon.value.empty());
    REQUIRE(action.exec.empty());

    REQUIRE(action.extensionEntries.size() == 1);
    REQUIRE(action.extensionEntries.front().key == "X-Custom");
    REQUIRE(action.extensionEntries.front().value == "value");
}

TEST_CASE("JobXdgEntry nested schema values are independent after copying",
          "[job_io][xdg][entry][copy][edge]")
{
    JobXdgLocalizedString original;

    original.value = "Original";
    original.localized["de"] = "Original DE";

    JobXdgLocalizedString copy = original;

    copy.value = "Changed";
    copy.localized["de"] = "Changed DE";
    copy.localized["fr"] = "Changed FR";

    REQUIRE(original.value == "Original");
    REQUIRE(original.localized.size() == 1);
    REQUIRE(original.localized.at("de") == "Original DE");

    REQUIRE(copy.value == "Changed");
    REQUIRE(copy.localized.size() == 2);
    REQUIRE(copy.localized.at("de") == "Changed DE");
    REQUIRE(copy.localized.at("fr") == "Changed FR");
}

//////////////////////////////////////////////////////////
// Reflection / serialization diagnostics
//////////////////////////////////////////////////////////

#ifdef JOB_TEST_IO_PTY_DEBUG_SER

TEST_CASE("JobXdgEntry serializes to JSON",
          "[job_io][xdg][entry][usage][serialization][json]")
{
    JobXdgEntry entry;

    const auto json = entry.toJson();

    REQUIRE(json.is_object());
    REQUIRE_FALSE(json.empty());

    const std::string serialized = json.dump(4);

    REQUIRE_FALSE(serialized.empty());

    WARN("[XDG ENTRY JSON]\n" << serialized);
}

TEST_CASE("JobXdgEntry serializes to YAML",
          "[job_io][xdg][entry][usage][serialization][yaml]")
{
    JobXdgEntry entry;

    const YAML::Node yaml = entry.toYaml();

    REQUIRE(yaml.IsMap());
    REQUIRE(yaml.size() > 0);

    YAML::Emitter emitter;
    emitter << yaml;

    REQUIRE(emitter.good());

    const std::string serialized = emitter.c_str();

    REQUIRE_FALSE(serialized.empty());

    WARN("[XDG ENTRY YAML]\n" << serialized);
}

TEST_CASE("JobXdgEntry serializes to binary",
          "[job_io][xdg][entry][usage][serialization][binary]")
{
    JobXdgEntry entry;

    std::vector<std::uint8_t> binary;
    entry.toBinary(binary);

    REQUIRE_FALSE(binary.empty());

    WARN("[XDG ENTRY BINARY] " << binary.size() << " bytes");
}

TEST_CASE("JobXdgEntry LocalizedString serializes through reflection",
          "[job_io][xdg][entry][localized][serialization][json]")
{
    JobXdgLocalizedString text;

    text.value = "Terminal";
    text.localized["de"] = "Terminal";
    text.localized["fr"] = "Terminal";

    const auto json = text.toJson();

    REQUIRE(json.is_object());
    REQUIRE_FALSE(json.empty());

    const std::string serialized = json.dump(4);

    REQUIRE(serialized.find("Terminal") != std::string::npos);

    WARN("[XDG LOCALIZED STRING JSON]\n" << serialized);
}

TEST_CASE("JobXdgEntry Action serializes through reflection",
          "[job_io][xdg][entry][action][serialization][json]")
{
    JobXdgAction action;

    action.id = "NewWindow";
    action.name.value = "New Window";
    action.name.localized["de"] = "Neues Fenster";
    action.icon.value = "window-new";
    action.exec = "/usr/bin/example --new-window";

    JobXdgRawEntry extension;
    extension.key = "X-JOB-Test";
    extension.value = "true";

    action.extensionEntries.push_back(std::move(extension));

    const auto json = action.toJson();

    REQUIRE(json.is_object());
    REQUIRE_FALSE(json.empty());

    const std::string serialized = json.dump(4);

    REQUIRE(serialized.find("NewWindow") != std::string::npos);
    REQUIRE(serialized.find("New Window") != std::string::npos);
    REQUIRE(serialized.find("Neues Fenster") != std::string::npos);
    REQUIRE(serialized.find("/usr/bin/example --new-window") != std::string::npos);
    REQUIRE(serialized.find("X-JOB-Test") != std::string::npos);

    WARN("[XDG ACTION JSON]\n" << serialized);
}

TEST_CASE("JobXdgEntry RawGroup serializes through reflection",
          "[job_io][xdg][entry][extension][serialization][json]")
{
    JobXdgRawGroup group;

    group.name = "X-JOB Metadata";

    JobXdgRawEntry entry;
    entry.key = "Feature";
    entry.value = "Enabled";

    group.entries.push_back(std::move(entry));

    const auto json = group.toJson();

    REQUIRE(json.is_object());
    REQUIRE_FALSE(json.empty());

    const std::string serialized = json.dump(4);

    REQUIRE(serialized.find("X-JOB Metadata") != std::string::npos);
    REQUIRE(serialized.find("Feature") != std::string::npos);
    REQUIRE(serialized.find("Enabled") != std::string::npos);

    WARN("[XDG RAW GROUP JSON]\n" << serialized);
}

#endif

//////////////////////////////////////////////////////////
// Block 3: Benchmarks / stress
//////////////////////////////////////////////////////////

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("JobXdgEntry construction benchmark",
          "[job_io][xdg][entry][benchmark][construction]")
{
    BENCHMARK("construct 4096 JobXdgEntry objects")
    {
        std::vector<JobXdgEntry> entries;
        entries.resize(4096);
        return entries;
    };
}

TEST_CASE("JobXdgEntry randomized nested schema allocation benchmark",
          "[job_io][xdg][entry][benchmark][allocation]")
{
    const std::uint64_t seed = job::crypto::JobRandom::secureU64();
    INFO("XDG nested allocation seed: " << seed);

    BENCHMARK_ADVANCED("construct 2048 randomized desktop actions")(Catch::Benchmark::Chronometer meter)
    {
        std::mt19937_64 rng(seed);

        meter.measure([&] {
            std::vector<JobXdgAction> actions;
            actions.reserve(2048);

            for (std::size_t i = 0; i < 2048; ++i) {
                const std::size_t extensionCount =
                    job::io::test::randomSize(rng, 0, 32);

                actions.push_back(
                    job::io::test::randomAction(rng, extensionCount));
            }

            return actions;
        });
    };
}

TEST_CASE("JobXdgEntry randomized schema mutation benchmark",
          "[job_io][xdg][entry][benchmark][mutation]")
{
    const std::uint64_t seed = job::crypto::JobRandom::secureU64();
    INFO("XDG schema mutation seed: " << seed);

    std::mt19937_64 buildRng(seed);

    std::vector<JobXdgAction> source;
    source.reserve(2048);

    for (std::size_t i = 0; i < 2048; ++i)
        source.push_back(job::io::test::randomAction(buildRng, 16));

    BENCHMARK_ADVANCED("copy and mutate 2048 randomized desktop actions")(Catch::Benchmark::Chronometer meter)
    {
        meter.measure([&] {
            auto actions = source;
            std::mt19937_64 rng(seed ^ 0x9e3779b97f4a7c15ULL);

            for (std::size_t i = 0; i < actions.size(); ++i) {
                auto &action = actions[i];

                if ((i % 3) == 0)
                    action.name.value = job::io::test::randomString(rng, 32, 256);

                if ((i % 5) == 0)
                    action.exec = job::io::test::randomString(rng, 64, 512);

                if ((i % 7) == 0 && !action.extensionEntries.empty())
                    action.extensionEntries.erase(action.extensionEntries.begin());

                if ((i % 11) == 0) {
                    JobXdgRawEntry entry;
                    entry.key = "X-JOB-MUTATED-" + std::to_string(i);
                    entry.value = job::io::test::randomString(rng, 128, 1024);
                    action.extensionEntries.push_back(std::move(entry));
                }
            }

            return actions;
        });
    };
}

TEST_CASE("JobXdgEntry Action binary serialization benchmark",
          "[job_io][xdg][entry][benchmark][serialization][binary]")
{
    const std::uint64_t seed = job::crypto::JobRandom::secureU64();
    INFO("XDG Action binary serialization seed: " << seed);

    std::mt19937_64 rng(seed);
    JobXdgAction action = job::io::test::randomAction(rng, 4096);

    for (std::size_t i = 0; i < action.extensionEntries.size(); i += 97)
        action.extensionEntries[i].value = job::io::test::randomString(rng, 4096, 16384);

    std::vector<std::uint8_t> binary;

    BENCHMARK("serialize large randomized Action to binary")
    {
        binary.clear();
        action.toBinary(binary);
        return binary.size();
    };

    action.toBinary(binary);
    WARN("[XDG ACTION BINARY BENCHMARK] seed: " << seed
                                                << ", serialized size: " << binary.size() << " bytes");
}

TEST_CASE("JobXdgEntry RawGroup binary serialization benchmark",
          "[job_io][xdg][entry][benchmark][serialization][binary]")
{
    const std::uint64_t seed = job::crypto::JobRandom::secureU64();
    INFO("XDG RawGroup binary serialization seed: " << seed);

    std::mt19937_64 rng(seed);
    JobXdgRawGroup group = job::io::test::randomRawGroup(rng, 8192);

    std::vector<std::uint8_t> binary;

    BENCHMARK("serialize RawGroup with 8192 entries to binary")
    {
        binary.clear();
        group.toBinary(binary);
        return binary.size();
    };

    group.toBinary(binary);
    WARN("[XDG RAW GROUP BINARY BENCHMARK] seed: " << seed
                                                   << ", serialized size: " << binary.size() << " bytes");
}

#endif

