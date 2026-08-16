#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <mutex>

#include <pipewire/pipewire.h>
#include <spa/utils/result.h>

#include <job_obj_hash.h>
#include <job_thread.h>

#include "jobsound_export.h"
#include "pipewire_graph_node.h"
#include "pipewire_link.h"

namespace job::sound {

class JOBSOUND_EXPORT PipeWireGraphAdapter
{
public:
    using Ptr  = std::shared_ptr<PipeWireGraphAdapter>;
    using WPtr = std::weak_ptr<PipeWireGraphAdapter>;
    using UPtr = std::unique_ptr<PipeWireGraphAdapter>;

    using NodeModel = core::JobObjHashFast<PipeWireGraphNode::UPtr>;
    using LinkModel = core::JobObjHashFast<PipeWireLink::UPtr>;

    static Ptr createShared()
    {
        return std::make_shared<PipeWireGraphAdapter>();
    }

    static UPtr createUnique()
    {
        return std::make_unique<PipeWireGraphAdapter>();
    }

    static PipeWireGraphAdapter *instance()
    {
        static PipeWireGraphAdapter s_instance;
        return &s_instance;
    }
    static pw_core *core() { return instance()->m_core; }

    explicit PipeWireGraphAdapter();
    ~PipeWireGraphAdapter();

    PipeWireGraphAdapter(const PipeWireGraphAdapter &) = delete;
    PipeWireGraphAdapter(PipeWireGraphAdapter &&) = delete;

    PipeWireGraphAdapter &operator=(const PipeWireGraphAdapter &) = delete;
    PipeWireGraphAdapter &operator=(PipeWireGraphAdapter &&) = delete;

    [[nodiscard]] NodeModel *nodes() noexcept;
    [[nodiscard]] const NodeModel *nodes() const noexcept;
    [[nodiscard]] LinkModel *links() noexcept;
    [[nodiscard]] const LinkModel *links() const noexcept;
    [[nodiscard]] pw_core *coreHandle() noexcept;
    [[nodiscard]] const pw_core *coreHandle() const noexcept;


    [[nodiscard]] threads::JobThread::StartResult start();
    void stop() noexcept;
    [[nodiscard]] bool isRunning() const noexcept;

    static void onGlobal(void *data,
                         std::uint32_t id,
                         std::uint32_t permissions,
                         const char *type,
                         std::uint32_t version,
                         const spa_dict *props);

    static void onGlobalRemove(void *data, std::uint32_t id);

private:
    void run(std::stop_token token);
    void cleanup() noexcept;

private:
    std::unique_ptr<NodeModel>  m_nodes;
    std::unique_ptr<LinkModel>  m_links;
    mutable std::mutex          m_graphMutex;
    pw_main_loop                *m_loop       = nullptr;
    spa_source                  *m_stopEvent  = nullptr;
    pw_context                  *m_context    = nullptr;
    pw_core                     *m_core       = nullptr;
    pw_registry                 *m_registry   = nullptr;
    spa_hook                    m_registryListener{};
    job::threads::JobThread     m_thread;
};

} // namespace job::sound