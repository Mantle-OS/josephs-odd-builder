#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include <pipewire_device.h>
#include <pipewire_graph_adapter.h>
#include <pipewire_link.h>
#include <pipewire_link_layer.h>
#include <pipewire_port.h>
#include <pipewire_stream.h>

using namespace job::sound;
using namespace std::chrono_literals;

namespace {

template <typename Predicate>
[[nodiscard]] bool waitUntil(Predicate&& predicate, std::chrono::milliseconds timeout = 1000ms) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate()) return true;
        std::this_thread::sleep_for(5ms);
    }
    return predicate();
}

[[nodiscard]] bool settleAndStop(PipeWireGraphAdapter& adapter, std::chrono::milliseconds discoveryTime = 250ms) {
    // Registry discovery is asynchronous on the PipeWire main-loop thread.
    std::this_thread::sleep_for(discoveryTime);
    adapter.stop();
    return waitUntil([&adapter]() { return !adapter.isRunning(); });
}

[[nodiscard]] PipeWireGraphNode* findNodeContainingPort(PipeWireGraphAdapter::NodeModel& nodes, const std::string& portUid) {
    for (auto& [_, node] : nodes) {
        if (node && node->ports() && node->ports()->contains(portUid)) {
            return node.get();
        }
    }
    return nullptr;
}

} // namespace

// ============================================================================
// Block 1: Usage / Examples
// ============================================================================

TEST_CASE("PipeWireDevice stores discovery metadata", "[sound][pipewire][device][usage]") {
    auto device = PipeWireDevice::createUnique();
    REQUIRE(device);

    device->updateFromDiscovery("42", "USB Audio", "Output");

    CHECK(device->uid() == "42");
    CHECK(device->name() == "USB Audio");
    CHECK(device->direction() == "Output");
}

TEST_CASE("PipeWirePort represents one graph port", "[sound][pipewire][port][usage]") {
    auto port = PipeWirePort::createUnique();
    REQUIRE(port);

    port->setPortId(101);
    port->setName("playback_FL");
    port->setDirection("Input");

    CHECK(port->uid() == "101");
    CHECK(port->portId() == 101);
    CHECK(port->name() == "playback_FL");
    CHECK(port->direction() == "Input");
}

TEST_CASE("PipeWireLink represents an output to input graph connection", "[sound][pipewire][link][usage]") {
    auto link = PipeWireLink::createUnique();
    REQUIRE(link);

    link->setLinkId(500);
    link->setOutputNodeId(10);
    link->setOutputPortId(11);
    link->setInputNodeId(20);
    link->setInputPortId(21);

    CHECK(link->uid() == "500");
    CHECK(link->outputNodeId() == 10);
    CHECK(link->outputPortId() == 11);
    CHECK(link->inputNodeId() == 20);
    CHECK(link->inputPortId() == 21);
}

TEST_CASE("PipeWireGraphNode owns discovered ports", "[sound][pipewire][graph][node][usage]") {
    PipeWireGraphNode node;
    node.setUid("10");
    node.setNodeId(10);
    node.setName("Test Node");
    node.setMediaClass("Audio/Sink");

    auto port = PipeWirePort::createUnique();
    port->setPortId(11);
    port->setName("playback_FL");
    port->setDirection("Input");

    node.ports()->insert(std::move(port));

    REQUIRE(node.ports()->size() == 1);
    auto* stored = node.ports()->at("11");
    REQUIRE(stored);
    CHECK(stored->portId() == 11);
    CHECK(stored->name() == "playback_FL");
}

TEST_CASE("PipewireLinkLayer builds a straight connection between two ports", "[sound][pipewire][linklayer][usage]") {
    PipewireLinkLayer layer;
    PipewireLinkLayer::LinkModel links;

    auto link = PipeWireLink::createUnique();
    link->setLinkId(100);
    link->setOutputNodeId(10);
    link->setOutputPortId(11);
    link->setInputNodeId(20);
    link->setInputPortId(21);
    links.insert(std::move(link));

    layer.setPortPosition(11, {10.0f, 20.0f});
    layer.setPortPosition(21, {110.0f, 70.0f});
    layer.setLineType(PipewireLinkLayer::LineType::Straight);

    const auto geometry = layer.build(links);

    REQUIRE(geometry.lines.size() == 1);
    REQUIRE(geometry.triangles.empty());
    REQUIRE(geometry.lines[0].points.size() == 2);

    CHECK(geometry.lines[0].points[0].x == 10.0f);
    CHECK(geometry.lines[0].points[0].y == 20.0f);
    CHECK(geometry.lines[0].points[1].x == 110.0f);
    CHECK(geometry.lines[0].points[1].y == 70.0f);
}

TEST_CASE("PipewireLinkLayer builds a Bezier connection", "[sound][pipewire][linklayer][bezier][usage]") {
    PipewireLinkLayer layer;
    PipewireLinkLayer::LinkModel links;

    auto link = PipeWireLink::createUnique();
    link->setLinkId(200);
    link->setOutputPortId(1);
    link->setInputPortId(2);
    links.insert(std::move(link));

    layer.setPortPosition(1, {0.0f, 0.0f});
    layer.setPortPosition(2, {200.0f, 100.0f});
    layer.setLineType(PipewireLinkLayer::LineType::Bezier);

    const auto geometry = layer.build(links);

    REQUIRE(geometry.lines.size() == 1);
    REQUIRE(geometry.lines[0].points.size() == 20);
    CHECK(geometry.lines[0].points.front().x == 0.0f);
    CHECK(geometry.lines[0].points.front().y == 0.0f);
    CHECK(geometry.lines[0].points.back().x == 200.0f);
    CHECK(geometry.lines[0].points.back().y == 100.0f);
}

TEST_CASE("PipewireLinkLayer builds a filled triangle arrowhead", "[sound][pipewire][linklayer][arrow][usage]") {
    PipewireLinkLayer layer;
    PipewireLinkLayer::LinkModel links;

    auto link = PipeWireLink::createUnique();
    link->setLinkId(300);
    link->setOutputPortId(1);
    link->setInputPortId(2);
    links.insert(std::move(link));

    layer.setPortPosition(1, {0.0f, 0.0f});
    layer.setPortPosition(2, {100.0f, 0.0f});
    layer.setLineType(PipewireLinkLayer::LineType::TriangleArrow);

    const auto geometry = layer.build(links);

    REQUIRE(geometry.lines.size() == 1);
    REQUIRE(geometry.triangles.size() == 1);
    CHECK(geometry.triangles.front().a.x == 100.0f);
    CHECK(geometry.triangles.front().a.y == 0.0f);
}

TEST_CASE("PipeWireGraphAdapter connects to the active PipeWire session", "[sound][pipewire][graph][hardware][usage]") {
    PipeWireGraphAdapter adapter;
    if (!adapter.coreHandle()) SKIP("No active PipeWire core is available");

    REQUIRE(waitUntil([&adapter]() { return adapter.isRunning(); }));
    adapter.stop();
    REQUIRE(waitUntil([&adapter]() { return !adapter.isRunning(); }));
    // REQUIRE(waitUntil([&adapter]() { return !adapter.isRunning(); }, std::chrono::milliseconds(2000)));
}

TEST_CASE("PipeWireGraphAdapter discovers nodes from the live graph", "[sound][pipewire][graph][hardware][node][usage]") {
    PipeWireGraphAdapter adapter;
    if (!adapter.coreHandle()) SKIP("No active PipeWire core is available");

    REQUIRE(settleAndStop(adapter));
    REQUIRE(adapter.nodes());

    if (adapter.nodes()->isEmpty()) SKIP("PipeWire session contains no discoverable nodes");

    for (const auto& [_, node] : *adapter.nodes()) {
        REQUIRE(node);
        CHECK_FALSE(node->uid().empty());
        CHECK(node->uid() == std::to_string(node->nodeId()));
        CHECK(adapter.nodes()->contains(node->uid()));
    }
}

TEST_CASE("PipeWireGraphAdapter discovered ports belong to graph nodes", "[sound][pipewire][graph][hardware][port][usage]") {
    PipeWireGraphAdapter adapter;
    if (!adapter.coreHandle()) SKIP("No active PipeWire core is available");

    REQUIRE(settleAndStop(adapter));
    if (!adapter.nodes() || adapter.nodes()->isEmpty()) SKIP("PipeWire session contains no discoverable nodes");

    std::size_t portCount = 0;
    std::unordered_set<std::string> globalPortUids;

    for (const auto& [_, node] : *adapter.nodes()) {
        REQUIRE(node);
        REQUIRE(node->ports());

        for (const auto& [_, port] : *node->ports()) {
            REQUIRE(port);
            ++portCount;

            CHECK_FALSE(port->uid().empty());
            CHECK(port->uid() == std::to_string(port->portId()));
            CHECK(globalPortUids.insert(port->uid()).second); // Globally unique across the graph
        }
    }

    if (portCount == 0) SKIP("PipeWire graph contains no discovered ports");
}

TEST_CASE("PipeWireGraphAdapter discovers valid live links", "[sound][pipewire][graph][hardware][link][usage]") {
    PipeWireGraphAdapter adapter;
    if (!adapter.coreHandle()) SKIP("No active PipeWire core is available");

    REQUIRE(settleAndStop(adapter));
    REQUIRE(adapter.links());

    if (adapter.links()->isEmpty()) SKIP("PipeWire graph currently contains no links");

    for (const auto& [_, link] : *adapter.links()) {
        REQUIRE(link);
        CHECK_FALSE(link->uid().empty());
        CHECK(link->uid() == std::to_string(link->linkId()));
        CHECK(link->outputNodeId() != 0);
        CHECK(link->outputPortId() != 0);
        CHECK(link->inputNodeId() != 0);
        CHECK(link->inputPortId() != 0);
    }
}

TEST_CASE("PipeWire output stream can initialize and shut down", "[sound][pipewire][stream][output][hardware][usage]") {
    PipeWireStream stream{PipeWireStream::Direction::Output};
    if (!stream.isValid()) SKIP("Unable to initialize a PipeWire output stream");

    REQUIRE(waitUntil([&stream]() { return stream.isRunning(); }));
    stream.stop();
    REQUIRE(waitUntil([&stream]() { return !stream.isRunning(); }));
}

TEST_CASE("PipeWire input stream can initialize and shut down", "[sound][pipewire][stream][input][hardware][usage]") {
    PipeWireStream stream{PipeWireStream::Direction::Input};
    if (!stream.isValid()) SKIP("Unable to initialize a PipeWire input stream");

    REQUIRE(waitUntil([&stream]() { return stream.isRunning(); }));
    stream.stop();
    REQUIRE(waitUntil([&stream]() { return !stream.isRunning(); }));
}

TEST_CASE("PipeWire output stream accepts a mono audio buffer", "[sound][pipewire][stream][output][buffer][usage]") {
    PipeWireStream stream{PipeWireStream::Direction::Output};
    if (!stream.isValid()) SKIP("Unable to initialize a PipeWire output stream");

    // Intentionally silent test buffer
    stream.enqueue(std::vector<float>(512, 0.0f));
    std::this_thread::sleep_for(50ms);

    stream.stop();
    REQUIRE(waitUntil([&stream]() { return !stream.isRunning(); }));
}

// ============================================================================
// Block 2: Edge Cases / Invariants
// ============================================================================

TEST_CASE("PipeWirePort uid follows its global port id", "[sound][pipewire][port][uid][edge]") {
    PipeWirePort port;
    port.setPortId(1);
    CHECK(port.uid() == "1");

    port.setPortId(999);
    CHECK(port.uid() == "999");
}

TEST_CASE("PipeWireLink uid follows its global link id", "[sound][pipewire][link][uid][edge]") {
    PipeWireLink link;
    link.setLinkId(10);
    CHECK(link.uid() == "10");

    link.setLinkId(9999);
    CHECK(link.uid() == "9999");
}

TEST_CASE("PipeWireGraphNode begins with an owned empty port model", "[sound][pipewire][graph][node][edge]") {
    PipeWireGraphNode node;
    REQUIRE(node.ports());
    REQUIRE(node.ports()->isEmpty());
}

TEST_CASE("PipewireLinkLayer ignores links whose port positions are unknown", "[sound][pipewire][linklayer][edge]") {
    PipewireLinkLayer layer;
    PipewireLinkLayer::LinkModel links;

    auto link = PipeWireLink::createUnique();
    link->setLinkId(1);
    link->setOutputPortId(10);
    link->setInputPortId(20);
    links.insert(std::move(link));

    REQUIRE(layer.build(links).isEmpty());
}

TEST_CASE("PipewireLinkLayer handles coincident arrow endpoints", "[sound][pipewire][linklayer][arrow][edge]") {
    PipewireLinkLayer layer;
    PipewireLinkLayer::LinkModel links;

    auto link = PipeWireLink::createUnique();
    link->setLinkId(1);
    link->setOutputPortId(10);
    link->setInputPortId(20);
    links.insert(std::move(link));

    layer.setPortPosition(10, {50.0f, 50.0f});
    layer.setPortPosition(20, {50.0f, 50.0f});
    layer.setLineType(PipewireLinkLayer::LineType::TriangleArrow);

    const auto geometry = layer.build(links);
    REQUIRE(geometry.lines.size() == 1);
    REQUIRE(geometry.triangles.size() == 1);
}

TEST_CASE("PipeWireGraphAdapter can remove a node by global id", "[sound][pipewire][graph][remove][edge]") {
    PipeWireGraphAdapter adapter;
    if (adapter.coreHandle()) REQUIRE(settleAndStop(adapter));

    constexpr std::uint32_t nodeId = 0xFFFF0001u;
    const std::string uid = std::to_string(nodeId);

    auto node = PipeWireGraphNode::createUnique();
    node->setNodeId(nodeId);
    node->setUid(uid);
    node->setName("Synthetic Test Node");

    REQUIRE_FALSE(adapter.nodes()->contains(uid));
    adapter.nodes()->insert(std::move(node));
    REQUIRE(adapter.nodes()->contains(uid));

    PipeWireGraphAdapter::onGlobalRemove(&adapter, nodeId);
    REQUIRE_FALSE(adapter.nodes()->contains(uid));
}

TEST_CASE("PipeWireGraphAdapter can remove a nested port by global id", "[sound][pipewire][graph][remove][port][edge]") {
    PipeWireGraphAdapter adapter;
    if (adapter.coreHandle()) REQUIRE(settleAndStop(adapter));

    constexpr std::uint32_t nodeId = 0xFFFF0010u;
    constexpr std::uint32_t portId = 0xFFFF0011u;
    const std::string nodeUid = std::to_string(nodeId);
    const std::string portUid = std::to_string(portId);

    auto node = PipeWireGraphNode::createUnique();
    node->setNodeId(nodeId);
    node->setUid(nodeUid);

    auto port = PipeWirePort::createUnique();
    port->setPortId(portId);
    port->setName("Synthetic Port");
    port->setDirection("Output");

    node->ports()->insert(std::move(port));
    adapter.nodes()->insert(std::move(node));

    REQUIRE(adapter.nodes()->contains(nodeUid));
    auto* storedNode = adapter.nodes()->at(nodeUid);
    REQUIRE(storedNode);
    REQUIRE(storedNode->ports()->contains(portUid));

    PipeWireGraphAdapter::onGlobalRemove(&adapter, portId);
    REQUIRE_FALSE(storedNode->ports()->contains(portUid));
}

TEST_CASE("PipeWireGraphAdapter can remove a link by global id", "[sound][pipewire][graph][remove][link][edge]") {
    PipeWireGraphAdapter adapter;
    if (adapter.coreHandle()) REQUIRE(settleAndStop(adapter));

    constexpr std::uint32_t linkId = 0xFFFF0020u;
    const std::string uid = std::to_string(linkId);

    auto link = PipeWireLink::createUnique();
    link->setLinkId(linkId);
    link->setOutputNodeId(1);
    link->setOutputPortId(2);
    link->setInputNodeId(3);
    link->setInputPortId(4);

    adapter.links()->insert(std::move(link));
    REQUIRE(adapter.links()->contains(uid));

    PipeWireGraphAdapter::onGlobalRemove(&adapter, linkId);
    REQUIRE_FALSE(adapter.links()->contains(uid));
}

// ============================================================================
// Block 3: Benchmarks
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("PipewireLinkLayer graph geometry benchmark", "[sound][pipewire][linklayer][benchmark]") {
    PipewireLinkLayer layer;
    PipewireLinkLayer::LinkModel links;
    constexpr std::uint32_t linkCount = 256;

    for (std::uint32_t i = 0; i < linkCount; ++i) {
        const std::uint32_t outputPort = 1000u + i * 2u;
        const std::uint32_t inputPort  = outputPort + 1u;

        auto link = PipeWireLink::createUnique();
        link->setLinkId(i + 1u);
        link->setOutputPortId(outputPort);
        link->setInputPortId(inputPort);
        links.insert(std::move(link));

        layer.setPortPosition(outputPort, {0.0f, static_cast<float>(i) * 4.0f});
        layer.setPortPosition(inputPort, {500.0f, static_cast<float>(i) * 4.0f});
    }

    layer.setLineType(PipewireLinkLayer::LineType::Bezier);

    BENCHMARK("build Bezier geometry for 256 PipeWire links") {
        return layer.build(links);
    };
}

#endif // JOB_TEST_BENCHMARKS