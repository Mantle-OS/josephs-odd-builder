#define CATCH_CONFIG_RUNNER

#include <catch2/catch_session.hpp>
#include <job_ggml.h>
#include "test_ggml_utils.h"

job::ggml::JobGgml *g_jobGgml{nullptr}; // Borrowed from main() stack.

int main(int argc, char *argv[])
{
    job::ggml::JobGgml ggml;
    ggml.setLogCallback([](ggml_log_level level, const char *text, void *userData) {
        // SHUT UP
        (void)level;
        (void)text;
        (void)userData;
        return;
        // switch (level) {
        // case GGML_LOG_LEVEL_DEBUG:
        // case GGML_LOG_LEVEL_NONE:
        // case GGML_LOG_LEVEL_INFO:
        // case GGML_LOG_LEVEL_WARN:
        // case GGML_LOG_LEVEL_ERROR:
        // case GGML_LOG_LEVEL_CONT:
        //     return;
        // }
    });

    g_jobGgml = &ggml;
    const int result = Catch::Session{}.run(argc, argv);
    g_jobGgml = nullptr;

    return result;
}