#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <job_logger.h>
#include <job_obj_hash.h>

#include "dsp_manager.h"
#include "jobsound_export.h"
#include "pipewire_device.h"
#include "pipewire_stream.h"

namespace job::sound {

class JOBSOUND_EXPORT PipeWireManager
{
public:
    using Ptr  = std::shared_ptr<PipeWireManager>;
    using WPtr = std::weak_ptr<PipeWireManager>;
    using UPtr = std::unique_ptr<PipeWireManager>;

    using DeviceModel =
        core::JobObjHashFast<PipeWireDevice::UPtr>;

    static Ptr createShared()
    {
        return std::make_shared<PipeWireManager>();
    }

    static UPtr createUnique()
    {
        return std::make_unique<PipeWireManager>();
    }

    explicit PipeWireManager()
        : m_devices(std::make_unique<DeviceModel>()),
        m_processingManager(
            AudioProcessingManager::createUnique()
            )
    {
        populatePipeWireDevices();
    }

    PipeWireManager(const PipeWireManager &) = delete;
    PipeWireManager(PipeWireManager &&) = delete;

    PipeWireManager &operator=(const PipeWireManager &) = delete;
    PipeWireManager &operator=(PipeWireManager &&) = delete;

    ~PipeWireManager()
    {
        stopCapture();
        stopPlayback();
    }

    [[nodiscard]] DeviceModel *devices() noexcept
    {
        return m_devices.get();
    }

    [[nodiscard]] const DeviceModel *devices() const noexcept
    {
        return m_devices.get();
    }

    [[nodiscard]] PipeWireDevice *selectedInputDevice() noexcept
    {
        return m_selectedInputDevice;
    }

    [[nodiscard]] const PipeWireDevice *selectedInputDevice() const noexcept
    {
        return m_selectedInputDevice;
    }

    [[nodiscard]] PipeWireDevice *selectedOutputDevice() noexcept
    {
        return m_selectedOutputDevice;
    }

    [[nodiscard]] const PipeWireDevice *selectedOutputDevice() const noexcept
    {
        return m_selectedOutputDevice;
    }

    [[nodiscard]] AudioProcessingManager *processingManager() noexcept
    {
        return m_processingManager.get();
    }

    [[nodiscard]] const AudioProcessingManager *processingManager() const noexcept
    {
        return m_processingManager.get();
    }

    [[nodiscard]] PipeWireStream *captureStream() noexcept
    {
        return m_captureStream.get();
    }

    [[nodiscard]] const PipeWireStream *captureStream() const noexcept
    {
        return m_captureStream.get();
    }

    [[nodiscard]] PipeWireStream *playbackStream() noexcept
    {
        return m_playbackStream.get();
    }

    [[nodiscard]] const PipeWireStream *playbackStream() const noexcept
    {
        return m_playbackStream.get();
    }

    void setSelectedInputDevice(PipeWireDevice *device)
    {
        if (device == m_selectedInputDevice)
            return;

        m_selectedInputDevice = device;
        onSelectedInputChanged(device);
    }

    void setSelectedOutputDevice(PipeWireDevice *device)
    {
        if (device == m_selectedOutputDevice)
            return;

        m_selectedOutputDevice = device;
        onSelectedOutputChanged(device);
    }

private:
    void populatePipeWireDevices()
    {
        //
        // [[FIXME]] Replace dummy devices with PipeWire registry
        // discovery through PipeWireGraphAdapter / registry callbacks.
        //

        auto input = PipeWireDevice::createUnique();

        input->updateFromDiscovery("input1", "Microphone", "Input");
        PipeWireDevice *inputPtr = input.get();
        m_devices->insert(std::move(input));

        auto output =
            PipeWireDevice::createUnique();

        output->updateFromDiscovery(
            "output1",
            "Speakers",
            "Output"
            );

        PipeWireDevice *outputPtr = output.get();

        m_devices->insert(
            std::move(output)
            );

        setSelectedInputDevice(inputPtr);
        setSelectedOutputDevice(outputPtr);
    }

    void onSelectedInputChanged(
        PipeWireDevice *device)
    {
        stopCapture();

        if (!device)
            return;

        m_captureStream =
            PipeWireStream::createUnique(
                PipeWireStream::Direction::Input
                );

        if (!m_captureStream->isValid()) {
            JOB_LOG_WARN(
                "[PipeWireManager] Failed to create capture stream for device: {}",
                device->name()
                );

            m_captureStream.reset();
            return;
        }

        m_captureStream->setSamplesReady(
            [this](const std::vector<float> &samples) {
                if (m_processingManager)
                    m_processingManager->feedSamples(samples);
            }
            );

        JOB_LOG_DEBUG(
            "[PipeWireManager] Started capture stream from device: {}",
            device->name()
            );
    }

    void onSelectedOutputChanged(
        PipeWireDevice *device)
    {
        stopPlayback();

        if (!device)
            return;

        m_playbackStream =
            PipeWireStream::createUnique(
                PipeWireStream::Direction::Output
                );

        if (!m_playbackStream->isValid()) {
            JOB_LOG_WARN(
                "[PipeWireManager] Failed to create playback stream for device: {}",
                device->name()
                );

            m_playbackStream.reset();
            return;
        }

        if (m_processingManager) {
            m_processingManager->setRenderRequested([this](const std::vector<float> &samples, AudioProcessingManager::StreamType type) {
                if (type == AudioProcessingManager::StreamType::Playback && m_playbackStream)
                    m_playbackStream->enqueue(samples);
            });
        }

        JOB_LOG_DEBUG("[PipeWireManager] Started playback stream to device: {}", device->name());
    }

    void stopCapture() noexcept
    {
        if (!m_captureStream)
            return;

        m_captureStream->stop();
        m_captureStream.reset();
    }

    void stopPlayback() noexcept
    {
        if (!m_playbackStream)
            return;

        m_playbackStream->stop();
        m_playbackStream.reset();
    }

private:
    std::unique_ptr<DeviceModel> m_devices;

    PipeWireDevice *m_selectedInputDevice  = nullptr;
    PipeWireDevice *m_selectedOutputDevice = nullptr;

    AudioProcessingManager::UPtr m_processingManager;

    PipeWireStream::UPtr m_captureStream;
    PipeWireStream::UPtr m_playbackStream;
};

} // namespace job::sound