#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <GL/gl.h>

#include <job_drm_damage.h>
#include <job_drm_modifier.h>
#include <job_gbm_buffer_lease.h>
#include <job_present_entry.h>
#include <job_present_frame.h>
#include <job_render_application.h>

    using namespace std::chrono_literals;

constexpr std::uint8_t kMaxLoops = 12;

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

    auto *drmDevice  = backend->drmDevice();
    auto *gbmSurface = backend->gbmSurface();
    auto *eglContext = backend->eglContext();
    auto *eglSurface = backend->eglSurface();
    auto *presenter  = backend->presenter();

    if (!drmDevice || !gbmSurface || !eglContext || !eglSurface || !presenter)
        return EXIT_FAILURE;

    if (!eglContext->makeCurrent(eglSurface->surface(), eglSurface->surface()))
        return EXIT_FAILURE;

    const auto width  = backend->width();
    const auto height = backend->height();

    if (width < 2 || height < 2)
        return EXIT_FAILURE;

    const auto *damageProperty = backend->plane().properties().find("FB_DAMAGE_CLIPS");
    if (!damageProperty) {
        std::cerr << "Selected KMS plane does not expose FB_DAMAGE_CLIPS.\n";
        return EXIT_FAILURE;
    }

    std::cout << "KMS damage test\n";
    std::cout << "  size:             " << width << 'x' << height << '\n';
    std::cout << "  plane:            " << backend->plane().id() << '\n';
    std::cout << "  FB_DAMAGE_CLIPS:  " << damageProperty->id() << '\n';
    std::cout << "  explicit mods:    " << (backend->explicitModifiers() ? "yes" : "no") << '\n';

    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));

    const std::int32_t screenWidth  = static_cast<std::int32_t>(width);
    const std::int32_t screenHeight = static_cast<std::int32_t>(height);

    const std::int32_t centerX = screenWidth / 2;
    const std::int32_t centerY = screenHeight / 2;

    const std::array<job::render::JobDrmDamageRect, 4> quadrants{
        job::render::JobDrmDamageRect{ 0,       0,       centerX,     centerY },
        job::render::JobDrmDamageRect{ centerX, 0,       screenWidth, centerY },
        job::render::JobDrmDamageRect{ centerX, centerY, screenWidth, screenHeight },
        job::render::JobDrmDamageRect{ 0,       centerY, centerX,     screenHeight }
    };

    const auto drawFrame = [&](std::size_t activeQuadrant) {
        glDisable(GL_SCISSOR_TEST);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        const auto &rect = quadrants[activeQuadrant];

        const GLint glX = rect.x1();
        const GLint glY = screenHeight - rect.y2();
        const GLsizei glWidth = rect.width();
        const GLsizei glHeight = rect.height();

        glEnable(GL_SCISSOR_TEST);
        glScissor(glX, glY, glWidth, glHeight);

        glClearColor(1.0f, 0.15f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glDisable(GL_SCISSOR_TEST);
        glFlush();
    };

    std::uint64_t frameId = 1;

    const auto presentFrame = [&](job::render::JobDrmDamage damage) -> bool {
        if (!eglSurface->swapBuffers()) {
            std::cerr << "eglSurface->swapBuffers() failed.\n";
            return false;
        }

        gbm_bo *buffer = gbmSurface->lockFrontBuffer();
        if (!buffer) {
            std::cerr << "gbmSurface->lockFrontBuffer() failed.\n";
            return false;
        }

        job::render::JobGbmBufferLease lease;

        if (!lease.reset(gbmSurface->surface(), buffer)) {
            std::cerr << "JobGbmBufferLease::reset() failed.\n";
            return false;
        }

        if (lease.planeCount() > job::render::JobDrmDevice::FramebufferCreateInfo::MaxPlanes) {
            std::cerr << "GBM buffer exposes too many planes.\n";
            return false;
        }

        const job::render::JobDrmFormat format = lease.format().drmFormat();

        if (format != backend->format()) {
            std::cerr << "GBM format does not match negotiated KMS format.\n";
            return false;
        }

        job::render::JobDrmModifier modifier = job::render::JobDrmModifier::invalid();

        if (backend->explicitModifiers()) {
            modifier = lease.modifier().drmModifier();

            if (!modifier.valid()) {
                std::cerr << "GBM buffer has no valid explicit DRM modifier.\n";
                return false;
            }

            if (!backend->plane().supportsFormatModifier(format, modifier)) {
                std::cerr << "Selected KMS plane does not support the GBM modifier.\n";
                return false;
            }
        }

        job::render::JobDrmDevice::FramebufferCreateInfo framebufferInfo{
            .width = lease.width(),
            .height = lease.height(),
            .format = format,
            .planeCount = lease.planeCount(),
            .modifier = modifier
        };

        for (std::size_t i = 0; i < lease.planeCount(); ++i) {
            const auto *plane = lease.plane(i);
            if (!plane) {
                std::cerr << "Missing GBM plane " << i << ".\n";
                return false;
            }

            framebufferInfo.handles[i] = plane->handle();
            framebufferInfo.pitches[i] = plane->stride();
            framebufferInfo.offsets[i] = plane->offset();
        }

        auto framebuffer = drmDevice->createFramebuffer(framebufferInfo);
        if (!framebuffer) {
            std::cerr << "Failed to register GBM buffer as DRM framebuffer.\n";
            return false;
        }

        job::render::JobPresentFrame frame{ frameId++, framebuffer->id() };

        if (!damage.empty())
            frame.setDamage(std::move(damage));

        job::render::JobPresentEntry entry{
            std::move(frame),
            std::move(*framebuffer),
            std::move(lease)
        };

        if (!presenter->enqueue(std::move(entry))) {
            std::cerr << "JobKmsPresenter::enqueue() failed.\n";
            return false;
        }

        if (!presenter->presentNext()) {
            std::cerr << "JobKmsPresenter::presentNext() failed.\n";
            return false;
        }

        return true;
    };

    /*
     * Initial frame.
     *
     * No damage metadata is supplied because this is the initial complete
     * scanout. The top-left quadrant begins active.
     */
    std::size_t activeQuadrant = 0;

    drawFrame(activeQuadrant);

    if (!presentFrame({}))
        return EXIT_FAILURE;

    std::cout << "Initial full frame presented.\n";

    std::this_thread::sleep_for(1s);

    /*
     * Move the active quadrant clockwise.
     *
     * Every new frame is rendered completely so every GBM back buffer contains
     * a coherent image. Only the two regions that logically changed are passed
     * to KMS through FB_DAMAGE_CLIPS:
     *
     *     old quadrant: active -> black
     *     new quadrant: black  -> active
     */
    for (std::uint8_t count = 0; count < kMaxLoops; ++count) {
        const std::size_t previousQuadrant = activeQuadrant;
        activeQuadrant = (activeQuadrant + 1) % quadrants.size();

        drawFrame(activeQuadrant);

        job::render::JobDrmDamage damage;
        damage.add(quadrants[previousQuadrant]);
        damage.add(quadrants[activeQuadrant]);

        std::cout << "Frame " << static_cast<unsigned>(count + 1)
                  << ": quadrant " << previousQuadrant
                  << " -> " << activeQuadrant
                  << ", damage rects=2\n";

        if (!presentFrame(std::move(damage))) {
            std::cerr << "Damage presentation failed on frame "
                      << static_cast<unsigned>(count + 1) << ".\n";

            app->quit(EXIT_FAILURE);
            return app->exec();
        }

        std::this_thread::sleep_for(1s);
    }

    std::cout << "KMS damage test complete.\n";

    app->quit(EXIT_SUCCESS);
    return app->exec();
}

