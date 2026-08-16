#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include <alsa_control.h>
#include <alsa_device.h>
#include <alsa_sound_card.h>

#include <pipewire_device.h>
#include <pipewire_graph_adapter.h>
#include <pipewire_link.h>
#include <pipewire_port.h>

#include <virtual_eq_band.h>
#include <virtual_mixer_channel.h>

using namespace job::sound;


//
// Block 1: usage / examples
//

TEST_CASE(
    "AlsaControl stores mixer state and exposes its uid",
    "[sound][model][alsa][control][usage]")
{
    auto control = AlsaControl::createUnique();

    REQUIRE(control);

    control->setUid("Master");
    control->updateVolume(75, 0, 100);
    control->updateDb(-12.5f, -60.0f, 0.0f);
    control->updateState(false, true, false);
    control->setMute(false);

    REQUIRE(control->uid() == "Master");

    REQUIRE(control->volume() == 75);
    REQUIRE(control->minVolume() == 0);
    REQUIRE(control->maxVolume() == 100);

    REQUIRE(control->dBVolume() == -12.5f);
    REQUIRE(control->minDb() == -60.0f);
    REQUIRE(control->maxDb() == 0.0f);

    REQUIRE(control->isPlayback());
    REQUIRE_FALSE(control->isCapture());
    REQUIRE_FALSE(control->isMuted());
    REQUIRE_FALSE(control->mute());
}


TEST_CASE(
    "AlsaDevice owns controls by unique pointer",
    "[sound][model][alsa][device][usage]")
{
    auto device = AlsaDevice::createUnique();

    REQUIRE(device);
    REQUIRE(device->controls());
    REQUIRE(device->controls()->isEmpty());

    device->setUid("hw:0,0:playback");
    device->setName("Built-in Audio");
    device->setDeviceNum(0);
    device->setSubdeviceNum(0);
    device->setType("Playback");
    device->setIsPlayback(true);

    auto master = AlsaControl::createUnique();

    master->setUid("Master");
    master->setVolume(80);

    AlsaControl *masterPtr = master.get();

    device->controls()->insert(
        std::move(master)
        );

    REQUIRE(device->controls()->size() == 1);

    auto *stored =
        device->controls()->at("Master");

    REQUIRE(stored);
    REQUIRE(stored == masterPtr);
    REQUIRE(stored->volume() == 80);
}


TEST_CASE(
    "AlsaSoundCard owns devices by unique pointer",
    "[sound][model][alsa][card][usage]")
{
    auto card = AlsaSoundCard::createUnique();

    REQUIRE(card);
    REQUIRE(card->devices());

    card->setUid("hw:0");
    card->setPath("hw:0");
    card->setCard("PCH");
    card->setChip("Realtek");
    card->setDriver("HDA-Intel");
    card->setLongName("Built-in Audio");
    card->setComponents("HDA");

    auto device = AlsaDevice::createUnique();

    device->setUid("hw:0,0:playback");
    device->setName("ALC Playback");
    device->setDeviceNum(0);
    device->setIsPlayback(true);

    AlsaDevice *devicePtr = device.get();

    card->devices()->insert(
        std::move(device)
        );

    REQUIRE(card->devices()->size() == 1);

    auto *stored =
        card->devices()->at("hw:0,0:playback");

    REQUIRE(stored);
    REQUIRE(stored == devicePtr);
    REQUIRE(stored->isPlayback());
}


TEST_CASE(
    "AlsaSoundCard can represent a complete card device control hierarchy",
    "[sound][model][alsa][hierarchy][usage]")
{
    auto card = AlsaSoundCard::createUnique();

    card->setUid("hw:2");
    card->setPath("hw:2");
    card->setName("USB Audio");

    auto device = AlsaDevice::createUnique();

    device->setUid("hw:2,0:playback");
    device->setName("USB Playback");
    device->setDeviceNum(0);
    device->setIsPlayback(true);

    auto control = AlsaControl::createUnique();

    control->setUid("PCM");
    control->setVolume(90);
    control->setIsPlayback(true);

    device->controls()->insert(
        std::move(control)
        );

    card->devices()->insert(
        std::move(device)
        );

    auto *storedDevice =
        card->devices()->at("hw:2,0:playback");

    REQUIRE(storedDevice);

    auto *storedControl =
        storedDevice->controls()->at("PCM");

    REQUIRE(storedControl);
    REQUIRE(storedControl->volume() == 90);
}


TEST_CASE(
    "PipeWireDevice stores discovered device information",
    "[sound][model][pipewire][device][usage]")
{
    auto device = PipeWireDevice::createUnique();

    REQUIRE(device);

    device->updateFromDiscovery(
        "42",
        "USB Microphone",
        "Input"
        );

    REQUIRE(device->uid() == "42");
    REQUIRE(device->name() == "USB Microphone");
    REQUIRE(device->direction() == "Input");
}


TEST_CASE(
    "PipeWirePort derives its uid from the PipeWire port id",
    "[sound][model][pipewire][port][usage]")
{
    auto port = PipeWirePort::createUnique();

    REQUIRE(port);

    port->setPortId(77);
    port->setName("capture_FL");
    port->setDirection("Output");

    REQUIRE(port->uid() == "77");
    REQUIRE(port->portId() == 77);
    REQUIRE(port->name() == "capture_FL");
    REQUIRE(port->direction() == "Output");
}


TEST_CASE(
    "PipeWireLink describes an output to input connection",
    "[sound][model][pipewire][link][usage]")
{
    auto link = PipeWireLink::createUnique();

    REQUIRE(link);

    link->setLinkId(100);
    link->setOutputNodeId(10);
    link->setOutputPortId(11);
    link->setInputNodeId(20);
    link->setInputPortId(21);

    REQUIRE(link->uid() == "100");

    REQUIRE(link->linkId() == 100);

    REQUIRE(link->outputNodeId() == 10);
    REQUIRE(link->outputPortId() == 11);

    REQUIRE(link->inputNodeId() == 20);
    REQUIRE(link->inputPortId() == 21);
}


TEST_CASE(
    "PipeWireGraphNode owns ports by unique pointer",
    "[sound][model][pipewire][graph][usage]")
{
    auto node = PipeWireGraphNode::createUnique();

    REQUIRE(node);
    REQUIRE(node->ports());

    node->setUid("50");
    node->setNodeId(50);
    node->setName("Audio Sink");
    node->setMediaClass("Audio/Sink");

    auto port = PipeWirePort::createUnique();

    port->setPortId(51);
    port->setName("playback_FL");
    port->setDirection("Input");

    PipeWirePort *portPtr = port.get();

    node->ports()->insert(
        std::move(port)
        );

    REQUIRE(node->ports()->size() == 1);

    auto *stored =
        node->ports()->at("51");

    REQUIRE(stored);
    REQUIRE(stored == portPtr);
}


TEST_CASE(
    "VirtualEqBand can be stored using its uid",
    "[sound][model][dsp][eq][usage]")
{
    VirtualEqBand band;

    band.setUid("17");
    band.setBandIndex(17);
    band.setFrequency(1000.0f);

    REQUIRE(band.uid() == "17");
}


TEST_CASE(
    "VirtualMixerChannel derives uid from channel index",
    "[sound][model][dsp][mixer][usage]")
{
    VirtualMixerChannel channel;

    channel.setChannelIndex(4);

    REQUIRE(channel.uid() == "4");
}


//
// Block 2: ownership / copies / edge cases
//


TEST_CASE(
    "Moving an AlsaControl unique pointer transfers ownership",
    "[sound][model][alsa][control][ownership]")
{
    auto control = AlsaControl::createUnique();

    control->setUid("Master");

    AlsaControl *raw = control.get();

    AlsaDevice device;

    device.controls()->insert(
        std::move(control)
        );

    REQUIRE_FALSE(control);
    REQUIRE(device.controls()->at("Master") == raw);
}


TEST_CASE(
    "Removing an AlsaControl destroys ownership in the model",
    "[sound][model][alsa][control][ownership]")
{
    AlsaDevice device;

    auto control = AlsaControl::createUnique();

    control->setUid("Master");

    device.controls()->insert(
        std::move(control)
        );

    REQUIRE(
        device.controls()->contains("Master")
        );

    REQUIRE(
        device.controls()->remove("Master")
        );

    REQUIRE_FALSE(
        device.controls()->contains("Master")
        );

    REQUIRE(device.controls()->isEmpty());
}


TEST_CASE(
    "Taking an AlsaControl transfers ownership out of the model",
    "[sound][model][alsa][control][ownership]")
{
    AlsaDevice device;

    auto control = AlsaControl::createUnique();

    control->setUid("PCM");
    control->setVolume(55);

    device.controls()->insert(
        std::move(control)
        );

    auto taken =
        device.controls()->take("PCM");

    REQUIRE(taken);
    REQUIRE(taken->uid() == "PCM");
    REQUIRE(taken->volume() == 55);

    REQUIRE(device.controls()->isEmpty());
}


TEST_CASE(
    "AlsaDevice copy constructs independent control objects",
    "[sound][model][alsa][device][copy]")
{
    AlsaDevice original;

    original.setUid("hw:0,0:playback");
    original.setName("Playback");

    auto control = AlsaControl::createUnique();

    control->setUid("Master");
    control->setVolume(75);

    original.controls()->insert(
        std::move(control)
        );

    AlsaDevice copy{original};

    REQUIRE(copy.uid() == original.uid());

    REQUIRE(copy.controls());
    REQUIRE(original.controls());

    REQUIRE(copy.controls() != original.controls());

    auto *originalControl =
        original.controls()->at("Master");

    auto *copiedControl =
        copy.controls()->at("Master");

    REQUIRE(originalControl);
    REQUIRE(copiedControl);

    REQUIRE(copiedControl != originalControl);

    REQUIRE(
        copiedControl->volume() ==
        originalControl->volume()
        );

    copiedControl->setVolume(25);

    REQUIRE(copiedControl->volume() == 25);
    REQUIRE(originalControl->volume() == 75);
}


TEST_CASE(
    "AlsaDevice copy assignment replaces existing controls with independent copies",
    "[sound][model][alsa][device][copy]")
{
    AlsaDevice source;

    source.setUid("hw:1,0:capture");

    auto sourceControl =
        AlsaControl::createUnique();

    sourceControl->setUid("Capture");
    sourceControl->setVolume(65);

    source.controls()->insert(
        std::move(sourceControl)
        );

    AlsaDevice destination;

    destination.setUid("old-device");

    auto oldControl =
        AlsaControl::createUnique();

    oldControl->setUid("Old");
    oldControl->setVolume(1);

    destination.controls()->insert(
        std::move(oldControl)
        );

    destination = source;

    REQUIRE(destination.uid() == source.uid());

    REQUIRE_FALSE(
        destination.controls()->contains("Old")
        );

    REQUIRE(
        destination.controls()->contains("Capture")
        );

    REQUIRE(
        destination.controls()->at("Capture") !=
        source.controls()->at("Capture")
        );
}


TEST_CASE(
    "AlsaSoundCard copy constructs a fully independent ownership tree",
    "[sound][model][alsa][card][copy]")
{
    AlsaSoundCard original;

    original.setUid("hw:0");
    original.setPath("hw:0");
    original.setName("Internal Audio");

    auto device =
        AlsaDevice::createUnique();

    device->setUid("hw:0,0:playback");
    device->setName("Playback");

    auto control =
        AlsaControl::createUnique();

    control->setUid("Master");
    control->setVolume(85);

    device->controls()->insert(
        std::move(control)
        );

    original.devices()->insert(
        std::move(device)
        );

    AlsaSoundCard copy{original};

    REQUIRE(copy.uid() == original.uid());

    REQUIRE(copy.devices());
    REQUIRE(original.devices());

    REQUIRE(copy.devices() != original.devices());

    auto *originalDevice =
        original.devices()->at(
            "hw:0,0:playback"
            );

    auto *copiedDevice =
        copy.devices()->at(
            "hw:0,0:playback"
            );

    REQUIRE(originalDevice);
    REQUIRE(copiedDevice);

    REQUIRE(copiedDevice != originalDevice);

    auto *originalControl =
        originalDevice->controls()->at(
            "Master"
            );

    auto *copiedControl =
        copiedDevice->controls()->at(
            "Master"
            );

    REQUIRE(originalControl);
    REQUIRE(copiedControl);

    REQUIRE(copiedControl != originalControl);

    copiedControl->setVolume(10);

    REQUIRE(
        copiedControl->volume() == 10
        );

    REQUIRE(
        originalControl->volume() == 85
        );
}


TEST_CASE(
    "AlsaSoundCard copy assignment clears old ownership before cloning source",
    "[sound][model][alsa][card][copy]")
{
    AlsaSoundCard source;

    source.setUid("hw:5");

    auto sourceDevice =
        AlsaDevice::createUnique();

    sourceDevice->setUid(
        "hw:5,0:playback"
        );

    source.devices()->insert(
        std::move(sourceDevice)
        );

    AlsaSoundCard destination;

    destination.setUid("hw:old");

    auto oldDevice =
        AlsaDevice::createUnique();

    oldDevice->setUid(
        "hw:old,0:playback"
        );

    destination.devices()->insert(
        std::move(oldDevice)
        );

    destination = source;

    REQUIRE(destination.uid() == "hw:5");

    REQUIRE_FALSE(
        destination.devices()->contains(
            "hw:old,0:playback"
            )
        );

    REQUIRE(
        destination.devices()->contains(
            "hw:5,0:playback"
            )
        );

    REQUIRE(
        destination.devices()->at(
            "hw:5,0:playback"
            ) !=
        source.devices()->at(
            "hw:5,0:playback"
            )
        );
}


TEST_CASE(
    "Moving AlsaDevice transfers its owned control model",
    "[sound][model][alsa][device][move]")
{
    AlsaDevice source;

    source.setUid("hw:0,0:playback");

    auto control =
        AlsaControl::createUnique();

    control->setUid("Master");

    source.controls()->insert(
        std::move(control)
        );

    auto *originalModel =
        source.controls();

    AlsaDevice moved{
        std::move(source)
    };

    REQUIRE(moved.controls() == originalModel);

    REQUIRE(
        moved.controls()->contains("Master")
        );
}


TEST_CASE(
    "Moving AlsaSoundCard transfers its owned device model",
    "[sound][model][alsa][card][move]")
{
    AlsaSoundCard source;

    source.setUid("hw:0");

    auto device =
        AlsaDevice::createUnique();

    device->setUid(
        "hw:0,0:playback"
        );

    source.devices()->insert(
        std::move(device)
        );

    auto *originalModel =
        source.devices();

    AlsaSoundCard moved{
        std::move(source)
    };

    REQUIRE(moved.devices() == originalModel);

    REQUIRE(
        moved.devices()->contains(
            "hw:0,0:playback"
            )
        );
}


TEST_CASE(
    "PipeWireGraphNode copy constructs independent ports",
    "[sound][model][pipewire][graph][copy]")
{
    PipeWireGraphNode original;

    original.setUid("10");
    original.setNodeId(10);
    original.setName("Sink");

    auto port =
        PipeWirePort::createUnique();

    port->setPortId(11);
    port->setName("playback_FL");

    original.ports()->insert(
        std::move(port)
        );

    PipeWireGraphNode copy{original};

    REQUIRE(copy.uid() == original.uid());

    REQUIRE(copy.ports());
    REQUIRE(original.ports());

    REQUIRE(copy.ports() != original.ports());

    auto *originalPort =
        original.ports()->at("11");

    auto *copiedPort =
        copy.ports()->at("11");

    REQUIRE(originalPort);
    REQUIRE(copiedPort);

    REQUIRE(copiedPort != originalPort);

    copiedPort->setName("changed");

    REQUIRE(
        originalPort->name() ==
        "playback_FL"
        );

    REQUIRE(
        copiedPort->name() ==
        "changed"
        );
}


TEST_CASE(
    "PipeWireGraphNode copy assignment replaces its existing ports",
    "[sound][model][pipewire][graph][copy]")
{
    PipeWireGraphNode source;

    source.setUid("20");

    auto sourcePort =
        PipeWirePort::createUnique();

    sourcePort->setPortId(21);

    source.ports()->insert(
        std::move(sourcePort)
        );

    PipeWireGraphNode destination;

    destination.setUid("old");

    auto oldPort =
        PipeWirePort::createUnique();

    oldPort->setPortId(99);

    destination.ports()->insert(
        std::move(oldPort)
        );

    destination = source;

    REQUIRE(destination.uid() == "20");

    REQUIRE_FALSE(
        destination.ports()->contains("99")
        );

    REQUIRE(
        destination.ports()->contains("21")
        );

    REQUIRE(
        destination.ports()->at("21") !=
        source.ports()->at("21")
        );
}


TEST_CASE(
    "Empty AlsaDevice owns an empty control model",
    "[sound][model][alsa][device][edge]")
{
    AlsaDevice device;

    REQUIRE(device.controls());
    REQUIRE(device.controls()->isEmpty());
    REQUIRE(device.controls()->size() == 0);
}


TEST_CASE(
    "Empty AlsaSoundCard owns an empty device model",
    "[sound][model][alsa][card][edge]")
{
    AlsaSoundCard card;

    REQUIRE(card.devices());
    REQUIRE(card.devices()->isEmpty());
    REQUIRE(card.devices()->size() == 0);
}


TEST_CASE(
    "Empty PipeWireGraphNode owns an empty port model",
    "[sound][model][pipewire][graph][edge]")
{
    PipeWireGraphNode node;

    REQUIRE(node.ports());
    REQUIRE(node.ports()->isEmpty());
}


TEST_CASE(
    "Changing PipeWirePort id changes its derived uid",
    "[sound][model][pipewire][port][edge]")
{
    PipeWirePort port;

    port.setPortId(10);

    REQUIRE(port.uid() == "10");

    port.setPortId(500);

    REQUIRE(port.uid() == "500");
}


TEST_CASE(
    "Changing PipeWireLink id changes its derived uid",
    "[sound][model][pipewire][link][edge]")
{
    PipeWireLink link;

    link.setLinkId(10);

    REQUIRE(link.uid() == "10");

    link.setLinkId(900);

    REQUIRE(link.uid() == "900");
}


//
// Block 3: benchmarks / stress
//

#ifdef JOB_TEST_BENCHMARKS


TEST_CASE(
    "AlsaSoundCard deep copy benchmark",
    "[sound][model][alsa][card][benchmark]")
{
    AlsaSoundCard card;

    card.setUid("hw:0");

    constexpr int deviceCount = 16;
    constexpr int controlsPerDevice = 32;

    for (int d = 0; d < deviceCount; ++d) {
        auto device =
            AlsaDevice::createUnique();

        device->setUid(
            "hw:0," +
            std::to_string(d) +
            ":playback"
            );

        for (int c = 0;
             c < controlsPerDevice;
             ++c) {

            auto control =
                AlsaControl::createUnique();

            control->setUid(
                "control-" +
                std::to_string(c)
                );

            control->setVolume(c);

            device->controls()->insert(
                std::move(control)
                );
        }

        card.devices()->insert(
            std::move(device)
            );
    }

    REQUIRE(
        card.devices()->size() ==
        deviceCount
        );

    BENCHMARK(
        "deep copy card with devices and controls")
    {
        AlsaSoundCard copy{card};

        return copy.devices()->size();
    };
}


TEST_CASE(
    "PipeWireGraphNode port model copy benchmark",
    "[sound][model][pipewire][graph][benchmark]")
{
    PipeWireGraphNode node;

    node.setUid("1");

    constexpr int portCount = 256;

    for (int i = 0;
         i < portCount;
         ++i) {

        auto port =
            PipeWirePort::createUnique();

        port->setPortId(
            static_cast<std::uint32_t>(
                i + 1
                )
            );

        port->setName(
            "port-" +
            std::to_string(i)
            );

        node.ports()->insert(
            std::move(port)
            );
    }

    REQUIRE(
        node.ports()->size() ==
        portCount
        );

    BENCHMARK(
        "deep copy graph node with 256 ports")
    {
        PipeWireGraphNode copy{node};

        return copy.ports()->size();
    };
}


#endif // JOB_TEST_BENCHMARKS