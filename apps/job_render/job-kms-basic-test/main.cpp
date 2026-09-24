#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

#include <GL/gl.h>

#include <job_render_application.h>

int main(int argc, char *argv[])
{
    std::vector<std::string> arguments;
    arguments.reserve(static_cast<std::size_t>(argc) + 2);

    arguments.emplace_back(argv[0]);

    for (int i = 1; i < argc; ++i)
        arguments.emplace_back(argv[i]);

    arguments.emplace_back("--backend");
    arguments.emplace_back("kms");

    std::vector<char *> argumentPointers;
    argumentPointers.reserve(arguments.size());

    for (auto &argument : arguments)
        argumentPointers.push_back(argument.data());

    auto app = job::render::JobRenderApplication::createUniq(
        static_cast<int>(argumentPointers.size()),
        argumentPointers.data());

    if (!app->start())
        return EXIT_FAILURE;

    auto *backend = app->kmsBackend();
    if (!backend)
        return EXIT_FAILURE;

    auto *eglContext = backend->eglContext();
    auto *eglSurface = backend->eglSurface();

    if (!eglContext || !eglSurface)
        return EXIT_FAILURE;

    if (!eglContext->makeCurrent(eglSurface->surface(), eglSurface->surface()))
        return EXIT_FAILURE;

    glViewport(0, 0, static_cast<GLsizei>(backend->width()), static_cast<GLsizei>(backend->height()));
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!backend->present())
        return EXIT_FAILURE;

    std::jthread timer([&app]() {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        app->quit();
    });

    return app->exec();
}