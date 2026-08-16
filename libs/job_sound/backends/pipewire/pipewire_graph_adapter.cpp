#include "pipewire_graph_adapter.h"

#include <cstdlib>
#include <cstring>
#include <string>

#include <job_logger.h>

namespace job::sound {

namespace {

const pw_registry_events g_registryEvents = {
    .version = PW_VERSION_REGISTRY,
    .global = PipeWireGraphAdapter::onGlobal,
    .global_remove = PipeWireGraphAdapter::onGlobalRemove
};

[[nodiscard]] std::uint32_t toUint(const char *value) noexcept
{
    if (!value)
        return 0;

    char *end = nullptr;
    const unsigned long result = std::strtoul(value, &end, 10);

    if (end == value)
        return 0;

    return static_cast<std::uint32_t>(result);
}

[[nodiscard]] std::string property(const spa_dict *props, const char *key)
{
    if (!props || !key)
        return {};

    const char *value = spa_dict_lookup(props, key);
    return value ? std::string{value} : std::string{};
}

} // namespace


PipeWireGraphAdapter::PipeWireGraphAdapter() :
    m_nodes(std::make_unique<NodeModel>()),
    m_links(std::make_unique<LinkModel>())
{
    JOB_LOG_DEBUG("[PipeWireGraphAdapter] Initializing PipeWire...");

    pw_init(nullptr, nullptr);
    m_loop = pw_main_loop_new(nullptr);

    if (!m_loop) {
        JOB_LOG_WARN("[PipeWireGraphAdapter] Failed to create PipeWire main loop");
        return;
    }

    // Create an eventfd trigger that unblocks epoll_wait across threads
    struct pw_loop *pwLoop = pw_main_loop_get_loop(m_loop);
    m_stopEvent = pw_loop_add_event(
        pwLoop,
        [](void *data, std::uint64_t) {
            auto *loop = static_cast<pw_main_loop *>(data);
            if (loop) {
                pw_main_loop_quit(loop);
            }
        },
        m_loop);

    m_context = pw_context_new(pw_main_loop_get_loop(m_loop), nullptr, 0);
    if (!m_context) {
        JOB_LOG_WARN("[PipeWireGraphAdapter] Failed to create PipeWire context");
        cleanup();
        return;
    }

    m_core = pw_context_connect(m_context, nullptr, 0);
    if (!m_core) {
        JOB_LOG_WARN("[PipeWireGraphAdapter] Failed to connect to PipeWire core");
        cleanup();
        return;
    }

    m_registry = pw_core_get_registry(m_core, PW_VERSION_REGISTRY, 0);
    if (!m_registry) {
        JOB_LOG_WARN("[PipeWireGraphAdapter] Failed to get PipeWire registry");
        cleanup();
        return;
    }

    pw_registry_add_listener(m_registry,
                             &m_registryListener,
                             &g_registryEvents,
                             this);

    JOB_LOG_DEBUG("[PipeWireGraphAdapter] Registry listener connected");
    m_thread.setRunFunction([this](std::stop_token token) {
        run(token);
    });

    const auto result = m_thread.start();

    if (result != job::threads::JobThread::StartResult::Started) {
        JOB_LOG_WARN("[PipeWireGraphAdapter] Failed to start graph thread");
        cleanup();
    }
}


PipeWireGraphAdapter::~PipeWireGraphAdapter()
{
    stop();
    (void)m_thread.join();

    cleanup();

    pw_deinit();
}

pw_core *PipeWireGraphAdapter::coreHandle() noexcept
{
    return m_core;
}

const pw_core *PipeWireGraphAdapter::coreHandle() const noexcept
{
    return m_core;
}

threads::JobThread::StartResult PipeWireGraphAdapter::start()
{
    return m_thread.start();
}

void PipeWireGraphAdapter::stop() noexcept
{
    m_thread.requestStop();
    if (m_loop) {
        pw_main_loop_quit(m_loop);
        if (m_stopEvent) {
            struct pw_loop *pwLoop = pw_main_loop_get_loop(m_loop);
            if (pwLoop) {
                // Wakes up epoll_wait immediately across threads
                pw_loop_signal_event(pwLoop, m_stopEvent);
            }
        }
    }
    (void)m_thread.join();
}

bool PipeWireGraphAdapter::isRunning() const noexcept
{
    return m_thread.isRunning();
}

const PipeWireGraphAdapter::LinkModel *PipeWireGraphAdapter::links() const noexcept
{
    return m_links.get();
}

PipeWireGraphAdapter::LinkModel *PipeWireGraphAdapter::links() noexcept
{
    return m_links.get();
}

const PipeWireGraphAdapter::NodeModel *PipeWireGraphAdapter::nodes() const noexcept
{
    return m_nodes.get();
}

PipeWireGraphAdapter::NodeModel *PipeWireGraphAdapter::nodes() noexcept
{
    return m_nodes.get();
}


void PipeWireGraphAdapter::run(std::stop_token token)
{
    if (!m_loop)
        return;

    JOB_LOG_DEBUG("[PipeWireGraphAdapter] Main loop thread started");
    const int result = pw_main_loop_run(m_loop);

    if (result < 0 && !token.stop_requested())
        JOB_LOG_WARN("[PipeWireGraphAdapter] Main loop failed: {}", spa_strerror(result));

    JOB_LOG_DEBUG("[PipeWireGraphAdapter] Main loop thread stopped");
}


void PipeWireGraphAdapter::cleanup() noexcept
{
    if (m_registry) {
        spa_hook_remove(&m_registryListener);
        pw_proxy_destroy(reinterpret_cast<pw_proxy *>(m_registry));
        m_registry = nullptr;
    }

    if (m_core) {
        pw_core_disconnect(m_core);
        m_core = nullptr;
    }

    if (m_context) {
        pw_context_destroy(m_context);
        m_context = nullptr;
    }

    if (m_stopEvent) {
        if (m_loop) {
            pw_loop_destroy_source(pw_main_loop_get_loop(m_loop), m_stopEvent);
        }
        m_stopEvent = nullptr;
    }

    if (m_loop) {
        pw_main_loop_destroy(m_loop);
        m_loop = nullptr;
    }

    if (m_nodes && !m_nodes->isEmpty())
        m_nodes->clear();

    if (m_links && !m_links->isEmpty())
        m_links->clear();
}


void PipeWireGraphAdapter::onGlobal(void *data,
                                    std::uint32_t id,
                                    std::uint32_t,
                                    const char *type,
                                    std::uint32_t,
                                    const spa_dict *props)
{
    auto *self = static_cast<PipeWireGraphAdapter *>(data);
    if (!self || !type)
        return;

    std::lock_guard<std::mutex> lock(self->m_graphMutex);

    //
    // Node
    //

    if (std::strcmp(type, "PipeWire:Interface:Node") == 0) {
        const std::string name = property(props, "node.name");
        const std::string mediaClass = property(props, "media.class");
        auto node = PipeWireGraphNode::createUnique();

        node->setNodeId(id);
        node->setUid(std::to_string(id));
        node->setName(name);
        node->setMediaClass(mediaClass);

        try {
            self->m_nodes->insert(std::move(node));
        } catch (const std::exception &e) {
            JOB_LOG_WARN("[PipeWireGraphAdapter] Failed to add node {}: {}", id, e.what());
            return;
        }

        JOB_LOG_DEBUG("[PipeWireGraphAdapter] Node {} {} {}", id, name, mediaClass);
        return;
    }

    //
    // Port
    //

    if (std::strcmp(type, "PipeWire:Interface:Port") == 0) {

        const std::uint32_t portId = id;
        const std::string name = property(props, "port.name");
        const std::string direction = property(props, "port.direction");

        const std::uint32_t nodeId = toUint(props ? spa_dict_lookup(props, "node.id") : nullptr);
        if (nodeId == 0 || name.empty()) {
            JOB_LOG_WARN("[PipeWireGraphAdapter] Skipping invalid port {}", portId);
            return;
        }

        const std::string nodeUid = std::to_string(nodeId);
        if (!self->m_nodes->contains(nodeUid)) {
            JOB_LOG_WARN("[PipeWireGraphAdapter] Couldn't find node {} for port {}", nodeId, portId);
            return;
        }

        PipeWireGraphNode *node = self->m_nodes->at(nodeUid);
        if (!node || !node->ports())
            return;

        auto port = PipeWirePort::createUnique();
        port->setPortId(portId);
        port->setName(name);
        port->setDirection(direction);

        try {
            node->ports()->insert(std::move(port));
        } catch (const std::exception &e) {
            JOB_LOG_WARN("[PipeWireGraphAdapter] Failed to add port {}: {}", portId, e.what());
            return;
        }

        JOB_LOG_DEBUG("[PipeWireGraphAdapter] Port {} {} added to node {}", portId, name, nodeId);
        return;
    }

    //
    // Link
    //

    if (std::strcmp(type, "PipeWire:Interface:Link") == 0) {

        const std::uint32_t outputNodeId = toUint(props ? spa_dict_lookup(props, "link.output.node") : nullptr);
        const std::uint32_t outputPortId = toUint(props ? spa_dict_lookup(props, "link.output.port") : nullptr);
        const std::uint32_t inputNodeId  = toUint(props ? spa_dict_lookup(props, "link.input.node")  : nullptr);
        const std::uint32_t inputPortId =  toUint(props ? spa_dict_lookup(props, "link.input.port")  : nullptr);
        if (outputNodeId == 0 || inputNodeId == 0)
            return;

        auto link = PipeWireLink::createUnique();
        link->setLinkId(id);
        link->setOutputNodeId(outputNodeId);
        link->setOutputPortId(outputPortId);
        link->setInputNodeId(inputNodeId);
        link->setInputPortId(inputPortId);

        try {
            self->m_links->insert(std::move(link));
        } catch (const std::exception &e) {
            JOB_LOG_WARN("[PipeWireGraphAdapter] Failed to add link {}: {}", id, e.what());
            return;
        }

        JOB_LOG_DEBUG("[PipeWireGraphAdapter] Link {} from {}:{} to {}:{}", id, outputNodeId, outputPortId, inputNodeId, inputPortId);
        return;
    }
}


void PipeWireGraphAdapter::onGlobalRemove(void *data, std::uint32_t id)
{
    auto *self = static_cast<PipeWireGraphAdapter *>(data);
    if (!self)
        return;

    std::lock_guard<std::mutex> lock(self->m_graphMutex);
    const std::string uid = std::to_string(id);

    //
    // Nodes
    //

    if (self->m_nodes->remove(uid)) {
        JOB_LOG_DEBUG("[PipeWireGraphAdapter] Node removed: {}", id);
        return;
    }

    //
    // Links
    //

    if (self->m_links->remove(uid)) {
        JOB_LOG_DEBUG("[PipeWireGraphAdapter] Link removed: {}", id);
        return;
    }

    //
    // Ports
    //
    // Ports live inside the owning node, so search each node's
    // PortModel for this globally-assigned PipeWire ID.
    //

    for (auto &item : *self->m_nodes) {
        auto &node = item.second;

        if (!node || !node->ports())
            continue;

        if (node->ports()->remove(uid)) {
            JOB_LOG_DEBUG("[PipeWireGraphAdapter] Port removed: {}", id);
            return;
        }
    }

    JOB_LOG_DEBUG("[PipeWireGraphAdapter] Unknown object removed: {}", id);
}

} // namespace job::sound