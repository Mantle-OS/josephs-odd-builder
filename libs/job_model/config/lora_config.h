#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>

#include <job_base_obj.h>
#include <job_list.h>

#include "jobmodel_export.h"

namespace job::model {

// A single LoRA (Low-Rank Adaptation) weight delta to apply on top of the
// base model. Pure data -- loading the file and actually applying the
// delta against LinearGraph is a separate, not-yet-built concern (see
// lora_graph.* once LinearGraph integration is clear).
class JOBMODEL_EXPORT Lora : public job::core::BaseObject
{
public:
    using Ptr  = std::shared_ptr<Lora>;
    using WPtr = std::weak_ptr<Lora>;
    using UPtr = std::unique_ptr<Lora>;

    Lora();
    ~Lora() = default;

    [[nodiscard]] static Ptr createShared() { return std::make_shared<Lora>(); }
    [[nodiscard]] static UPtr createUniq()  { return std::make_unique<Lora>(); }

    Lora(const Lora &) = default;
    Lora &operator=(const Lora &) = default;
    Lora(Lora &&) noexcept = default;
    Lora &operator=(Lora &&) noexcept = default;

    // Blend strength applied when this LoRA's delta is composed with the
    // base weights. Negative values are legitimate (subtracting a LoRA's
    // effect is a real technique), so only finiteness is enforced --
    // throws on NaN/Inf, not on sign or magnitude.
    [[nodiscard]] float multiplier() const noexcept { return m_multiplier; }
    void setMultiplier(float value);

    [[nodiscard]] const std::filesystem::path &path() const noexcept { return m_path; }
    void setPath(std::filesystem::path path) { m_path = std::move(path); }

    [[nodiscard]] bool isEnabled() const noexcept { return m_isEnabled; }
    void setEnabled(bool value) noexcept { m_isEnabled = value; }

    // Never touches disk -- only checks in-memory state (finite
    // multiplier, non-empty path). Whether the file actually exists is a
    // loader's concern, not this class's, same reasoning as every other
    // config's isValid() staying I/O-free.
    [[nodiscard]] bool isValid() const noexcept;

private:
    float                 m_multiplier{1.0f};
    std::filesystem::path m_path;
    bool                  m_isEnabled{true};
};

// An ordered set of LoRAs to apply on top of the base model.
class JOBMODEL_EXPORT LoraConfig : public job::core::BaseObject
{
public:
    using Ptr  = std::shared_ptr<LoraConfig>;
    using WPtr = std::weak_ptr<LoraConfig>;
    using UPtr = std::unique_ptr<LoraConfig>;

    LoraConfig();
    ~LoraConfig() = default;

    [[nodiscard]] static Ptr createShared() { return std::make_shared<LoraConfig>(); }
    [[nodiscard]] static UPtr createUniq()  { return std::make_unique<LoraConfig>(); }

    LoraConfig(const LoraConfig &) = default;
    LoraConfig &operator=(const LoraConfig &) = default;
    LoraConfig(LoraConfig &&) noexcept = default;
    LoraConfig &operator=(LoraConfig &&) noexcept = default;

    // Adds a LoRA to the end of the application order.
    void append(Lora lora);

    // Adds a LoRA to the front of the application order.
    void prepend(Lora lora);

    // Marks the LoRA at index x as disabled without removing it from
    // loras() the entry stays present (e.g. for later re-enabling) and
    // simply stops counting toward isValid() or application. Bounds
    // checking (throws std::out_of_range) comes straight from
    // JobList::at(), same as before.
    void disable(int x);

    [[nodiscard]] core::JobList<Lora> &loras() noexcept { return m_loras; }
    [[nodiscard]] const core::JobList<Lora> &loras() const noexcept { return m_loras; }

    // Only enabled LoRAs are checked a disabled entry with a bogus
    // path/multiplier is inert and shouldn't block overall validity.
    // Same "absence/inactivity doesn't poison validity" reasoning as
    // ModelConfig::hasMoeConfig().
    [[nodiscard]] bool isValid() const noexcept;

private:
    core::JobList<Lora> m_loras;
};

} // namespace job::model