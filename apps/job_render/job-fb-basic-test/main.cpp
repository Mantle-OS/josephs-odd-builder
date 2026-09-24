#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

#include <job_render_application.h>

int main(int argc, char *argv[])
{
    std::vector<std::string> arguments;
    arguments.reserve(static_cast<std::size_t>(argc) + 2);

    arguments.emplace_back(argv[0]);

    for (int i = 1; i < argc; ++i)
        arguments.emplace_back(argv[i]);

    arguments.emplace_back("--backend");
    arguments.emplace_back("framebuffer");

    std::vector<char *> argumentPointers;
    argumentPointers.reserve(arguments.size());

    for (auto &argument : arguments)
        argumentPointers.push_back(argument.data());

    auto app = job::render::JobRenderApplication::createUniq(static_cast<int>(argumentPointers.size()), argumentPointers.data());
    if (!app->start())
        return EXIT_FAILURE;

    // get the backend framebuffer
    auto *backend = app->framebufferBackend();
    if (!backend)
        return EXIT_FAILURE;

    // get the framebuffer
    auto framebuffer = backend->framebuffer();
    if (!framebuffer.valid())
        return EXIT_FAILURE;

    // fill it white
    std::ranges::fill(framebuffer.bytes(), std::byte{0xFF});


    std::jthread timer([&app]() {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        app->quit();
    });

    return app->exec();
}