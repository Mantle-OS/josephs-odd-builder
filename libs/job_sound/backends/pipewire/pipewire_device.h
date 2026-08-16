#pragma once

#include <memory>
#include <string>
#include <utility>

#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT PipeWireDevice
{
public:
    using Ptr  = std::shared_ptr<PipeWireDevice>;
    using WPtr = std::weak_ptr<PipeWireDevice>;
    using UPtr = std::unique_ptr<PipeWireDevice>;

    static Ptr createShared() { return std::make_shared<PipeWireDevice>(); }
    static UPtr createUnique() { return std::make_unique<PipeWireDevice>(); }

    explicit PipeWireDevice() = default;

    PipeWireDevice(const PipeWireDevice &) = default;
    PipeWireDevice(PipeWireDevice &&) noexcept = default;

    PipeWireDevice &operator=(const PipeWireDevice &) = default;
    PipeWireDevice &operator=(PipeWireDevice &&) noexcept = default;

    ~PipeWireDevice() = default;

    [[nodiscard]] const std::string &uid() const noexcept { return m_uid; }
    [[nodiscard]] const std::string &name() const noexcept { return m_name; }
    [[nodiscard]] const std::string &direction() const noexcept { return m_direction; }

    void setUid(const std::string &uid) { m_uid = uid; }
    void setName(const std::string &name) { m_name = name; }
    void setDirection(const std::string &direction) { m_direction = direction; }

    void updateFromDiscovery(const std::string &id, const std::string &label, const std::string &dir)
    {
        setUid(id);
        setName(label);
        setDirection(dir);
    }

    friend class PipeWireManager;

private:
    std::string m_uid;
    std::string m_name;
    std::string m_direction; // "Input" or "Output"
};

} // namespace job::sound