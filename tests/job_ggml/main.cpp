#define CATCH_CONFIG_RUNNER

#include <catch2/catch_session.hpp>

#include <job_ggml_device_manager.h>

#include "test_ggml_utils.h"

job::ggml::JobGgmlDeviceManager *g_deviceManager{nullptr};

int main(int argc, char *argv[])
{
    /*
     * Keep the device manager stack-local so all GGML backend objects are
     * destroyed before main() returns and before process-level GPU runtime
     * teardown begins.
     */
    job::ggml::JobGgmlDeviceManager manager;

    g_deviceManager = &manager;

    const int result =
        Catch::Session{}.run(argc, argv);

    g_deviceManager = nullptr;

    return result;
}