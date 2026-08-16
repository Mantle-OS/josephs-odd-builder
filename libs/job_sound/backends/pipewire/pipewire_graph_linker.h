#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <pipewire/pipewire.h>
#include <pipewire/keys.h>
#include <pipewire/link.h>

#include <job_logger.h>

#include "jobsound_export.h"
#include "pipewire_graph_adapter.h"

namespace job::sound {

class JOBSOUND_EXPORT PipewireGraphLinker
{
public:
    using Ptr  = std::shared_ptr<PipewireGraphLinker>;
    using WPtr = std::weak_ptr<PipewireGraphLinker>;
    using UPtr = std::unique_ptr<PipewireGraphLinker>;

    enum class State : std::uint8_t {
        WaitingForSource,
        WaitingForTarget
    };

    static Ptr createShared() { return std::make_shared<PipewireGraphLinker>(); }
    static UPtr createUnique() { return std::make_unique<PipewireGraphLinker>(); }

    static PipewireGraphLinker *instance()
    {
        static PipewireGraphLinker s_instance;
        return &s_instance;
    }

    explicit PipewireGraphLinker() = default;

    PipewireGraphLinker(const PipewireGraphLinker &) = delete;
    PipewireGraphLinker(PipewireGraphLinker &&) = delete;

    PipewireGraphLinker &operator=(const PipewireGraphLinker &) = delete;
    PipewireGraphLinker &operator=(PipewireGraphLinker &&) = delete;

    ~PipewireGraphLinker() = default;

    [[nodiscard]] State state() const noexcept { return m_state; }
    [[nodiscard]] std::uint32_t selectedSourceNode() const noexcept { return m_srcNode; }
    [[nodiscard]] std::uint32_t selectedSourcePort() const noexcept { return m_srcPort; }

    void cancel() noexcept
    {
        JOB_LOG_DEBUG("[PipewireGraphLinker] Cancelled patch in progress.");
        resetState();
    }

    void handlePortClick(std::uint32_t portId, const std::string &direction, std::uint32_t nodeId)
    {
        // First click on an Output port
        if (m_state == State::WaitingForSource && direction == "Output") {
            m_srcNode = nodeId;
            m_srcPort = portId;
            m_state = State::WaitingForTarget;

            JOB_LOG_DEBUG("[PipewireGraphLinker] Selected output: {} on node {}", portId, nodeId);
            return;
        }

        // Second click on an Input port -> Complete the link
        if (m_state == State::WaitingForTarget && direction == "Input") {
            // Prevent self-looping ports or invalid destinations
            if (m_srcPort == portId && m_srcNode == nodeId) {
                JOB_LOG_WARN("[PipewireGraphLinker] Cannot link a port to itself. Resetting.");
                resetState();
                return;
            }

            JOB_LOG_DEBUG("[PipewireGraphLinker] Connecting output {}:{} -> input {}:{}",
                          m_srcNode, m_srcPort, nodeId, portId);

            createLink(m_srcNode, m_srcPort, nodeId, portId);
            resetState();
            return;
        }

        // User clicked another output or invalid target -> Reset selection
        JOB_LOG_DEBUG("[PipewireGraphLinker] Invalid port selection state. Resetting.");
        resetState();
    }

    // Direct link creation
    bool createLink(std::uint32_t outputNode,
                    std::uint32_t outputPort,
                    std::uint32_t inputNode,
                    std::uint32_t inputPort,
                    bool linger = true)
    {
        auto *core = PipeWireGraphAdapter::core();
        if (!core) {
            JOB_LOG_WARN("[PipewireGraphLinker] No PipeWire core available.");
            return false;
        }

        pw_properties *props = pw_properties_new(
            PW_KEY_LINK_OUTPUT_NODE, std::to_string(outputNode).c_str(),
            PW_KEY_LINK_OUTPUT_PORT, std::to_string(outputPort).c_str(),
            PW_KEY_LINK_INPUT_NODE,  std::to_string(inputNode).c_str(),
            PW_KEY_LINK_INPUT_PORT,  std::to_string(inputPort).c_str(),
            PW_KEY_OBJECT_LINGER,    linger ? "true" : "false",
            nullptr
            );

        if (!props) {
            JOB_LOG_WARN("[PipewireGraphLinker] Failed to allocate link properties");
            return false;
        }

        auto *proxy = static_cast<pw_proxy *>(pw_core_create_object(core,
                                                                    "link-factory",
                                                                    PW_TYPE_INTERFACE_Link,
                                                                    PW_VERSION_LINK,
                                                                    &props->dict,
                                                                    0));

        pw_properties_free(props);

        if (!proxy) {
            JOB_LOG_WARN("[PipewireGraphLinker] Failed to create link proxy on server: {}:{} -> {}:{}",
                         outputNode, outputPort, inputNode, inputPort);
            return false;
        }

        JOB_LOG_INFO("[PipewireGraphLinker] Link created successfully: {}:{} -> {}:{}",
                     outputNode, outputPort, inputNode, inputPort);
        return true;
    }

    // Destroy an existing link by its PipeWire global object ID
    bool destroyLink(std::uint32_t linkId)
    {
        auto *core = PipeWireGraphAdapter::core();
        if (!core) {
            JOB_LOG_WARN("[PipewireGraphLinker] No PipeWire core available to destroy link.");
            return false;
        }

        auto *registry = pw_core_get_registry(core, PW_VERSION_REGISTRY, 0);
        if (!registry) {
            JOB_LOG_WARN("[PipewireGraphLinker] Failed to retrieve registry for link destruction.");
            return false;
        }

        // Send destroy request to server
        pw_registry_destroy(registry, linkId);
        pw_proxy_destroy(reinterpret_cast<pw_proxy *>(registry));

        JOB_LOG_INFO("[PipewireGraphLinker] Destroyed link {}", linkId);
        return true;
    }

private:
    void resetState() noexcept
    {
        m_state = State::WaitingForSource;
        m_srcNode = 0;
        m_srcPort = 0;
    }

private:
    State m_state = State::WaitingForSource;
    std::uint32_t m_srcNode = 0;
    std::uint32_t m_srcPort = 0;
};

} // namespace job::sound