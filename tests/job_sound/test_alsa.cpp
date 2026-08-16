#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <cstddef>
#include <string>
#include <unordered_set>

#include <alsa/asoundlib.h>

#include <alsa_control.h>
#include <alsa_device.h>
#include <alsa_manager.h>
#include <alsa_sound_card.h>

using namespace job::sound;

namespace {

[[nodiscard]] bool hasAlsaCards() {
    int card = -1;
    return snd_card_next(&card) >= 0 && card >= 0;
}

[[nodiscard]] std::size_t countSystemAlsaCards() {
    std::size_t count = 0;
    int card = -1;

    while (snd_card_next(&card) >= 0 && card >= 0) {
        ++count;
    }
    return count;
}

// Helper to enforce the ALSA skip check concisely
auto requireCards() {
    if (!hasAlsaCards()) {
        SKIP("This system does not expose any ALSA sound cards");
    }
}

} // namespace

// ============================================================================
// Block 1: Usage / Examples
// ============================================================================

TEST_CASE("ALSA can enumerate the sound cards installed on the system", "[sound][alsa][hardware][usage]") {
    requireCards();
    REQUIRE(countSystemAlsaCards() > 0);
}

TEST_CASE("AlsaManager discovers ALSA cards into the JOB sound model", "[sound][alsa][manager][usage]") {
    requireCards();

    AlsaManager manager;
    REQUIRE(manager.cards());
    REQUIRE_FALSE(manager.cards()->isEmpty());
    REQUIRE(manager.cards()->size() > 0);
}

TEST_CASE("AlsaManager discovered cards have stable JOB identities", "[sound][alsa][manager][card][usage]") {
    requireCards();

    AlsaManager manager;
    REQUIRE(manager.cards());
    REQUIRE_FALSE(manager.cards()->isEmpty());

    std::unordered_set<std::string> seenUids;

    for (const auto& [_, card] : *manager.cards()) {
        REQUIRE(card);
        REQUIRE_FALSE(card->uid().empty());
        REQUIRE_FALSE(card->path().empty());

        REQUIRE(seenUids.insert(card->uid()).second);
        REQUIRE(manager.cards()->contains(card->uid()));
        REQUIRE(manager.cards()->at(card->uid()) == card.get());
    }
}

TEST_CASE("AlsaManager exposes a selected card after hardware discovery", "[sound][alsa][manager][selection][usage]") {
    requireCards();

    AlsaManager manager;
    REQUIRE(manager.cards());
    REQUIRE_FALSE(manager.cards()->isEmpty());

    auto* selected = manager.selectedCard();
    REQUIRE(selected);
    REQUIRE_FALSE(selected->uid().empty());
    REQUIRE(manager.cards()->contains(selected->uid()));
}

TEST_CASE("AlsaSoundCard exposes discovered PCM devices", "[sound][alsa][card][device][usage]") {
    requireCards();

    AlsaManager manager;
    bool foundDevice = false;

    for (const auto& [_, card] : *manager.cards()) {
        REQUIRE(card);
        REQUIRE(card->devices());

        for (const auto& [_, device] : *card->devices()) {
            REQUIRE(device);
            foundDevice = true;

            REQUIRE_FALSE(device->uid().empty());
            REQUIRE_FALSE(device->name().empty());
            REQUIRE((device->isPlayback() || device->isCapture()));
            REQUIRE(card->devices()->contains(device->uid()));
        }
    }

    if (!foundDevice) {
        SKIP("ALSA cards were found but no PCM devices were exposed");
    }
}

TEST_CASE("ALSA playback and capture devices use distinct JOB identities", "[sound][alsa][device][uid][usage]") {
    requireCards();

    AlsaManager manager;
    std::unordered_set<std::string> deviceUids;
    std::size_t deviceCount = 0;

    for (const auto& [_, card] : *manager.cards()) {
        REQUIRE(card);

        for (const auto& [_, device] : *card->devices()) {
            REQUIRE(device);
            ++deviceCount;

            REQUIRE_FALSE(device->uid().empty());
            REQUIRE(deviceUids.insert(device->uid()).second);
        }
    }

    if (deviceCount == 0) {
        SKIP("No ALSA PCM devices were discovered");
    }
}

TEST_CASE("AlsaDevice exposes mixer controls discovered from ALSA", "[sound][alsa][device][control][usage]") {
    requireCards();

    AlsaManager manager;
    bool foundControl = false;

    for (const auto& [_, card] : *manager.cards()) {
        REQUIRE(card);

        for (const auto& [_, device] : *card->devices()) {
            REQUIRE(device);
            REQUIRE(device->controls());

            for (const auto& [_, control] : *device->controls()) {
                REQUIRE(control);
                foundControl = true;

                REQUIRE_FALSE(control->uid().empty());
                REQUIRE(device->controls()->contains(control->uid()));
            }
        }
    }

    if (!foundControl) {
        SKIP("ALSA hardware is present but exposes no mixer controls through this path");
    }
}

TEST_CASE("AlsaControl volume state stays within the reported hardware range", "[sound][alsa][control][volume][usage]") {
    requireCards();

    AlsaManager manager;
    std::size_t checked = 0;

    for (const auto& [_, card] : *manager.cards()) {
        REQUIRE(card);

        for (const auto& [_, device] : *card->devices()) {
            REQUIRE(device);

            for (const auto& [_, control] : *device->controls()) {
                REQUIRE(control);

                if (control->minVolume() > control->maxVolume()) {
                    continue;
                }

                REQUIRE(control->volume() >= control->minVolume());
                REQUIRE(control->volume() <= control->maxVolume());
                ++checked;
            }
        }
    }

    if (checked == 0) {
        SKIP("No ALSA controls with a usable volume range were discovered");
    }
}

TEST_CASE("Selected ALSA device belongs to the selected card", "[sound][alsa][manager][selection][usage]") {
    requireCards();

    AlsaManager manager;
    auto* card = manager.selectedCard();
    auto* device = manager.selectedDevice();

    REQUIRE(card);
    if (!device) {
        SKIP("Selected ALSA card exposes no PCM devices");
    }

    REQUIRE(card->devices());
    REQUIRE(card->devices()->contains(device->uid()));
    REQUIRE(card->devices()->at(device->uid()) == device);
}

// ============================================================================
// Block 2: Edge Cases / Invariants
// ============================================================================

TEST_CASE("AlsaControl ignores an empty uid update", "[sound][alsa][control][edge]") {
    AlsaControl control;
    control.setUid("Master");
    REQUIRE(control.uid() == "Master");

    control.setUid("");
    REQUIRE(control.uid() == "Master");
}

TEST_CASE("AlsaControl accepts repeated discovery updates", "[sound][alsa][control][edge]") {
    AlsaControl control;
    control.setUid("Master");
    control.updateVolume(50, 0, 100);
    control.updateDb(-20.0f, -60.0f, 0.0f);
    control.updateState(false, true, false);

    CHECK(control.volume() == 50);
    CHECK(control.minVolume() == 0);
    CHECK(control.maxVolume() == 100);
    CHECK(control.dBVolume() == -20.0f);
    CHECK(control.minDb() == -60.0f);
    CHECK(control.maxDb() == 0.0f);
    CHECK_FALSE(control.isMuted());
    CHECK(control.isPlayback());
    CHECK_FALSE(control.isCapture());

    control.updateVolume(25, 10, 75);
    CHECK(control.volume() == 25);
    CHECK(control.minVolume() == 10);
    CHECK(control.maxVolume() == 75);
}

TEST_CASE("AlsaDevice begins with an owned empty control model", "[sound][alsa][device][edge]") {
    AlsaDevice device;
    REQUIRE(device.controls());
    REQUIRE(device.controls()->isEmpty());
    REQUIRE_FALSE(device.isPlayback());
    REQUIRE_FALSE(device.isCapture());
}

TEST_CASE("AlsaSoundCard begins with an owned empty device model", "[sound][alsa][card][edge]") {
    AlsaSoundCard card;
    REQUIRE(card.devices());
    REQUIRE(card.devices()->isEmpty());
}

TEST_CASE("AlsaDevice playback and capture identities can coexist in one card", "[sound][alsa][device][uid][edge]") {
    AlsaSoundCard card;
    card.setUid("hw:0");

    auto playback = AlsaDevice::createUnique();
    playback->setUid("hw:0,0:playback");
    playback->setIsPlayback(true);

    auto capture = AlsaDevice::createUnique();
    capture->setUid("hw:0,0:capture");
    capture->setIsCapture(true);

    card.devices()->insert(std::move(playback));
    card.devices()->insert(std::move(capture));

    REQUIRE(card.devices()->size() == 2);
    REQUIRE(card.devices()->contains("hw:0,0:playback"));
    REQUIRE(card.devices()->contains("hw:0,0:capture"));
}

TEST_CASE("Refreshing AlsaManager hardware does not accumulate duplicate cards", "[sound][alsa][manager][refresh][edge]") {
    requireCards();

    AlsaManager manager;
    const std::size_t initialCount = manager.cards()->size();
    REQUIRE(initialCount > 0);

    manager.populateAlsaHardware();
    REQUIRE(manager.cards()->size() == initialCount);

    std::unordered_set<std::string> cardUids;
    for (const auto& [_, card] : *manager.cards()) {
        REQUIRE(card);
        REQUIRE(cardUids.insert(card->uid()).second);
    }
}

TEST_CASE("Refreshing ALSA hardware leaves selected pointers inside the new model", "[sound][alsa][manager][refresh][ownership][edge]") {
    requireCards();

    AlsaManager manager;
    manager.populateAlsaHardware();

    auto* card = manager.selectedCard();
    REQUIRE(card);
    REQUIRE(manager.cards()->contains(card->uid()));

    auto* device = manager.selectedDevice();
    if (!device) {
        SKIP("Selected ALSA card exposes no PCM devices");
    }
    REQUIRE(card->devices()->contains(device->uid()));
}

TEST_CASE("ALSA model hierarchy contains no duplicate control uid within a device", "[sound][alsa][control][uid][edge]") {
    requireCards();

    AlsaManager manager;
    for (const auto& [_, card] : *manager.cards()) {
        REQUIRE(card);

        for (const auto& [_, device] : *card->devices()) {
            REQUIRE(device);
            std::unordered_set<std::string> controlUids;

            for (const auto& [_, control] : *device->controls()) {
                REQUIRE(control);
                REQUIRE(controlUids.insert(control->uid()).second);
            }
        }
    }
}

TEST_CASE("ALSA discovered device metadata remains internally consistent", "[sound][alsa][device][metadata][edge]") {
    requireCards();

    AlsaManager manager;
    std::size_t checked = 0;

    for (const auto& [_, card] : *manager.cards()) {
        REQUIRE(card);

        for (const auto& [_, device] : *card->devices()) {
            REQUIRE(device);
            REQUIRE_FALSE(device->uid().empty());
            REQUIRE(device->deviceNum() >= 0);
            REQUIRE(device->subdeviceNum() >= 0);

            if (device->isPlayback()) {
                REQUIRE(device->type() == "Playback");
            }
            if (device->isCapture()) {
                REQUIRE(device->type() == "Capture");
            }
            ++checked;
        }
    }

    if (checked == 0) {
        SKIP("No ALSA PCM devices were discovered");
    }
}

TEST_CASE("AlsaPlaybackThread can start and stop on a discovered playback device", "[sound][alsa][playback][hardware][usage]") {
    AlsaManager manager;
    auto *card = manager.selectedCard();
    if (!card)
        SKIP("No ALSA card was selected");

    AlsaDevice *playbackDevice = nullptr;
    for (const auto &[_, device] : *card->devices()) {
        if (device && device->isPlayback()) {
            playbackDevice = device.get();
            break;
        }
    }
    if (!playbackDevice) SKIP("Selected ALSA card exposes no playback device");

    const std::string fullDevice = card->path() + "," + std::to_string(playbackDevice->deviceNum());
    AlsaPlaybackThread playback{fullDevice};

    const auto result = playback.start();
    if (result != job::threads::JobThread::StartResult::Started) {
        SKIP("Could not start playback thread (device likely busy or unavailable)");
    }

    REQUIRE(playback.isRunning());
    playback.stop();
    REQUIRE(playback.join());
    REQUIRE_FALSE(playback.isRunning());
}
TEST_CASE("AlsaCaptureThread can start and stop on a discovered capture device", "[sound][alsa][capture][hardware][usage]")
{
    AlsaManager manager;
    auto *card = manager.selectedCard();

    if (!card)
        SKIP("No ALSA card was selected");

    AlsaDevice *captureDevice = nullptr;
    for (auto &item : *card->devices()) {
        auto &device = item.second;
        if (device && device->isCapture()) {
            captureDevice = device.get();
            break;
        }
    }

    if (!captureDevice)
        SKIP("Selected ALSA card exposes no capture device");

    const std::string fullDevice = card->path() + "," + std::to_string(captureDevice->deviceNum());
    AlsaCaptureThread capture{fullDevice};

    const auto result = capture.start();
    REQUIRE(result == job::threads::JobThread::StartResult::Started);

    REQUIRE(capture.isRunning());
    capture.stop();

    REQUIRE(capture.join());
    REQUIRE_FALSE(capture.isRunning());
}