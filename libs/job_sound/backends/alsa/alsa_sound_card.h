#pragma once

#include <memory>
#include <string>
#include <utility>

#include <job_obj_hash.h>

#include "alsa_device.h"
#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT AlsaSoundCard
{
public:
    using Ptr  = std::shared_ptr<AlsaSoundCard>;
    using WPtr = std::weak_ptr<AlsaSoundCard>;
    using UPtr = std::unique_ptr<AlsaSoundCard>;

    using MVC = core::JobObjHashFast<AlsaDevice::UPtr>;

    static Ptr createShared() { return std::make_shared<AlsaSoundCard>(); }
    static UPtr createUnique() { return std::make_unique<AlsaSoundCard>(); }

    explicit AlsaSoundCard()
        : m_devices(std::make_unique<MVC>())
    {
    }

    AlsaSoundCard(const AlsaSoundCard &other) :
        m_uid(other.m_uid),
        m_name(other.m_name),
        m_path(other.m_path),
        m_card(other.m_card),
        m_chip(other.m_chip),
        m_driver(other.m_driver),
        m_longName(other.m_longName),
        m_components(other.m_components),
        m_devices(std::make_unique<MVC>())
    {
        if (!other.m_devices)
            return;

        m_devices->reserve(other.m_devices->size());

        for (const auto &[uid, device] : *other.m_devices) {
            if (!device)
                continue;

            m_devices->insert(std::make_unique<AlsaDevice>(*device));
        }
    }

    AlsaSoundCard(AlsaSoundCard &&) noexcept = default;

    AlsaSoundCard &operator=(const AlsaSoundCard &other)
    {
        if (this == &other)
            return *this;

        m_uid        = other.m_uid;
        m_name       = other.m_name;
        m_path       = other.m_path;
        m_card       = other.m_card;
        m_chip       = other.m_chip;
        m_driver     = other.m_driver;
        m_longName   = other.m_longName;
        m_components = other.m_components;

        if (!m_devices)
            m_devices = std::make_unique<MVC>();
        else if (!m_devices->isEmpty())
            m_devices->clear();

        if (!other.m_devices)
            return *this;

        m_devices->reserve(other.m_devices->size());
        for (const auto &item : *other.m_devices) {
            const auto &device = item.second;
            if (!device)
                continue;

            m_devices->insert(std::make_unique<AlsaDevice>(*device));
        }

        return *this;
    }

    AlsaSoundCard &operator=(AlsaSoundCard &&) noexcept = default;

    ~AlsaSoundCard() = default;

    [[nodiscard]] const std::string &uid() const noexcept { return m_uid; }
    [[nodiscard]] const std::string &name() const noexcept { return m_uid; }
    [[nodiscard]] const std::string &path() const noexcept { return m_path; }
    [[nodiscard]] const std::string &card() const noexcept { return m_card; }
    [[nodiscard]] const std::string &chip() const noexcept { return m_chip; }
    [[nodiscard]] const std::string &driver() const noexcept { return m_driver; }
    [[nodiscard]] const std::string &longName() const noexcept { return m_longName; }
    [[nodiscard]] const std::string &components() const noexcept { return m_components; }

    [[nodiscard]] MVC *devices() noexcept { return m_devices.get(); }
    [[nodiscard]] const MVC *devices() const noexcept { return m_devices.get(); }

    void setUid(const std::string &uid) { m_uid = uid; }
    void setName(const std::string &name) { m_name = name; }
    void setPath(const std::string &path) { m_path = path; }
    void setCard(const std::string &card) { m_card = card; }
    void setChip(const std::string &chip) { m_chip = chip; }
    void setDriver(const std::string &driver) { m_driver = driver; }
    void setLongName(const std::string &longName) { m_longName = longName; }
    void setComponents(const std::string &components) { m_components = components; }

private:
    std::string m_uid;
    std::string m_name;
    std::string m_path;
    std::string m_card;
    std::string m_chip;
    std::string m_driver;
    std::string m_longName;
    std::string m_components;

    std::unique_ptr<MVC> m_devices;
};

} // namespace job::sound