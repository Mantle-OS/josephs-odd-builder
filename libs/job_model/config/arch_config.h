#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include <job_base_obj.h>

#include "model_architecture.h"
#include "jobmodel_export.h"

namespace job::model {

// Architecture-specific quirks and specialized knobs.
// Because every model family thinks standard transformer math is just a suggestion.
class JOBMODEL_EXPORT ArchConfig : public job::core::BaseObject
{
public:
    using Ptr  = std::shared_ptr<ArchConfig>;
    using WPtr = std::weak_ptr<ArchConfig>;
    using UPtr = std::unique_ptr<ArchConfig>;

    ArchConfig();
    ~ArchConfig() = default;

    [[nodiscard]] static Ptr createShared() { return std::make_shared<ArchConfig>(); }
    [[nodiscard]] static UPtr createUniq()  { return std::make_unique<ArchConfig>(); }

    ArchConfig(const ArchConfig &) = default;
    ArchConfig &operator=(const ArchConfig &) = default;
    ArchConfig(ArchConfig &&) noexcept = default;
    ArchConfig &operator=(ArchConfig &&) noexcept = default;

    [[nodiscard]] ModelArchitecture arch() const noexcept { return m_arch; }
    void setArch(ModelArchitecture arch) noexcept { m_arch = arch; }

    [[nodiscard]] const std::string &archName() const noexcept { return m_archName; }
    void setArchName(std::string archName) { m_archName = std::move(archName); }

    [[nodiscard]] const std::string &modelName() const noexcept { return m_modelName; }
    void setModelName(std::string modelName) { m_modelName = std::move(modelName); }

    // Kept for ModelConfigConcept: string_view-returning name, independent of storage.
    [[nodiscard]] std::string_view architectureName() const noexcept { return m_archName; }

    // Activation function flavor (e.g., "silu", "gelu", "relu")
    [[nodiscard]] const std::string &hiddenActivation() const noexcept { return m_hiddenActivation; }
    void setHiddenActivation(std::string activation) { m_hiddenActivation = std::move(activation); }

    [[nodiscard]] bool isValid() const noexcept;

private:
    ModelArchitecture   m_arch{ModelArchitecture::Unknown};
    std::string         m_archName{"unknown"};
    std::string         m_modelName{"unknown"};    
    std::string         m_hiddenActivation{"silu"};
};

} // namespace job::model
